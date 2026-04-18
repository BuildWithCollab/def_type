#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>

#include <def_type/field.hpp>

namespace def_type {

namespace detail {
    template <typename T>
    inline constexpr bool is_validator_pack_v = false;

    template <typename... Vs>
    inline constexpr bool is_validator_pack_v<validator_pack<Vs...>> = true;

    template <typename T>
    inline constexpr bool is_with_v = false;

    template <typename... Exts>
    inline constexpr bool is_with_v<with<Exts...>> = true;

}

namespace validations {

    struct not_empty {
        std::optional<std::string> operator()(const std::string& value) const {
            if (!value.empty()) return std::nullopt;
            return "must not be empty";
        }
    };

    struct max_length {
        std::size_t limit;
        std::optional<std::string> operator()(const std::string& value) const {
            if (value.size() <= limit) return std::nullopt;
            return "length " + std::to_string(value.size()) +
                   " exceeds maximum of " + std::to_string(limit);
        }
    };

    struct positive {
        template <typename T>
        std::optional<std::string> operator()(const T& value) const {
            if (value > 0) return std::nullopt;
            return std::to_string(value) + " must be positive";
        }
    };

}  // namespace validations

}  // namespace def_type
