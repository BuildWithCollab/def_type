module;

#include <array>
#include <cstddef>
#include <concepts>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <nameof.hpp>

export module collab.core:field;

export namespace collab::model {

// ── validation_error — shared across all paths ──────────────────────────

struct validation_error {
    std::string path;
    std::string message;
    std::string constraint;
};

// ── with<Exts...> ───────────────────────────────────────────────────────

template <typename... Exts>
struct with : Exts... {};

namespace detail {

    template <typename T>
    using field_validator_fn = std::function<
        std::vector<validation_error>(const T& value, std::string_view field_name)>;

    inline std::string extract_short_validator_name(std::string_view full_name) {
        if (auto bracket = full_name.rfind('['); bracket != std::string_view::npos)
            full_name = full_name.substr(0, bracket);
        if (auto at_sign = full_name.rfind('@'); at_sign != std::string_view::npos)
            full_name = full_name.substr(0, at_sign);
        if (full_name.starts_with("struct "))
            full_name.remove_prefix(7);
        else if (full_name.starts_with("class "))
            full_name.remove_prefix(6);
        if (auto last_colon = full_name.rfind("::"); last_colon != std::string_view::npos)
            full_name = full_name.substr(last_colon + 2);
        return std::string(full_name);
    }

}  // namespace detail

// ── validator_pack — wrapper returned by validators() ───────────────────

template <typename... Vs>
struct validator_pack {
    std::tuple<Vs...> packed;

    // Implicit conversion to field_validator_fn<T> for use in field<T>.validators
    template <typename T>
    operator detail::field_validator_fn<T>() const {
        auto captured = packed;
        return [captured](const T& value, std::string_view field_name)
            -> std::vector<validation_error>
        {
            std::vector<validation_error> errors;
            std::apply([&](const auto&... each_validator) {
                (([&] {
                    auto result = each_validator(value);
                    if (result.has_value()) {
                        using validator_type = std::remove_cvref_t<decltype(each_validator)>;
                        errors.push_back({
                            std::string(field_name),
                            std::move(*result),
                            detail::extract_short_validator_name(NAMEOF_TYPE(validator_type))
                        });
                    }
                }()), ...);
            }, captured);
            return errors;
        };
    }
};

// ── validators() — composes validators into a pack ──────────────────────

template <typename... Vs>
validator_pack<Vs...> validators(Vs... vs) {
    return {std::tuple<Vs...>{std::move(vs)...}};
}

// ── field<T, WithPack> ─────────────────────────────────────────────────

template <typename T, typename WithPack = with<>>
struct field {
    using value_type = T;

    WithPack              with{};
    T                     value{};
    detail::field_validator_fn<T> validators{};

    constexpr operator const T&() const& noexcept { return value; }
    constexpr operator       T&()       & noexcept { return value; }
    constexpr operator       T&&()      && noexcept { return std::move(value); }

    constexpr field& operator=(const T& v) {
        value = v;
        return *this;
    }
    constexpr field& operator=(T&& v) noexcept(std::is_nothrow_move_assignable_v<T>) {
        value = std::move(v);
        return *this;
    }

    constexpr const T* operator->() const noexcept { return &value; }
    constexpr       T* operator->()       noexcept { return &value; }
};

// ── is_field concept ─────────────────────────────────────────────────────

namespace detail {

    template <typename U>
    inline constexpr bool is_field_v = false;

    template <typename T, typename W>
    inline constexpr bool is_field_v<field<T, W>> = true;

}  // namespace detail

template <typename T>
concept is_field = detail::is_field_v<std::remove_cvref_t<T>>;

// ── struct_info<T>() — user registration point ───────────────────────────
//
// Specialize this for your type to register it for reflection.
// The primary template is deleted — unspecialized types fall through
// to the PFR backend (if enabled) or hit a static_assert.
//
// Example:
//   template <>
//   constexpr auto collab::model::struct_info<MyArgs>() {
//       return collab::model::field_info<MyArgs>("name", "age", "active");
//   }

template <typename T>
constexpr auto struct_info() = delete;

// ── field_info — registration helper ────────────────────────────────────

template <std::size_t N>
struct field_names_t {
    std::array<std::string_view, N> names{};
};

template <typename T, typename... Strs>
consteval auto field_info(Strs... names) -> field_names_t<sizeof...(Strs)> {
    return {.names = {std::string_view(names)...}};
}

// ── has_struct_info detection ──────────────────────────────────────────────

namespace detail {

    template <typename T>
    concept has_struct_info = requires { struct_info<T>(); };

}  // namespace detail

}  // namespace collab::model
