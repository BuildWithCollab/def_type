#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#include <def_type/field.hpp>
#include <def_type/meta.hpp>
#include <def_type/detail/pfr_backend.hpp>
#include <def_type/detail/registry_backend.hpp>

// ════════════════════════════════════════════════════════════════════════════
// Dispatch logic, reflection, concepts, helpers
// ════════════════════════════════════════════════════════════════════════════

namespace def_type {

// ── type_name<T>() ──────────────────────────────────────────────────────

namespace detail {

    template <typename T>
    consteval std::string_view extract_type_name() {
#if defined(_MSC_VER)
        std::string_view sv = __FUNCSIG__;
        auto start = sv.find("extract_type_name<") + 18;
        if (sv.substr(start, 7) == "struct ") start += 7;
        else if (sv.substr(start, 6) == "class ")  start += 6;
        else if (sv.substr(start, 5) == "enum ")   start += 5;
        auto end = sv.rfind(">(void)");
        return sv.substr(start, end - start);
#elif defined(__clang__)
        std::string_view sv = __PRETTY_FUNCTION__;
        auto start = sv.find("T = ") + 4;
        auto end = sv.rfind("]");
        return sv.substr(start, end - start);
#elif defined(__GNUC__)
        std::string_view sv = __PRETTY_FUNCTION__;
        auto start = sv.find("T = ") + 4;
        auto end = sv.find(";", start);
        if (end == std::string_view::npos) end = sv.rfind("]");
        return sv.substr(start, end - start);
#else
        return "unknown";
#endif
    }

    template <typename T>
    consteval std::string_view type_name() {
        return extract_type_name<T>();
    }

}  // namespace detail

// ── Backend dispatch ────────────────────────────────────────────────────
//
// One ifdef picks the fallback. Everything else is clean C++.

namespace detail {

#ifdef DEF_TYPE_HAS_PFR
inline constexpr bool has_pfr_backend = true;
#else
inline constexpr bool has_pfr_backend = false;
#endif

    // ── Fallback backend ─────────────────────────────────────────────
    //
    // When PFR is enabled, the fallback delegates to pfr_impl.
    // When PFR is disabled, every method static_asserts.

    template <bool PfrEnabled>
    struct fallback;

    template <>
    struct fallback<false> {
        template <typename T>
        static consteval std::size_t field_count() {
            static_assert(has_struct_info<T>,
                "Type has no struct_info<T>() specialization and PFR is not enabled.");
            return 0;
        }

        template <std::size_t I, typename T>
        static consteval std::string_view field_name() {
            static_assert(has_struct_info<T>,
                "Type has no struct_info<T>() specialization and PFR is not enabled.");
            return "";
        }

#if !defined(_MSC_VER) || defined(__clang__)
        template <std::size_t I, typename T>
        static std::string_view field_name_rt() {
            static_assert(has_struct_info<T>,
                "Type has no struct_info<T>() specialization and PFR is not enabled.");
            return "";
        }
#endif

        template <std::size_t I, typename T>
        using member_type = void;

        template <std::size_t I, typename T>
        static constexpr decltype(auto) get_member(T&) {
            static_assert(has_struct_info<std::remove_cvref_t<T>>,
                "Type has no struct_info<T>() specialization and PFR is not enabled.");
        }

        template <typename T, typename F>
        static constexpr void for_each_member(T&, F&&) {
            static_assert(has_struct_info<std::remove_cvref_t<T>>,
                "Type has no struct_info<T>() specialization and PFR is not enabled.");
        }
    };

#ifdef DEF_TYPE_HAS_PFR
    template <>
    struct fallback<true> {
        template <typename T>
        static consteval std::size_t field_count() {
            return pfr_impl::field_count<T>();
        }

        template <std::size_t I, typename T>
        static consteval std::string_view field_name() {
            return pfr_impl::field_name<I, T>();
        }

#if !defined(_MSC_VER) || defined(__clang__)
        template <std::size_t I, typename T>
        static std::string_view field_name_rt() {
            return pfr_impl::field_name_rt<I, T>();
        }
#endif

        template <std::size_t I, typename T>
        using member_type = pfr_impl::member_type<I, T>;

        template <std::size_t I, typename T>
        static constexpr decltype(auto) get_member(T& obj) {
            return pfr_impl::get_member<I>(obj);
        }

        template <typename T, typename F>
        static constexpr void for_each_member(T& obj, F&& fn) {
            pfr_impl::for_each_member(obj, std::forward<F>(fn));
        }
    };
#endif

    using pfr_fallback = fallback<has_pfr_backend>;

    // ── Dispatch: registry first, PFR fallback second ────────────────

    template <typename T>
    consteval std::size_t dispatch_field_count() {
        if constexpr (has_struct_info<T>) return struct_info<T>().names.size();
        else return pfr_fallback::template field_count<T>();
    }

    template <std::size_t I, typename T>
    consteval std::string_view dispatch_field_name() {
        if constexpr (has_struct_info<T>) return struct_info<T>().names[I];
        else return pfr_fallback::template field_name<I, T>();
    }

    // Runtime-safe variant: avoids consteval string_view materialization
    // to work around a clang bug where consteval results get assigned to
    // the wrong template instantiation across C++20 module boundaries.
    // GCC and MSVC don't have this bug, so they use the consteval path.
    template <std::size_t I, typename T>
    std::string_view dispatch_field_name_rt() {
        if constexpr (has_struct_info<T>) return struct_info<T>().names[I];
#if !defined(_MSC_VER) || defined(__clang__)
        else return pfr_fallback::template field_name_rt<I, T>();
#else
        else return pfr_fallback::template field_name<I, T>();
#endif
    }

