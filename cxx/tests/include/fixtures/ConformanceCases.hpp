#pragma once

#include "nlohmann/json.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Lists the cases of one group of the shared conformance suite, which every bridge reads from the same files.
class ConformanceCases
{
public:
    static std::vector<std::string> in(const std::string &group)
    {
        std::vector<std::string> names;

        for (const auto &entry : std::filesystem::directory_iterator(std::filesystem::path(XPLPC_CONFORMANCE_DIR) / group))
        {
            if (entry.path().extension() == ".json")
            {
                names.push_back(entry.path().stem().string());
            }
        }

        std::sort(names.begin(), names.end());

        return names;
    }

    static nlohmann::json read(const std::string &group, const std::string &name)
    {
        std::ifstream file(std::filesystem::path(XPLPC_CONFORMANCE_DIR) / group / (name + ".json"));
        return nlohmann::json::parse(file);
    }

    // A test name may only carry letters, digits and underscores.
    static std::string testName(const std::string &name)
    {
        auto result = name;
        std::replace(result.begin(), result.end(), '-', '_');

        return result;
    }
};
