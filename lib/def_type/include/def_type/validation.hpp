#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <def_type/field.hpp>
#include <def_type/unknown.hpp>

namespace def_type {

// ── validation_error — shared across all paths ──────────────────────────
//
// Validator authors fill: message, code (optional), sub_path (optional),
//                         data (optional, any user struct via `unknown`).
// Framework fills:        path (field name joined with sub_path),
//                         validator (validator struct name via nameof).
// Anything a validator writes into .path or .validator is overwritten.

struct validation_error {
    std::string                message;
    std::optional<std::string> code;
    std::optional<std::string> sub_path;
    std::optional<unknown>     data;

    std::string path;
    std::string validator;
};

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
