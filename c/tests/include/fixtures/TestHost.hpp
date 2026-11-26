#pragma once

#include "xplpc/c/platform.h"
#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// A host reached through the abi, which records every call it serves and everything its post callback carries, in the order it arrived.
class TestHost
{
public:
    enum class Serving
    {
        Inline,
        Later,
        Never,
        Raise,
        DetachSelf,
    };

    struct Invocation
    {
        xplpc_handle call;
        std::string request;
        bool owned;
        std::thread::id thread;
    };

    struct Post
    {
        xplpc_post_kind kind;
        std::uint64_t id;
        std::string data;
    };

    struct Outcome
    {
        bool answered;
        std::string value;
        xplpc_handle call;
    };

    explicit TestHost(bool enterableAnywhere = false, Serving mode = Serving::Inline, std::string answer = R"({"r":"from-host"})", const std::string &name = "")
        : serving(mode)
        , response(std::move(answer))
    {
        const xplpc_host host{this, &TestHost::onInvoke, &TestHost::onPost, enterableAnywhere, name.empty() ? nullptr : name.data(), name.size()};
        layer = xplpc_attach(&host);
        attached = layer != 0;
    }

    // The context this host hands the library has to outlive the layer, so nothing is torn down before the release arrived.
    ~TestHost()
    {
        releaseLater();
        joinWorkers();

        if (replaced)
        {
            std::unique_lock<std::mutex> lock(mutex);

            EXPECT_EQ(0u, releases);
            EXPECT_EQ(receivedWhenReplaced, received.size());
            EXPECT_EQ(servedWhenReplaced, served.size());

            return;
        }

        if (attached)
        {
            xplpc_detach(layer);
        }

        if (layer != 0 && !waitForRelease(5000))
        {
            ADD_FAILURE() << "The layer was never released";
        }

        joinWorkers();

        std::unique_lock<std::mutex> lock(mutex);

        EXPECT_LE(releases, 1u);
        EXPECT_EQ(0u, lateEvents);
        EXPECT_FALSE(releasedWhileInside);
    }

    TestHost(const TestHost &) = delete;
    TestHost &operator=(const TestHost &) = delete;

    xplpc_handle handle() const
    {
        return layer;
    }

    void detach()
    {
        attached = false;
        xplpc_detach(layer);
    }

    // Marks the moment another host is about to take this one's place, after which the library must never reach it again, not even to release it.
    void expectReplaced()
    {
        std::unique_lock<std::mutex> lock(mutex);

        replaced = true;
        attached = false;
        receivedWhenReplaced = received.size();
        servedWhenReplaced = served.size();
    }

    void declare(const std::string &name) const
    {
        xplpc_declare(layer, name.data(), name.size());
    }

    void retract(const std::string &name) const
    {
        xplpc_retract(layer, name.data(), name.size());
    }

    Outcome call(std::uint64_t token, const std::string &request) const
    {
        return call(token, request.data(), request.size());
    }

    Outcome call(std::uint64_t token, const char *request, size_t requestSize) const
    {
        xplpc_call_result result{};
        const auto answered = xplpc_call(layer, token, request, requestSize, &result);

        Outcome outcome{answered, std::string(), result.call};

        if (answered && result.data)
        {
            outcome.value = std::string(result.data, result.dataSize);
        }

        xplpc_free(result.data);

        return outcome;
    }

    bool waitForPosts(xplpc_post_kind kind, size_t count, int milliseconds)
    {
        std::unique_lock<std::mutex> lock(mutex);

        // clang-format off
        return condition.wait_for(lock, std::chrono::milliseconds(milliseconds), [this, kind, count] {
            return countLocked(kind) >= count;
        });
        // clang-format on
    }

    bool waitForRelease(int milliseconds)
    {
        return waitForPosts(XPLPC_POST_RELEASE, 1, milliseconds);
    }

