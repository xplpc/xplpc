#include "fixtures/AnswerWaiter.hpp"
#include "fixtures/CoroutineTest.hpp"
#include "fixtures/GeneralTest.hpp"
#include "fixtures/LogRecorder.hpp"
#include "fixtures/VerifiedCall.hpp"
#include "fixtures/ViewBytes.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

using namespace xplpc::core;
using namespace xplpc::client;
using namespace xplpc::message;
using namespace xplpc::type;

TEST_F(GeneralTest, ClientTestLogin)
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ("LOGGED-WITH-REMEMBER", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestLoginAsync)
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    // clang-format off
    auto task = [](Request awaited) -> CoroutineTest {
        auto response = co_await Client::callAsync<std::string>(awaited);
        EXPECT_EQ("LOGGED-WITH-REMEMBER", response.value());
    }(request);
    // clang-format on

    task.wait();
}

TEST_F(GeneralTest, ClientTestCallAsyncSuspendsForAnAnswerThatArrivesLater)
{
    // The awaitable has a path for an answer already there and one for an answer that is not, and this is the second.

    auto request = Request{"sample.async"};

    // clang-format off
    auto task = [](Request awaited) -> CoroutineTest {
        auto response = co_await Client::callAsync<std::string>(awaited);
        EXPECT_EQ(std::nullopt, response);
    }(request);
    // clang-format on

    task.wait();
}

TEST_F(GeneralTest, ClientTestCallAsyncFromAStringAnswersTheDocument)
{
    // The string form answers what the wire carried, the same way callSync does from a string.

    // clang-format off
    auto task = []() -> CoroutineTest {
        auto response = co_await Client::callAsync(std::string{R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})"});
        EXPECT_EQ(R"({"r":"LOGGED-WITH-REMEMBER"})", response);
    }();
    // clang-format on

    task.wait();
}

TEST_F(GeneralTest, ClientTestCallAsyncFromAStringAnswersEmptyWhenNothingOwnsTheFunction)
{
    // clang-format off
    auto task = []() -> CoroutineTest {
        auto response = co_await Client::callAsync(std::string{R"({"f":"not.found","p":[]})"});
        EXPECT_EQ("", response);
    }();
    // clang-format on

    task.wait();
}

TEST_F(GeneralTest, ClientSyncAnswersTheValueFromAnInlineMapping)
{
    // A mapping that answers before it returns has already resolved the call, so the value is there to be read.

    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    EXPECT_EQ("LOGGED-WITH-REMEMBER", Client::callSync<std::string>(request).value_or(""));
}

TEST_F(GeneralTest, ClientSyncAnswersEmptyWhenTheMappingDefers)
{
    // A mapping that answers later cannot be read synchronously, and the call it would have resolved is cancelled rather than left waiting.
    // The answer is then produced once the frame that asked for it is gone, so what it writes into has to outlive that frame.

    std::vector<Response> held;

    // clang-format off
    MappingList::shared()->add("test.sync.defers", Map::create<std::string>({}, [&held](const Message &, const Response &r) {
        held.push_back(r);
    }));
    // clang-format on

    const auto before = Router::shared().pending();

    EXPECT_EQ(std::nullopt, Client::callSync<std::string>(Request{"test.sync.defers"}));
    EXPECT_EQ(before, Router::shared().pending());

    // The mapping answers a call that was already cancelled, which resolves nothing.
    ASSERT_EQ(1u, held.size());
    held.front()(std::string{"late"});

    EXPECT_EQ(before, Router::shared().pending());
}

TEST_F(GeneralTest, ClientSyncAnswersEmptyWhenNothingOwnsTheFunction)
{
    const auto before = Router::shared().pending();

    EXPECT_EQ(std::nullopt, Client::callSync<std::string>(Request{"not.found"}));
    EXPECT_EQ(before, Router::shared().pending());
}

TEST_F(GeneralTest, ClientSyncAnswersNothingForAMismatchedType)
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    EXPECT_EQ(std::nullopt, Client::callSync<bool>(request));
}

TEST_F(GeneralTest, ClientSyncAnswersTheDocumentFromAString)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    EXPECT_EQ("{\"r\":\"LOGGED-WITH-REMEMBER\"}", Client::callSync(request));
}

