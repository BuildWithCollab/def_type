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

    // Fails when value < min or value > max. T is the type of both the bounds
    // and the value being checked — there is no implicit conversion between
    // them, so `in_range<int>` will refuse to validate a `double` field at
    // compile time. T must satisfy `std::totally_ordered` (defines `<`, `>`,
    // `<=`, `>=`, `==`, `!=`).
    //
    // Pick T via CTAD on positional init, or write it explicitly:
    //   in_range{1, 100}                          // in_range<int>
    //   in_range{0.0, 1.0}                        // in_range<double>
    //   in_range{.min = 1, .max = 100}            // in_range<int> (aggregate CTAD)
    //   in_range<std::int64_t>{.min = 0, .max = 9}

    template <std::totally_ordered T>
    struct in_range {
        T min;
        T max;

        // operator() is constrained to U = T — same_as, not convertible_to —
        // so `in_range<int>` cannot be silently invoked on a double field via
        // narrowing conversion, and `in_range<double>` cannot be silently
        // invoked on an int field. Mismatch is a compile error at the field
        // declaration that pairs validator with wrong-type field.
        template <typename U>
            requires std::same_as<U, T>
        std::vector<validation_error> operator()(const U& value) const {
            if (value >= min && value <= max) return {};
            return { { .message = fmt::format(
                "{} must be between {} and {}", value, min, max) } };
        }
    };

    template <typename T>
    in_range(T, T) -> in_range<T>;

}  // namespace validations

}  // namespace def_type