    // Lazy member_type — only instantiates the chosen backend
    template <std::size_t I, typename T, bool Registered = has_struct_info<T>>
    struct dispatch_member_type_impl;

    template <std::size_t I, typename T>
    struct dispatch_member_type_impl<I, T, true> {
        using type = registry::member_type<I, T>;
    };

    template <std::size_t I, typename T>
    struct dispatch_member_type_impl<I, T, false> {
        using type = typename pfr_fallback::template member_type<I, T>;
    };

    template <std::size_t I, typename T>
    using member_type = typename dispatch_member_type_impl<I, T>::type;

    template <std::size_t I, typename T>
    constexpr decltype(auto) dispatch_get_member(T& obj) {
        if constexpr (has_struct_info<std::remove_cvref_t<T>>) return registry::get_member<I>(obj);
        else return pfr_fallback::template get_member<I>(obj);
    }

    template <typename T, typename F>
    constexpr void dispatch_for_each_member(T& obj, F&& fn) {
        if constexpr (has_struct_info<std::remove_cvref_t<T>>) registry::for_each_member(obj, std::forward<F>(fn));
        else pfr_fallback::for_each_member(obj, std::forward<F>(fn));
    }

}  // namespace detail

// ── reflected_struct concept ─────────────────────────────────────────────

namespace detail {

    template <typename T, std::size_t... Is>
    consteval bool any_member_is_reflectable(std::index_sequence<Is...>) {
        return (!is_meta<member_type<Is, T>> || ...);
    }

    template <typename T>
    consteval bool has_any_reflectable_member() {
        if constexpr (!std::is_aggregate_v<T>) {
            return false;
        } else {
            constexpr auto N = dispatch_field_count<T>();
            if constexpr (N == 0) {
                return false;
            } else {
                return any_member_is_reflectable<T>(std::make_index_sequence<N>{});
            }
        }
    }

    // Count reflectable members (everything except meta<>)
    template <typename T, std::size_t... Is>
    consteval std::size_t count_field_members(std::index_sequence<Is...>) {
        return (0 + ... + (!is_meta<member_type<Is, T>> ? 1 : 0));
    }

    template <typename T>
    concept reflected_struct =
        std::is_aggregate_v<T>
        && has_any_reflectable_member<T>();

    template <typename T>
    consteval std::size_t field_count() {
        return count_field_members<T>(
            std::make_index_sequence<dispatch_field_count<T>()>{}
        );
    }

    template <std::size_t I, typename T>
    consteval std::string_view field_name() {
        return dispatch_field_name<I, T>();
    }

}  // namespace detail

namespace detail {

    // Collect reflectable member names into an array (everything except meta<>)
    template <typename T, std::size_t... Is>
    constexpr auto collect_field_names(std::index_sequence<Is...>) {
        constexpr std::size_t total = sizeof...(Is);
        // First pass: count non-meta members
        constexpr std::size_t field_n = (0 + ... + (!is_meta<member_type<Is, T>> ? 1 : 0));
        // Second pass: collect their names
        std::array<std::string_view, field_n> result{};
        std::size_t idx = 0;
        ((!is_meta<member_type<Is, T>>
            ? (result[idx++] = dispatch_field_name_rt<Is, T>(), 0) : 0), ...);
        return result;
    }

    template <typename T>
    constexpr auto field_names() {
        constexpr auto N = dispatch_field_count<T>();
        return collect_field_names<T>(std::make_index_sequence<N>{});
    }

}  // namespace detail

// ── field_descriptor<T, I> — schema-only field info ─────────────────────

template <typename T, std::size_t I>
struct field_descriptor {
    std::string_view name() const {
        return detail::dispatch_field_name_rt<I, T>();
    }

    consteval std::size_t index() const { return I; }

    template <typename M>
    consteval bool has_meta() const {
        using member_t = detail::member_type<I, T>;
        if constexpr (is_field<member_t>) {
            using with_type = std::remove_cvref_t<decltype(std::declval<member_t>().with)>;
            return std::is_base_of_v<M, with_type>;
        } else {
            return false;
        }
    }

    template <typename M>
    constexpr M meta() const {
        using member_t = detail::member_type<I, T>;
        static_assert(is_field<member_t>, "meta<M>() requires a field<> member with with<>");
        using with_type = std::remove_cvref_t<decltype(std::declval<member_t>().with)>;
        static_assert(std::is_base_of_v<M, with_type>,
            "meta<M>() called with a meta type not present on this field's with<>");
        T instance{};
        return static_cast<const M&>(detail::dispatch_get_member<I>(instance).with);
    }

    constexpr auto value() const {
        using member_t = detail::member_type<I, T>;
        T instance{};
        if constexpr (is_field<member_t>) {
            return detail::dispatch_get_member<I>(instance).value;
        } else {
            return detail::dispatch_get_member<I>(instance);
        }
    }

    constexpr auto with() const {
        using member_t = detail::member_type<I, T>;
        static_assert(is_field<member_t>, "with() requires a field<> member");
        T instance{};
        return detail::dispatch_get_member<I>(instance).with;
    }
};

}  // namespace def_type
