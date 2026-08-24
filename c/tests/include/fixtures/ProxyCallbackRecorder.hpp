#pragma once

#include "xplpc/c/platform.h"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>

// What the native layer hands back to the host is captured here, so the tests can assert on the exact bytes and sizes.
class ProxyCallbackRecorder
{
public:
    static void reset()
    {
        std::unique_lock<std::mutex> lock(mutex);

        key.clear();
        data.clear();
        callKey.clear();
        callData.clear();
        callKeyFromThread.clear();
        callDataFromThread.clear();
        keyFromThread.clear();
        dataFromThread.clear();
        answersFromThread = 0;
    }

    static void onNativeProxyCallback(const char *keyValue, size_t keySize, const char *dataValue, size_t dataSize)
    {
        // A deferred mapping answers from its own thread, so the fields are published under the lock the waiters read them through.

        {
            std::unique_lock<std::mutex> lock(mutex);

            key = std::string(keyValue, keySize);
            data = std::string(dataValue, dataSize);
        }

        condition.notify_all();
    }

    static void onNativeProxyCall(const char *keyValue, size_t keySize, const char *dataValue, size_t dataSize)
    {
        callKey = std::string(keyValue, keySize);
        callData = std::string(dataValue, dataSize);

        const std::string response = R"({"r":"from-host"})";
        xplpc_native_call_proxy_callback(callKey.c_str(), callKey.size(), response.c_str(), response.size());
    }

    static void onNativeProxyCallDeferred(const char *keyValue, size_t keySize, const char *dataValue, size_t dataSize)
    {
        // The host takes the call and answers from a thread of its own, which is what a network request looks like here.

        callKey = std::string(keyValue, keySize);
        callData = std::string(dataValue, dataSize);

        // clang-format off
        std::thread([key = callKey] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            const std::string response = R"({"r":"from-host-later"})";
            xplpc_native_call_proxy_callback(key.c_str(), key.size(), response.c_str(), response.size());
        }).detach();
        // clang-format on
    }

    static void onNativeProxyCallFromThread(char *keyValue, size_t keySize, char *dataValue, size_t dataSize)
    {
        // The core reached the host from a thread the host did not start, so these buffers belong to this side.

        {
            std::unique_lock<std::mutex> lock(mutex);

            callKeyFromThread = std::string(keyValue, keySize);
            callDataFromThread = std::string(dataValue, dataSize);
        }

        const std::string response = R"({"r":"from-host-off-thread"})";
        xplpc_native_call_proxy_callback(callKeyFromThread.c_str(), callKeyFromThread.size(), response.c_str(), response.size());

        xplpc_free(keyValue);
        xplpc_free(dataValue);
    }

    static void onNativeProxyCallbackFromThread(char *keyValue, size_t keySize, char *dataValue, size_t dataSize)
    {
        // The buffers belong to this side once they arrive, so they are released after being read.

        {
            std::unique_lock<std::mutex> lock(mutex);

            keyFromThread = std::string(keyValue, keySize);
            dataFromThread = std::string(dataValue, dataSize);
            answersFromThread++;
        }

        xplpc_free(keyValue);
        xplpc_free(dataValue);

        condition.notify_all();
    }

    static bool waitForAnswer(int milliseconds)
    {
        std::unique_lock<std::mutex> lock(mutex);

        // clang-format off
        return condition.wait_for(lock, std::chrono::milliseconds(milliseconds), [] {
            return !data.empty();
        });
        // clang-format on
    }

    static std::string takeKey()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return key;
    }

    static std::string takeData()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return data;
    }

    static bool waitForAnswerFromThread(int milliseconds)
    {
        // The answer is produced by a thread that outlives the call, so the test waits for it instead of reading a race.

        std::unique_lock<std::mutex> lock(mutex);

        // The answer is waited on through the data, since a host is free to have called with an empty key and the answer still has to arrive.

        // clang-format off
        return condition.wait_for(lock, std::chrono::milliseconds(milliseconds), [] {
            return !dataFromThread.empty();
        });
        // clang-format on
    }

    static bool waitForAnswerCountFromThread(int milliseconds, size_t expected)
    {
        // An answer of the empty value carries nothing in either buffer, so arrival is counted rather than read.

        std::unique_lock<std::mutex> lock(mutex);

        // clang-format off
        return condition.wait_for(lock, std::chrono::milliseconds(milliseconds), [expected] {
            return answersFromThread >= expected;
        });
        // clang-format on
    }

    static std::string takeKeyFromThread()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return keyFromThread;
    }

    static std::string takeDataFromThread()
    {
        std::unique_lock<std::mutex> lock(mutex);
        return dataFromThread;
    }

    static std::string key;
    static std::string data;
    static std::string callKey;
    static std::string callData;
    static std::string callKeyFromThread;
    static std::string callDataFromThread;
    static std::string keyFromThread;
    static std::string dataFromThread;
    static size_t answersFromThread;
    static std::mutex mutex;
    static std::condition_variable condition;
};
