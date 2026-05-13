#pragma once

#include <concepts>
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>

#include <fmt/format.h>

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

    // Fails when `.empty()` returns true. Works on anything with `.empty()`:
    // std::string, std::vector<T>, std::map, std::set, std::unordered_set, etc.

    struct not_empty {
        template <typename T>
            requires requires(const T& v) {
                { v.empty() } -> std::convertible_to<bool>;
            }
        std::vector<validation_error> operator()(const T& value) const {
            if (!value.empty()) return {};
            return { { .message = "must not be empty" } };
        }
    };

    // Fails when `.size()` exceeds `limit`. Works on anything with `.size()`:
    // std::string, all standard containers, your own.

    struct max_length {
        std::size_t limit{};
        template <typename T>
            requires requires(const T& v) {
                { v.size() } -> std::convertible_to<std::size_t>;
            }
        std::vector<validation_error> operator()(const T& value) const {
            if (value.size() <= limit) return {};
            return { { .message = fmt::format(
                "length {} exceeds maximum of {}", value.size(), limit) } };
        }
    };

    // Fails when value < min or value > max. Generic over any orderable T.
    // CTAD on positional init: `in_range{0.0, 1.0}` deduces `in_range<double>`.
    // Designated init defaults to `in_range<int>` — explicit when needed:
    //   in_range{.min = 1, .max = 100}            // T = int (default)
    //   in_range<double>{.min = 0.0, .max = 1.0}  // T = double explicit

    template <typename T = int>
    struct in_range {
        T min{};
        T max{};

        template <typename V>
            requires requires(const V& v, const T& b) {
                { v < b } -> std::convertible_to<bool>;
                { v > b } -> std::convertible_to<bool>;
            }
        std::vector<validation_error> operator()(const V& value) const {
            if (!(value < min) && !(value > max)) return {};
            return { { .message = fmt::format(
                "{} must be between {} and {}", value, min, max) } };
        }
    };

    template <typename T>
    in_range(T, T) -> in_range<T>;

}  // namespace validations

}  // namespace def_type
