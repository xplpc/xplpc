#pragma once

#ifdef XPLPC_SERIALIZER_JSON

#include "xplpc/type/DataView.hpp"

#include "nlohmann/json.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <forward_list>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <ratio>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace xplpc
{
namespace serializer
{

// Says whether a value can be read as the declared type without changing it, since nlohmann reads a boolean as a number, truncates a fraction and wraps an integer that does not fit.
class JsonValueGuard
{
public:
    template <typename T>
    static bool fits(const nlohmann::json &j)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            return j.is_boolean();
        }
        else if constexpr (std::is_same_v<T, char>)
        {
            // A char is signed on some abis and unsigned on others, so only the code points every one of them holds are read.
            return fitsIntegral<std::int8_t>(j) && j.get<std::int64_t>() >= 0;
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return fitsIntegral<T>(j);
        }
        else if constexpr (std::is_enum_v<T>)
        {
            // An enumeration nlohmann reads through its underlying integer is held to that integer, and one a consumer maps to text decides that text itself.
            return j.is_string() || fitsIntegral<std::underlying_type_t<T>>(j);
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            return fitsFloating<T>(j);
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            return j.is_string();
        }
        else if constexpr (std::is_same_v<T, xplpc::type::DataView>)
        {
            // Anything that is not a handle names no buffer, which reads as the empty view wherever it sits.
            return true;
        }
        else if constexpr (IsOptional<T>::value)
        {
            return j.is_null() || fits<typename T::value_type>(j);
        }
        else if constexpr (IsTimePoint<T>::value)
        {
            return fitsIntegral<std::int64_t>(j) && holdsMilliseconds<typename T::duration>(j.get<std::int64_t>());
        }
        else if constexpr (IsSequence<T>::value)
        {
            return fitsEach<typename T::value_type>(j);
        }
        else if constexpr (IsFixedArray<T>::value)
        {
            // An array of a fixed size reads only as many values as it holds, so a list of another length is not the value that was sent.
            return j.is_array() && j.size() == std::tuple_size_v<T> && fitsEach<typename T::value_type>(j);
        }
        else if constexpr (IsTuple<T>::value)
        {
            return j.is_array() && j.size() == std::tuple_size_v<T> && fitsElements<T>(j, std::make_index_sequence<std::tuple_size_v<T>>());
        }
        else if constexpr (IsMap<T>::value)
        {
            return fitsMap<typename T::key_type, typename T::mapped_type>(j);
        }
        else
        {
            // A type this library did not write is filled by its own from_json, which decides what it accepts.
            return true;
        }
    }

    template <typename T>
    static bool fitsIntegral(const nlohmann::json &j)
    {
        if (j.is_number_unsigned())
        {
            return j.get<std::uint64_t>() <= static_cast<std::uint64_t>((std::numeric_limits<T>::max)());
        }

        if (!j.is_number_integer())
        {
            return false;
        }

        const auto value = j.get<std::int64_t>();

        if (value >= 0)
        {
            return static_cast<std::uint64_t>(value) <= static_cast<std::uint64_t>((std::numeric_limits<T>::max)());
        }

        if constexpr (std::is_signed_v<T>)
        {
            return value >= static_cast<std::int64_t>((std::numeric_limits<T>::min)());
        }

        return false;
    }

    // Says whether a count of milliseconds can become the declared duration, since a clock counting nanoseconds overflows past the year 2262.
    template <typename Duration>
    static bool holdsMilliseconds(std::int64_t milliseconds)
    {
        using Rep = typename Duration::rep;
        using Ratio = std::ratio_divide<std::milli, typename Duration::period>;

        if constexpr (std::is_floating_point_v<Rep>)
        {
            return true;
        }
        else
        {
            constexpr auto widest = (std::numeric_limits<std::intmax_t>::max)();

            if (milliseconds > widest / Ratio::num || milliseconds < -(widest / Ratio::num))
            {
                return false;
            }

            const auto count = static_cast<std::intmax_t>(milliseconds) * Ratio::num / Ratio::den;

            return count >= static_cast<std::intmax_t>((std::numeric_limits<Rep>::min)()) && count <= static_cast<std::intmax_t>((std::numeric_limits<Rep>::max)());
        }
    }

private:
    template <typename T>
    static bool fitsFloating(const nlohmann::json &j)
    {
        // A fraction is rounded to the nearest value the type holds, while an integer has to arrive exactly and a magnitude past the type is refused rather than read as infinity.

        if (j.is_number_float())
        {
            const auto value = j.get<double>();

            if constexpr ((std::numeric_limits<T>::max)() >= (std::numeric_limits<double>::max)())
            {
                return std::isfinite(value);
            }
            else
            {
                return std::isfinite(value) && std::fabs(value) <= static_cast<double>((std::numeric_limits<T>::max)());
            }
        }

        if (j.is_number_unsigned())
        {
            return holdsExactly<T>(j.get<std::uint64_t>());
        }

        if (j.is_number_integer())
        {
            return holdsExactly<T>(j.get<std::int64_t>());
        }

        return false;
    }

    template <typename T, typename I>
    static bool holdsExactly(I value)
    {
        // The bound is a power of two the floating type holds exactly, so the cast back is only taken for a value the integer type can carry.

        const auto converted = static_cast<T>(value);
        const auto bound = std::ldexp(T(1), std::numeric_limits<I>::digits);

        if (converted >= bound || converted < -bound)
        {
            return false;
        }

        return static_cast<I>(converted) == value;
    }

    template <typename T>
    struct IsOptional : std::false_type
    {
    };

    template <typename T>
    struct IsOptional<std::optional<T>> : std::true_type
    {
    };

    template <typename T>
    struct IsTimePoint : std::false_type
    {
    };

    template <typename Clock, typename Duration>
    struct IsTimePoint<std::chrono::time_point<Clock, Duration>> : std::true_type
    {
    };

    template <typename T>
    struct IsSequence : std::false_type
    {
    };

    template <typename T, typename Allocator>
    struct IsSequence<std::vector<T, Allocator>> : std::true_type
    {
    };

    template <typename T, typename Allocator>
    struct IsSequence<std::list<T, Allocator>> : std::true_type
    {
    };

    template <typename T, typename Allocator>
    struct IsSequence<std::deque<T, Allocator>> : std::true_type
    {
    };

    template <typename T, typename Allocator>
    struct IsSequence<std::forward_list<T, Allocator>> : std::true_type
    {
    };

    template <typename T, typename Compare, typename Allocator>
    struct IsSequence<std::set<T, Compare, Allocator>> : std::true_type
    {
    };

    template <typename T, typename Compare, typename Allocator>
    struct IsSequence<std::multiset<T, Compare, Allocator>> : std::true_type
    {
    };

    template <typename T, typename Hash, typename Equal, typename Allocator>
    struct IsSequence<std::unordered_set<T, Hash, Equal, Allocator>> : std::true_type
    {
    };

    template <typename T, typename Hash, typename Equal, typename Allocator>
    struct IsSequence<std::unordered_multiset<T, Hash, Equal, Allocator>> : std::true_type
    {
    };

    template <typename T>
    struct IsFixedArray : std::false_type
    {
    };

    template <typename T, std::size_t Size>
    struct IsFixedArray<std::array<T, Size>> : std::true_type
    {
    };

    template <typename T>
    struct IsTuple : std::false_type
    {
    };

    template <typename First, typename Second>
    struct IsTuple<std::pair<First, Second>> : std::true_type
    {
    };

    template <typename... Types>
    struct IsTuple<std::tuple<Types...>> : std::true_type
    {
    };

    template <typename T>
    struct IsMap : std::false_type
    {
    };

    template <typename Key, typename T, typename Hash, typename Equal, typename Allocator>
    struct IsMap<std::unordered_map<Key, T, Hash, Equal, Allocator>> : std::true_type
    {
    };

    template <typename Key, typename T, typename Compare, typename Allocator>
    struct IsMap<std::map<Key, T, Compare, Allocator>> : std::true_type
    {
    };

    // A map whose key is not text travels as a list of key and value pairs.
    template <typename Key, typename T>
    static bool fitsMap(const nlohmann::json &j)
    {
        if constexpr (std::is_same_v<Key, std::string>)
        {
            return fitsEachMember<T>(j);
        }
        else
        {
            return fitsEach<std::pair<Key, T>>(j);
        }
    }

    template <typename T, std::size_t... Index>
    static bool fitsElements(const nlohmann::json &j, std::index_sequence<Index...>)
    {
        return (fits<std::tuple_element_t<Index, T>>(j[Index]) && ...);
    }

    template <typename T>
    static bool fitsEach(const nlohmann::json &j)
    {
        if (!j.is_array())
        {
            return false;
        }

        for (const auto &item : j)
        {
            if (!fits<T>(item))
            {
                return false;
            }
        }

        return true;
    }

    template <typename T>
    static bool fitsEachMember(const nlohmann::json &j)
    {
        if (!j.is_object())
        {
            return false;
        }

        for (const auto &item : j.items())
        {
            if (!fits<T>(item.value()))
            {
                return false;
            }
        }

        return true;
    }
};

} // namespace serializer
} // namespace xplpc

#endif
