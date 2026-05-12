#pragma once

#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

#include <def_type/field.hpp>
#include <def_type/validation_error.hpp>

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
        std::vector<validation_error> operator()(const std::string& value) const {
            if (!value.empty()) return {};
            return { { .message = "must not be empty" } };
        }
    };

    struct max_length {
        std::size_t limit;
        std::vector<validation_error> operator()(const std::string& value) const {
            if (value.size() <= limit) return {};
            return { { .message = "length " + std::to_string(value.size()) +
                                  " exceeds maximum of " + std::to_string(limit) } };
        }
    };

    struct in_range {
        int min = 0;
        int max = 0;
        std::vector<validation_error> operator()(int value) const {
            if (value >= min && value <= max) return {};
            return { { .message = std::to_string(value) + " must be between " +
                                  std::to_string(min) + " and " + std::to_string(max) } };
        }
    };

}  // namespace validations

}  // namespace def_type
