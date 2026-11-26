#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "nlohmann/json.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace xplpc
{
namespace serializer
{

// Reads the function name of a request without building the document, and refuses a request every bridge would not route the same way.
class JsonRequestScanner : public nlohmann::json_sax<nlohmann::json>
{
public:
    std::string name;
    bool unreadable = false;
    const char *refusal = nullptr;
    std::string repeatedParameter;

    bool key(string_t &value) override
    {
        auto &frame = frames.back();

        // A key written twice reads as the first value on one bridge and as the last on another, so it is refused in whatever object it sits.
        if (!frame.keys.insert(value).second)
        {
            return refuse("a key appears more than once in an object");
        }

        const auto role = frame.role;

        if (role == Role::Root && value == "f")
        {
            pending = Slot::FunctionName;
            return true;
        }

        if (role == Role::Root && value == "p")
        {
            pending = Slot::Parameters;
            return true;
        }

        if (role == Role::Parameter && value == "n")
        {
            pending = Slot::ParameterName;
            return true;
        }

        pending = Slot::None;
        return true;
    }

    bool string(string_t &value) override
    {
        if (frames.empty())
        {
            return refuse("the document is not an object");
        }

        const auto slot = take();

        if (slot == Slot::FunctionName)
        {
            name = value;
            return true;
        }

        if (slot == Slot::ParameterName)
        {
            return readParameterName(value);
        }

        if (slot == Slot::Parameters)
        {
            return refuse("the parameters are not an array");
        }

        return valueOutsideASlot();
    }

    bool start_object(std::size_t) override
    {
        if (frames.empty())
        {
            frames.push_back({Role::Root});
            return true;
        }

        const auto slot = take();

        if (slot != Slot::None)
        {
            return refuseSlot(slot);
        }

        const auto role = frames.back().role == Role::Parameters ? Role::Parameter : Role::Other;
        frames.push_back({role});

        return true;
    }

    bool end_object() override
    {
        // A parameter with no name cannot be read by any bridge, so it is refused here rather than routed to one that would.
        if (frames.back().role == Role::Parameter && !frames.back().named)
        {
            return refuse("a parameter carries no name");
        }

        frames.pop_back();
        return true;
    }

    bool start_array(std::size_t) override
    {
        if (frames.empty())
        {
            return refuse("the document is not an object");
        }

        const auto slot = take();

        if (slot == Slot::Parameters)
        {
            frames.push_back({Role::Parameters});
            return true;
        }

        if (slot != Slot::None)
        {
            return refuseSlot(slot);
        }

        if (!valueOutsideASlot())
        {
            return false;
        }

        frames.push_back({Role::Other});

        return true;
    }

    bool end_array() override
    {
        frames.pop_back();
        return true;
    }

    bool null() override
    {
        // The format reads a null parameter list as no parameters, so only a null name is refused.
        const auto slot = take();

        if (slot == Slot::Parameters)
        {
            return true;
        }

        return scalar(slot);
    }

    bool boolean(bool) override { return scalar(take()); }
    bool number_integer(number_integer_t) override { return scalar(take()); }
    bool number_unsigned(number_unsigned_t) override { return scalar(take()); }
    bool number_float(number_float_t, const string_t &) override { return scalar(take()); }
    bool binary(binary_t &) override { return scalar(take()); }

    bool parse_error(std::size_t, const std::string &, const nlohmann::detail::exception &) override
    {
        if (!refusal)
        {
            unreadable = true;
        }

        return false;
    }

private:
    enum class Role
    {
        Root,
        Parameters,
        Parameter,
        Other,
    };

    enum class Slot
    {
        None,
        FunctionName,
        Parameters,
        ParameterName,
    };

    struct Frame
    {
        Role role;
        bool named = false;
        std::unordered_set<std::string> keys;
    };

    std::vector<Frame> frames;
    std::unordered_set<std::string> parameterNames;
    Slot pending = Slot::None;

    Slot take()
    {
        const auto slot = pending;
        pending = Slot::None;
        return slot;
    }

    bool scalar(Slot slot)
    {
        if (frames.empty())
        {
            return refuse("the document is not an object");
        }

        if (slot == Slot::None)
        {
            return valueOutsideASlot();
        }

        return refuseSlot(slot);
    }

    // Every element of the parameter list is an object, so a value standing there alone is a parameter no bridge can read.
    bool valueOutsideASlot()
    {
        if (frames.back().role == Role::Parameters)
        {
            return refuse("a parameter is not an object");
        }

        return true;
    }

    bool refuseSlot(Slot slot)
    {
        if (slot == Slot::FunctionName)
        {
            return refuse("the function name is not a string");
        }

        if (slot == Slot::Parameters)
        {
            return refuse("the parameters are not an array");
        }

        return refuse("a parameter name is not a string");
    }

    bool readParameterName(const std::string &value)
    {
        frames.back().named = true;

        if (!parameterNames.insert(value).second)
        {
            repeatedParameter = value;
            return refuse("a parameter name appears more than once");
        }

        return true;
    }

    bool refuse(const char *reason)
    {
        refusal = reason;
        return false;
    }
};

} // namespace serializer
} // namespace xplpc

#endif