TEST_F(GeneralTest, ClientSyncAnswersEmptyFromAStringWhenTheMappingDefers)
{
    std::vector<Response> held;

    // clang-format off
    MappingList::shared()->add("test.sync.defers.string", Map::create<std::string>({}, [&held](const Message &, const Response &r) {
        held.push_back(r);
    }));
    // clang-format on

    const auto before = Router::shared().pending();

    EXPECT_EQ("", Client::callSync(R"({"f":"test.sync.defers.string","p":[]})"));
    EXPECT_EQ(before, Router::shared().pending());

    // The mapping answers a call that was already cancelled, which resolves nothing.
    ASSERT_EQ(1u, held.size());
    held.front()(std::string{"late"});

    EXPECT_EQ(before, Router::shared().pending());
}

TEST_F(GeneralTest, ClientSyncAnswersEmptyFromAStringWhenNothingOwnsTheFunction)
{
    const auto before = Router::shared().pending();

    EXPECT_EQ("", Client::callSync(R"({"f":"not.found","p":[]})"));
    EXPECT_EQ(before, Router::shared().pending());
}

TEST_F(GeneralTest, ClientSyncAnswersWhatTheNestedMappingProduced)
{
    // The core reaches back for a mapping this side owns, and a nested answer produced inline is still there when the outer call returns.

    // clang-format off
    MappingList::shared()->add("platform.reverse.response", Map::create<std::string>({}, [](const Message &, const Response &r) {
        r(std::string{"ok"});
    }));
    // clang-format on

    EXPECT_EQ("response-is-ok", Client::callSync<std::string>(Request{"sample.reverse"}).value_or(""));
}

TEST_F(GeneralTest, ClientTestAsync)
{
    auto request = Request{"sample.async"};
    auto waiter = std::make_shared<AnswerWaiter<std::optional<std::string>>>();

    // clang-format off
    Client::call<std::string>(request, [waiter](const auto &response) {
        waiter->answer(response);
    });
    // clang-format on

    ASSERT_TRUE(waiter->waitFor(5000));
    EXPECT_EQ(std::nullopt, waiter->value());
}

