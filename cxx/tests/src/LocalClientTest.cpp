#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <thread>

using namespace xplpc::core;
using namespace xplpc::client;
using namespace xplpc::message;

TEST_F(GeneralTest, LocalClientTestLogin)
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    // clang-format off
    LocalClient::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ("LOGGED-WITH-REMEMBER", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestLoginAsync)
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    // clang-format off
    std::thread([=] {
        LocalClient::call<std::string>(request, [](const auto &response) {
            EXPECT_EQ("LOGGED-WITH-REMEMBER", response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestAsync)
{
    auto request = Request{"sample.async"};

    // clang-format off
    LocalClient::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestAsyncWithThread)
{
    auto request = Request{"sample.async"};

    // clang-format off
    std::thread([=] {
        LocalClient::call<std::string>(request, [](const auto &response) {
            EXPECT_EQ(std::nullopt, response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestReverse)
{
    auto request = Request{"sample.reverse"};

    // clang-format off
    LocalClient::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ("response-is-empty", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestReverseAsyncWithThread)
{
    auto request = Request{"sample.reverse"};

    // clang-format off
    std::thread([=] {
        LocalClient::call<std::string>(request, [](const auto &response) {
            EXPECT_EQ("response-is-empty", response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestImageToGrayscale)
{
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255, // red pixel
        0, 255, 0, 255, // green pixel
        0, 0, 255, 255, // blue pixel
        0, 0, 0, 0,     // transparent pixel
    };

    int width = 1;
    int height = 1;

    auto request = Request{
        "sample.image.grayscale",
        Param{"image", imageData},
        Param{"width", width},
        Param{"height", height},
    };

    // clang-format off
    LocalClient::call<std::vector<uint8_t>>(request, [](const auto &response) {
        EXPECT_EQ(16, response.value().size());
    });
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestImageToGrayscaleAsyncWithThread)
{
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255, // red pixel
        0, 255, 0, 255, // green pixel
        0, 0, 255, 255, // blue pixel
        0, 0, 0, 0,     // transparent pixel
    };

    int width = 1;
    int height = 1;

    auto request = Request{
        "sample.image.grayscale",
        Param{"image", imageData},
        Param{"width", width},
        Param{"height", height},
    };

    // clang-format off
    std::thread([=] {
        LocalClient::call<std::vector<uint8_t>>(request, [](const auto &response) {
            EXPECT_EQ(16, response.value().size());
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestImageToGrayscaleFromPointer)
{
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255, // red pixel
        0, 255, 0, 255, // green pixel
        0, 0, 255, 255, // blue pixel
        0, 0, 0, 0,     // transparent pixel
    };

    int width = 1;
    int height = 1;

    uint8_t *pointer = imageData.data();
    int64_t pointerAddress = reinterpret_cast<int64_t>(pointer);
    size_t pointerSize = imageData.size();

    auto request = Request{
        "sample.image.grayscale.pointer",
        Param{"pointer", pointerAddress},
        Param{"width", width},
        Param{"height", height},
    };

    // clang-format off
    LocalClient::call<std::string>(request, [pointerAddress, pointerSize](const auto &response) {
        EXPECT_EQ("OK", response.value());

        std::vector<uint8_t> originalVector(pointerSize);
        memcpy(originalVector.data(), reinterpret_cast<uint8_t*>(pointerAddress), pointerSize);

        EXPECT_EQ(16, originalVector.size());
    });
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestImageToGrayscaleFromPointerAsyncWithThread)
{
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255, // red pixel
        0, 255, 0, 255, // green pixel
        0, 0, 255, 255, // blue pixel
        0, 0, 0, 0,     // transparent pixel
    };

    int width = 1;
    int height = 1;

    uint8_t *pointer = imageData.data();
    int64_t pointerAddress = reinterpret_cast<int64_t>(pointer);
    size_t pointerSize = imageData.size();

    auto request = Request{
        "sample.image.grayscale.pointer",
        Param{"pointer", pointerAddress},
        Param{"width", width},
        Param{"height", height},
    };

    // clang-format off
    std::thread([=] {
        LocalClient::call<std::string>(request, [pointerAddress, pointerSize](const auto &response) {
            EXPECT_EQ("OK", response.value());

            std::vector<uint8_t> originalVector(pointerSize);
            memcpy(originalVector.data(), reinterpret_cast<uint8_t*>(pointerAddress), pointerSize);

            EXPECT_EQ(16, originalVector.size());
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, LocalClientTestNotFound)
{
    auto request = Request{"not.found"};

    // clang-format off
    LocalClient::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}
