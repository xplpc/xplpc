#include "fixtures/GeneralTest.hpp"
#include "xplpc/xplpc.hpp"
#include "gtest/gtest.h"

#include <algorithm>
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
    Client::call<std::string>(request, [](const auto &response) {
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

    auto future = Client::callAsync<std::string>(request);
    auto response = future.get();
    EXPECT_EQ("LOGGED-WITH-REMEMBER", response);
}

TEST_F(GeneralTest, ClientTestLoginAsyncWithThread)
{
    auto request = Request{
        "sample.login",
        Param<std::string>{"username", "paulo"},
        Param<std::string>{"password", "123456"},
        Param<bool>{"remember", true},
    };

    // clang-format off
    std::thread([=] {
        Client::call<std::string>(request, [](const auto &response) {
            EXPECT_EQ("LOGGED-WITH-REMEMBER", response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, ClientTestAsync)
{
    auto request = Request{"sample.async"};

    // clang-format off
    Client::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestAsyncWithThread)
{
    auto request = Request{"sample.async"};

    // clang-format off
    std::thread([=] {
        Client::call<std::string>(request, [](const auto &response) {
            EXPECT_EQ(std::nullopt, response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, ClientTestReverse)
{
    auto request = Request{"sample.reverse"};

    // clang-format off
    Client::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ("response-is-empty", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestReverseAsyncWithThread)
{
    auto request = Request{"sample.reverse"};

    // clang-format off
    std::thread([=] {
        Client::call<std::string>(request, [](const auto &response) {
            EXPECT_EQ("response-is-empty", response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, ClientTestImageToGrayscale)
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
    Client::call<std::vector<uint8_t>>(request, [](const auto &response) {
        EXPECT_EQ(16, response.value().size());
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestImageToGrayscaleAsyncWithThread)
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
        Client::call<std::vector<uint8_t>>(request, [](const auto &response) {
            EXPECT_EQ(16, response.value().size());
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, ClientTestImageToGrayscaleFromDataView)
{
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255, // red pixel
        0, 255, 0, 255, // green pixel
        0, 0, 255, 255, // blue pixel
        0, 0, 0, 0,     // transparent pixel
    };

    auto dataView = DataView{imageData.data(), imageData.size()};

    auto request = Request{
        "sample.image.grayscale.dataview",
        Param{"dataView", dataView},
    };

    // clang-format off
    Client::call<std::string>(request, [&dataView](const auto &response) {
        EXPECT_EQ("OK", response.value());

        std::vector<uint8_t> originalVector(dataView.size());

        dataView.copy(originalVector.data());

        EXPECT_EQ(16, originalVector.size());
        EXPECT_EQ(originalVector[0], 85);
        EXPECT_EQ(originalVector[4], 85);
        EXPECT_EQ(originalVector[8], 85);
        EXPECT_EQ(originalVector[12], 0);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestImageToGrayscaleFromDataViewAsyncWithThread)
{
    std::vector<uint8_t> imageData = {
        255, 0, 0, 255, // red pixel
        0, 255, 0, 255, // green pixel
        0, 0, 255, 255, // blue pixel
        0, 0, 0, 0,     // transparent pixel
    };

    auto dataView = DataView{imageData.data(), imageData.size()};

    auto request = Request{
        "sample.image.grayscale.dataview",
        Param{"dataView", dataView},
    };

    // clang-format off
    std::thread([=] {
        Client::call<std::string>(request, [&dataView](const auto &response) {
            EXPECT_EQ("OK", response.value());

            std::vector<uint8_t> originalVector(dataView.size());

            dataView.copy(originalVector.data());

            EXPECT_EQ(16, originalVector.size());
            EXPECT_EQ(originalVector[0], 85);
            EXPECT_EQ(originalVector[4], 85);
            EXPECT_EQ(originalVector[8], 85);
            EXPECT_EQ(originalVector[12], 0);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, ClientTestDataView)
{
    auto request = Request{"sample.dataview"};

    // clang-format off
    Client::call<DataView>(request, [](const auto &response) {
        auto dataView = response.value();

        // check current values
        EXPECT_EQ(16, dataView.size());
        EXPECT_EQ(dataView.ptr()[0], 255);
        EXPECT_EQ(dataView.ptr()[5], 255);
        EXPECT_EQ(dataView.ptr()[10], 255);
        EXPECT_EQ(dataView.ptr()[12], 0);

        auto request = Request{
            "sample.image.grayscale.dataview",
            Param{"dataView", dataView},
        };

        // send original data and check modified data
        Client::call<std::string>(request, [&dataView](const auto &response) {
            EXPECT_EQ("OK", response.value());

            std::vector<uint8_t> originalVector(dataView.size());
            dataView.copy(originalVector.data());

            // check copied values
            EXPECT_EQ(16, originalVector.size());
            EXPECT_EQ(originalVector[0], 85);
            EXPECT_EQ(originalVector[4], 85);
            EXPECT_EQ(originalVector[8], 85);
            EXPECT_EQ(originalVector[12], 0);

            // current original values again
            EXPECT_EQ(16, dataView.size());
            EXPECT_EQ(dataView.ptr()[0], 85);
            EXPECT_EQ(dataView.ptr()[5], 85);
            EXPECT_EQ(dataView.ptr()[10], 85);
            EXPECT_EQ(dataView.ptr()[12], 0);
        });
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestDataViewAsyncWithThread)
{
    auto request = Request{"sample.dataview"};

    // clang-format off
    std::thread([=] {
        Client::call<DataView>(request, [](const auto &response) {
            auto dataView = response.value();

            // check current values
            EXPECT_EQ(16, dataView.size());
            EXPECT_EQ(dataView.ptr()[0], 255);
            EXPECT_EQ(dataView.ptr()[5], 255);
            EXPECT_EQ(dataView.ptr()[10], 255);
            EXPECT_EQ(dataView.ptr()[12], 0);

            auto request = Request{
                "sample.image.grayscale.dataview",
                Param{"dataView", dataView},
            };

            Client::call<std::string>(request, [&dataView](const auto &response) {
                EXPECT_EQ("OK", response.value());

                std::vector<uint8_t> originalVector(dataView.size());
                dataView.copy(originalVector.data());

                // check copied values
                EXPECT_EQ(16, originalVector.size());
                EXPECT_EQ(originalVector[0], 85);
                EXPECT_EQ(originalVector[4], 85);
                EXPECT_EQ(originalVector[8], 85);
                EXPECT_EQ(originalVector[12], 0);

                // current original values again
                EXPECT_EQ(16, dataView.size());
                EXPECT_EQ(dataView.ptr()[0], 85);
                EXPECT_EQ(dataView.ptr()[5], 85);
                EXPECT_EQ(dataView.ptr()[10], 85);
                EXPECT_EQ(dataView.ptr()[12], 0);
            });
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, ClientTestNotFound)
{
    auto request = Request{"not.found"};

    // clang-format off
    Client::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ(std::nullopt, response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestEcho)
{
    std::string value = "paulo";
    auto request = Request{"sample.echo", Param{"value", value}};

    // clang-format off
    Client::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ("paulo", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestEchoNullOpt)
{
    std::optional<std::string> value = std::nullopt;
    auto request = Request{"sample.echo", Param{"value", value}};

    // clang-format off
    Client::call<std::string>(request, [](const auto &response) {
        EXPECT_EQ("<EMPTY>", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestLoginFromString)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    // clang-format off
    Client::call(request, [](auto const &response) {
        EXPECT_EQ("{\"r\":\"LOGGED-WITH-REMEMBER\"}", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestLoginAsyncFromStringWithThread)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":"paulo"},{"n":"password","v":"123456"},{"n":"remember","v":true}]})";

    // clang-format off
    std::thread([=] {
        Client::call(request, [](auto const &response) {
            EXPECT_EQ("{\"r\":\"LOGGED-WITH-REMEMBER\"}", response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, ClientTestLoginWithNullDataFromString)
{
    auto request = R"({"f":"sample.login","p":[{"n":"username","v":null},{"n":"password","v":null},{"n":"remember","v":null}]})";

    // clang-format off
    Client::call(request, [](auto const &response) {
        EXPECT_EQ("{\"r\":\"NOT-LOGGED\"}", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestAsyncFromString)
{
    auto request = R"({"f":"sample.async","p":[]})";

    // clang-format off
    Client::call(request, [](auto const &response) {
        EXPECT_EQ("{\"r\":null}", response);
    });
    // clang-format on
}

TEST_F(GeneralTest, ClientTestAsyncFromStringWithThread)
{
    auto request = R"({"f":"sample.async","p":[]})";

    // clang-format off
    std::thread([=] {
        Client::call(request, [](const auto &response) {
            EXPECT_EQ("{\"r\":null}", response);
        });
    }).join();
    // clang-format on
}

TEST_F(GeneralTest, ClientTestNotFoundFromString)
{
    auto request = R"({"f":"not.found","p":[]})";

    // clang-format off
    Client::call(request, [](auto const &response) {
        EXPECT_EQ("", response);
    });
    // clang-format on
}