TEST_F(GeneralTest, ClientAnswersWithWhatTheNestedMappingProduced)
{
    // The core reaches back for a mapping this side owns, and what it answers has to travel out through the call that started it.

    // clang-format off
    MappingList::shared()->add("platform.reverse.response", Map::create<std::string>({}, [](const Message &, const Response &r) {
        r(std::string{"ok"});
    }));
    // clang-format on

    auto request = Request{"sample.reverse"};

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ("response-is-ok", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientAnswersWithWhatTheNestedMappingProducedLater)
{
    // The inner mapping holds its responder past the call, and the outer call still receives what it built once that one answers.

    std::vector<Response> held;
    AnswerWaiter<std::string> waiter;

    // clang-format off
    MappingList::shared()->add("platform.reverse.response", Map::create<std::string>({}, [&held](const Message &, const Response &r) {
        held.push_back(r);
    }));

    Client::call<std::string>(Request{"sample.reverse"}, [&waiter](const auto &response) {
        waiter.answer(response.value_or(""));
    });
    // clang-format on

    ASSERT_EQ(1u, held.size());
    EXPECT_FALSE(waiter.waitFor(0));

    held.front()(std::string{"ok"});

    ASSERT_TRUE(waiter.waitFor(5000));
    EXPECT_EQ("response-is-ok", waiter.value());
}

TEST_F(GeneralTest, ClientTestReverse)
{
    auto request = Request{"sample.reverse"};

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ("response-is-empty", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestImageToGrayscale)
{
    // The four pixels are red, green, blue and transparent.
    // clang-format off
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        0, 0, 0, 0,
    };
    // clang-format on

    // Four pixels of four bytes are a two by two image, and declaring anything else is refused by the mapping.
    int width = 2;
    int height = 2;

    auto request = Request{
        "sample.image.grayscale",
        Param{"image", imageData},
        Param{"width", width},
        Param{"height", height},
    };

    // clang-format off
    VerifiedCall::run<std::vector<uint8_t>>(request, [](const auto &response) {
        ASSERT_TRUE(response.has_value());
        EXPECT_EQ(std::vector<uint8_t>({85, 85, 85, 255, 85, 85, 85, 255, 85, 85, 85, 255, 0, 0, 0, 0}), response.value());
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestImageToGrayscaleFromDataView)
{
    // The four pixels are red, green, blue and transparent.
    // clang-format off
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        0, 0, 0, 0,
    };
    // clang-format on

    Buffer buffer{imageData.data(), imageData.size()};
    auto dataView = buffer.view();

    auto request = Request{
        "sample.image.grayscale.dataview",
        Param{"dataView", dataView},
    };

    // clang-format off
    VerifiedCall::run<std::string>(request, [&dataView](const auto &response) {
        EXPECT_EQ("OK", response.value());

        const auto originalVector = ViewBytes::of(dataView);

        EXPECT_EQ(16, originalVector.size());
        EXPECT_EQ(originalVector[0], 85);
        EXPECT_EQ(originalVector[4], 85);
        EXPECT_EQ(originalVector[8], 85);
        EXPECT_EQ(originalVector[12], 0);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestImageToGrayscaleFromDataViewStopsAtTheLastWholePixel)
{
    // The size is what says how far the walk may go, so a size leaving a partial pixel must stop before it.

    // The first four bytes are the only whole pixel the view declares, the next three are the partial pixel it ends inside, and the last is an opaque byte a walk reading one pixel too far would act on.
    // clang-format off
    std::vector<uint8_t> storage = {
        255, 0, 0, 255,
        1, 2, 3,
        255,
    };
    // clang-format on

    Buffer buffer{storage.data(), 7};

    auto request = Request{
        "sample.image.grayscale.dataview",
        Param{"dataView", buffer.view()},
    };

    // clang-format off
    VerifiedCall::run<std::string>(request, [&storage](const auto &response) {
        EXPECT_EQ("OK", response.value());

        EXPECT_EQ(85, storage[0]);
        EXPECT_EQ(85, storage[1]);
        EXPECT_EQ(85, storage[2]);

        EXPECT_EQ(1, storage[4]);
        EXPECT_EQ(2, storage[5]);
        EXPECT_EQ(3, storage[6]);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestImageToGrayscaleFromAViewNamingNoBuffer)
{
    // A view that names no buffer has nothing to read, and the mapping answers rather than reading anything.

    auto request = Request{
        "sample.image.grayscale.dataview",
        Param{"dataView", DataView{}},
    };

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ("INVALID-DATA", response.value());
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestDataView)
{
    auto request = Request{"sample.dataview"};

    // clang-format off
    VerifiedCall::run<DataView>(request, [](const auto &response) {
        auto dataView = response.value();

        EXPECT_EQ(16, ViewBytes::of(dataView).size());
        EXPECT_EQ(ViewBytes::of(dataView)[0], 255);
        EXPECT_EQ(ViewBytes::of(dataView)[5], 255);
        EXPECT_EQ(ViewBytes::of(dataView)[10], 255);
        EXPECT_EQ(ViewBytes::of(dataView)[12], 0);

        auto request = Request{
            "sample.image.grayscale.dataview",
            Param{"dataView", dataView},
        };

        VerifiedCall::run<std::string>(request, [&dataView](const auto &response) {
            EXPECT_EQ("OK", response.value());

            const auto originalVector = ViewBytes::of(dataView);

            EXPECT_EQ(16, originalVector.size());
            EXPECT_EQ(originalVector[0], 85);
            EXPECT_EQ(originalVector[4], 85);
            EXPECT_EQ(originalVector[8], 85);
            EXPECT_EQ(originalVector[12], 0);

            EXPECT_EQ(16, ViewBytes::of(dataView).size());
            EXPECT_EQ(ViewBytes::of(dataView)[0], 85);
            EXPECT_EQ(ViewBytes::of(dataView)[5], 85);
            EXPECT_EQ(ViewBytes::of(dataView)[10], 85);
            EXPECT_EQ(ViewBytes::of(dataView)[12], 0);
        });
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestNotFound)
{
    auto request = Request{"not.found"};

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestEcho)
{
    std::string value = "paulo";
    auto request = Request{"sample.echo", Param{"value", value}};

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ("paulo", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestEchoNullOpt)
{
    std::optional<std::string> value = std::nullopt;
    auto request = Request{"sample.echo", Param{"value", value}};

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ("<EMPTY>", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestLoginFromString)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    // clang-format off
    VerifiedCall::run(request, [](auto const &response) {
        EXPECT_EQ("{\"r\":\"LOGGED-WITH-REMEMBER\"}", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestLoginWithNullDataFromString)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":null},{"n":"password","v":null},{"n":"remember","v":null}]})";

    // clang-format off
    VerifiedCall::run(request, [](auto const &response) {
        EXPECT_EQ("{\"r\":\"NOT-LOGGED\"}", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestAsyncFromString)
{
    auto request = R"({"f":"sample.async","p":[]})";
    auto waiter = std::make_shared<AnswerWaiter<std::string>>();

    // clang-format off
    Client::call(request, [waiter](auto const &response) {
        waiter->answer(response);
    });
    // clang-format on

    ASSERT_TRUE(waiter->waitFor(5000));
    EXPECT_EQ("{\"r\":null}", waiter->value());
}

TEST_F(GeneralTest, ClientTestNotFoundFromString)
{
    auto request = R"({"f":"not.found","p":[]})";

    // clang-format off
    VerifiedCall::run(request, [](auto const &response) {
        EXPECT_EQ("", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientEchoesEveryEncodingWidth)
{
    // A string has to survive every width utf8 can encode it in, on the way out and on the way back.

    const std::vector<std::string> samples = {
        "plain ascii",
        "caf\u00e9 na\u00efve",
        "\u4e2d\u6587\u30c6\u30b9\u30c8",
        "\U0001F600\U0001F468\u200D\U0001F469",
        "mixed \u00e9 \u4e2d \U0001F600 end",
    };

    for (const auto &sample : samples)
    {
        auto request = Request{"sample.echo", Param<std::string>{"value", sample}};

        // clang-format off
        VerifiedCall::run<std::string>(request, [&sample](const auto &response) {
            EXPECT_EQ(sample, response.value_or(""));
        });
        // clang-format on
    }
}

TEST_F(GeneralTest, ClientAnswersNothingForAMismatchedType)
{
    // Asking for a type the answer cannot hold is answered with nothing rather than a value read the wrong way.

    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    // clang-format off
    VerifiedCall::run<bool>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}

TEST_F(GeneralTest, MappingReadingAMismatchedTypeGetsNothing)
{
    // Reading a value as a type it does not hold answers nothing rather than a value read the wrong way.

    // clang-format off
    MappingList::shared()->add("test.mismatched.type", Map::create<std::string, int64_t>({"value"}, [](const Message &m, const Response &r) {
        const auto asString = m.get<std::string>("value");
        const auto asNumber = m.get<int64_t>("value");

        r(std::string{asString.has_value() ? "value" : "nil"} + "/" + std::to_string(asNumber.value_or(-1)));
    }));
    // clang-format on

    auto request = Request{"test.mismatched.type", Param<int64_t>{"value", 10}};

    // clang-format off
    VerifiedCall::run<std::string>(request, [](const auto &response) {
        EXPECT_EQ("nil/10", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientRefusesAnImageThatDoesNotMatchItsDeclaredSize)
{
    // The dimensions arrive from the caller, so a buffer that does not match them must not be walked by them.

    auto request = Request{
        "sample.image.grayscale",
        Param<std::vector<uint8_t>>{"image", std::vector<uint8_t>(16, 255)},
        Param<int>{"width", 1000},
        Param<int>{"height", 1000},
    };

    // clang-format off
    VerifiedCall::run<std::vector<uint8_t>>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientAnswersOnceWhenTheMappingAnswersTwice)
{
    // A call is taken out of the router by its first answer, so a mapping that answers twice resolves the caller once.

    // clang-format off
    MappingList::shared()->add("test.answers.twice", Map::create<std::string>({}, [](const Message &, const Response &r) {
        r(std::string("first"));
        r(std::string("second"));
    }));
    // clang-format on

    auto answers = 0;
    auto request = Request{"test.answers.twice"};

    // clang-format off
    VerifiedCall::run<std::string>(request, [&answers](const auto &response) {
        answers++;
        EXPECT_EQ("first", response.value_or(""));
    });
    // clang-format on

    EXPECT_EQ(1, answers);
}

TEST_F(GeneralTest, ClientKeepsOnePendingCallWhenTheMappingNeverAnswers)
{
    // A mapping that has not answered leaves its own call waiting and takes nothing else with it.

    auto responder = std::make_shared<Response>();

    // clang-format off
    MappingList::shared()->add("test.never.answers", Map::create<std::string>({}, [responder](const Message &, const Response &r) {
        *responder = r;
    }));
    // clang-format on

    const auto before = Router::shared().pending();
    auto answers = std::make_shared<int>(0);

    // clang-format off
    Client::call<std::string>(Request{"test.never.answers"}, [answers](const auto &) {
        (*answers)++;
    });
    // clang-format on

    EXPECT_EQ(0, *answers);
    EXPECT_EQ(before + 1, Router::shared().pending());

    // The test answers the call itself, so it leaves nothing waiting for the next one.
    (*responder)(std::string{"late"});

    EXPECT_EQ(1, *answers);
    EXPECT_EQ(before, Router::shared().pending());
}

TEST_F(GeneralTest, ClientCarriesALargeImageWithoutTruncating)
{
    // A buffer far past what a small integer holds has to cross without being truncated anywhere on the way.

    const size_t pixels = 1024 * 1024;
    auto image = std::vector<uint8_t>(pixels * 4, 200);

    auto request = Request{
        "sample.image.grayscale",
        Param<std::vector<uint8_t>>{"image", image},
        Param<int>{"width", 1024},
        Param<int>{"height", 1024},
    };

    // clang-format off
    VerifiedCall::run<std::vector<uint8_t>>(request, [&image](const auto &response) {
        ASSERT_TRUE(response.has_value());
        EXPECT_EQ(image.size(), response.value().size());
        EXPECT_EQ(200, response.value().front());
        EXPECT_EQ(200, response.value().back());
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientAnswersWhenAValueCannotBeEncodedOnAnotherThread)
{
    // A value the encoder refuses is reported and answered wherever the mapping produced it, and a thread of the mapping's own is the one place nothing above can catch.

    AnswerWaiter<std::string> waiter;

    // clang-format off
    MappingList::shared()->add("test.invalid.utf8.deferred", Map::create<std::string>({}, [](const Message &, const Response &r) {
        std::thread([r] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            r(std::string("\xff\xfe"));
        }).detach();
    }));
    // clang-format on

    auto request = Request{"test.invalid.utf8.deferred"};

    // clang-format off
    Client::call<std::string>(request, [&waiter](const auto &response) {
        waiter.answer(response.value_or("empty"));
    });
    // clang-format on

    ASSERT_TRUE(waiter.waitFor(5000));
    EXPECT_EQ("empty", waiter.value());
}

TEST_F(GeneralTest, ClientAnswersWhenAMappingAnswersWithTheWrongType)
{
    // The encode casts the answer against the type the mapping declared, so a mapping answering something else is a failing path like any other rather than a raise.

    AnswerWaiter<std::string> waiter;

    // clang-format off
    MappingList::shared()->add("test.wrong.answer.type", Map::create<std::string>({}, [](const Message &, const Response &r) {
        r(int64_t{42});
    }));
    // clang-format on

    auto request = Request{"test.wrong.answer.type"};

    // clang-format off
    Client::call<std::string>(request, [&waiter](const auto &response) {
        waiter.answer(response.value_or("empty"));
    });
    // clang-format on

    ASSERT_TRUE(waiter.waitFor(5000));
    EXPECT_EQ("empty", waiter.value());
}

TEST_F(GeneralTest, ClientRoutesByTheFunctionNameTheDocumentDeclares)
{
    // Routing scans for one field rather than building the document, so a parameter value carrying a field of the same name must not be the one it stops at.

    auto request = R"({"p":[{"n":"value","v":{"f":"sample.reverse"}}],"f":"sample.echo"})";

    // clang-format off
    VerifiedCall::run(request, [](auto const &response) {
        EXPECT_EQ("{\"r\":\"<EMPTY>\"}", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientReportsARequestWithNoFunctionNameOnce)
{
    // The decoder is the one that can tell an unreadable document from one carrying no name, so it reports and nothing repeats it.

    LogRecorder recorder;
    auto answered = false;

    // clang-format off
    Client::call(std::string{R"({"p":[]})"}, [&answered](const auto &response) {
        answered = true;
        EXPECT_EQ(response, "");
    });
    // clang-format on

    EXPECT_TRUE(answered);
    EXPECT_EQ(recorder.errors().size(), 1u);
}

TEST_F(GeneralTest, ClientReportsARequestThatCannotBeParsedOnce)
{
    LogRecorder recorder;
    auto answered = false;

    // clang-format off
    Client::call(std::string{"{"}, [&answered](const auto &response) {
        answered = true;
        EXPECT_EQ(response, "");
    });
    // clang-format on

    EXPECT_TRUE(answered);
    EXPECT_EQ(recorder.errors().size(), 1u);
}