    std::vector<Post> posts()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return received;
    }

    std::vector<Post> answers()
    {
        std::unique_lock<std::mutex> lock(mutex);

        std::vector<Post> value;

        for (const auto &post : received)
        {
            if (post.kind == XPLPC_POST_ANSWER)
            {
                value.push_back(post);
            }
        }

        return value;
    }

    std::vector<Invocation> invocations()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return served;
    }

    size_t count(xplpc_post_kind kind)
    {
        std::unique_lock<std::mutex> lock(mutex);
        return countLocked(kind);
    }

    // A host serving later answers each call only once the test lets it, so what the test asserts before that is never raced by the answer.
    void releaseLater()
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            laterReleased = true;
        }

        condition.notify_all();
    }

    void joinWorkers()
    {
        std::vector<std::thread> running;

        {
            std::unique_lock<std::mutex> lock(mutex);
            running.swap(workers);
        }

        for (auto &worker : running)
        {
            worker.join();
        }
    }

private:
    // Counts how deep the library has entered this host, so a release that arrives while a call is still inside it is caught.
    class Inside
    {
    public:
        explicit Inside(std::atomic<int> &depth)
            : depth(depth)
        {
            depth++;
        }

        ~Inside()
        {
            depth--;
        }

        Inside(const Inside &) = delete;
        Inside &operator=(const Inside &) = delete;

    private:
        std::atomic<int> &depth;
    };

    xplpc_handle layer = 0;
    std::atomic<bool> attached{false};
    std::atomic<int> depth{0};

    std::mutex mutex;
    std::condition_variable condition;
    Serving serving;
    std::string response;
    std::vector<Invocation> served;
    std::vector<Post> received;
    std::vector<std::thread> workers;
    size_t releases = 0;
    size_t lateEvents = 0;
    bool replaced = false;
    size_t receivedWhenReplaced = 0;
    size_t servedWhenReplaced = 0;
    bool releasedWhileInside = false;
    bool laterReleased = false;

    static void onInvoke(void *context, xplpc_handle call, const char *request, size_t requestSize)
    {
        static_cast<TestHost *>(context)->serveCall(call, std::string(request, requestSize), false);
    }

    static void onPost(void *context, xplpc_post_kind kind, std::uint64_t id, char *data, size_t dataSize)
    {
        // Everything a post carries belongs to the host, so it is read and then released with the allocator that produced it.

        auto host = static_cast<TestHost *>(context);
        const auto value = data ? std::string(data, dataSize) : std::string();

        xplpc_free(data);
        host->record(kind, id, value);

        if (kind == XPLPC_POST_INVOKE)
        {
            host->serveCall(id, value, true);
        }
    }

    void record(xplpc_post_kind kind, std::uint64_t id, const std::string &value)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);

            if (releases > 0)
            {
                lateEvents++;
            }

            if (kind == XPLPC_POST_RELEASE)
            {
                releases++;
                releasedWhileInside = releasedWhileInside || depth.load() > 0;
            }

            received.push_back(Post{kind, id, value});
        }

        condition.notify_all();
    }

    void serveCall(xplpc_handle call, const std::string &request, bool owned)
    {
        Inside inside(depth);

        Serving mode = Serving::Inline;
        std::string value;

        {
            std::unique_lock<std::mutex> lock(mutex);

            if (releases > 0)
            {
                lateEvents++;
            }

            served.push_back(Invocation{call, request, owned, std::this_thread::get_id()});
            mode = serving;
            value = response;
        }

        condition.notify_all();
        respond(mode, call, value);
    }

    void respond(Serving mode, xplpc_handle call, const std::string &value)
    {
        if (mode == Serving::Inline)
        {
            xplpc_answer(call, value.data(), value.size());
            return;
        }

        if (mode == Serving::Later)
        {
            std::unique_lock<std::mutex> lock(mutex);

            // clang-format off
            workers.emplace_back([this, call, value] {
                {
                    std::unique_lock<std::mutex> released(mutex);
                    condition.wait(released, [this] { return laterReleased; });
                }

                xplpc_answer(call, value.data(), value.size());
            });
            // clang-format on

            return;
        }

        if (mode == Serving::Raise)
        {
            throw std::runtime_error("the host raised while serving");
        }

        if (mode == Serving::DetachSelf)
        {
            // The answer after detaching has nobody left to reach, so it has to be a no-op.

            attached = false;
            xplpc_detach(layer);
            xplpc_answer(call, value.data(), value.size());
        }
    }

    size_t countLocked(xplpc_post_kind kind) const
    {
        size_t value = 0;

        for (const auto &post : received)
        {
            if (post.kind == kind)
            {
                value++;
            }
        }

        return value;
    }
};
