#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#ifdef DEF_TYPE_HAS_PFR
#include <pfr.hpp>
#endif

#include <def_type/field.hpp>

// ════════════════════════════════════════════════════════════════════════════
// Section 1 — PFR backend (opt-in via DEF_TYPE_HAS_PFR)
// ════════════════════════════════════════════════════════════════════════════

#ifdef DEF_TYPE_HAS_PFR

namespace def_type::pfr_impl {

template <typename T>
consteval std::size_t field_count() {
    return pfr::tuple_size_v<T>;
}

template <std::size_t I, typename T>
consteval std::string_view field_name() {
    return pfr::get_name<I, T>();
}

// Runtime field name extraction — completely avoids consteval to work around
// a compiler bug where consteval results get assigned to the wrong template
// instantiation across C++20 module boundaries. Affects clang and GCC.
//
// Uses __PRETTY_FUNCTION__ parsing at runtime (same technique as PFR, but
// evaluated at runtime instead of compile time).
#if !defined(_MSC_VER) || defined(__clang__)
template <auto ptr>
const char* name_of_field_pretty_fn() {
    return __PRETTY_FUNCTION__;
}

template <auto ptr>
std::string_view name_of_field_rt() {
    static const std::string_view result = [] {
        std::string_view sv = name_of_field_pretty_fn<ptr>();
#if defined(__clang__)
        // Clang: ...{&Type.field_name}]
        auto end = sv.rfind("}]");
        if (end == std::string_view::npos) end = sv.size();
        auto sub = sv.substr(0, end);
        auto sep = sub.rfind('.');
#elif defined(__GNUC__)
        // GCC: ...Type::field_name)]
        auto end = sv.rfind(")]");
        if (end == std::string_view::npos) end = sv.size();
        auto sub = sv.substr(0, end);
        auto sep = sub.rfind("::");
#endif
#if defined(__clang__)
        constexpr std::size_t sep_len = 1;  // "."
#elif defined(__GNUC__)
        constexpr std::size_t sep_len = 2;  // "::"
#endif
        if (sep == std::string_view::npos) return sv;
        return sub.substr(sep + sep_len);
    }();
    return result;
}

template <std::size_t I, typename T>
std::string_view field_name_rt() {
    return name_of_field_rt<
        pfr::detail::make_clang_wrapper(std::addressof(pfr::detail::sequence_tuple::get<I>(
            pfr::detail::tie_as_tuple(pfr::detail::fake_object<T>())
        )))
    >();
}
#endif

template <std::size_t I, typename T>
using member_type = pfr::tuple_element_t<I, T>;

template <std::size_t I, typename T>
constexpr decltype(auto) get_member(T& obj) {
    return pfr::get<I>(obj);
}

template <typename T, typename F>
constexpr void for_each_member(T& obj, F&& fn) {
    pfr::for_each_field(obj, std::forward<F>(fn));
}

}  // namespace def_type::pfr_impl

#endif

// ════════════════════════════════════════════════════════════════════════════
// Section 2 — Registry backend (structured-binding decomposition)
// ════════════════════════════════════════════════════════════════════════════

namespace def_type::registry {

// Decomposes aggregates via structured bindings up to 16 members.
// This is the non-PFR backend — works on any C++23 compiler.

template <std::size_t I, typename T>
constexpr decltype(auto) get_member(T& obj) {
    constexpr auto N = struct_info<std::remove_cvref_t<T>>().names.size();
    if constexpr (N == 1) {
        auto& [m0] = obj;
        if constexpr (I == 0) return (m0);
    } else if constexpr (N == 2) {
        auto& [m0, m1] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
    } else if constexpr (N == 3) {
        auto& [m0, m1, m2] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
    } else if constexpr (N == 4) {
        auto& [m0, m1, m2, m3] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
    } else if constexpr (N == 5) {
        auto& [m0, m1, m2, m3, m4] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
    } else if constexpr (N == 6) {
        auto& [m0, m1, m2, m3, m4, m5] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
    } else if constexpr (N == 7) {
        auto& [m0, m1, m2, m3, m4, m5, m6] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
        else if constexpr (I == 6) return (m6);
    } else if constexpr (N == 8) {
        auto& [m0, m1, m2, m3, m4, m5, m6, m7] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
        else if constexpr (I == 6) return (m6);
        else if constexpr (I == 7) return (m7);
    } else if constexpr (N == 9) {
        auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
        else if constexpr (I == 6) return (m6);
        else if constexpr (I == 7) return (m7);
        else if constexpr (I == 8) return (m8);
    } else if constexpr (N == 10) {
        auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
        else if constexpr (I == 6) return (m6);
        else if constexpr (I == 7) return (m7);
        else if constexpr (I == 8) return (m8);
        else if constexpr (I == 9) return (m9);
    } else if constexpr (N == 11) {
        auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
        else if constexpr (I == 6) return (m6);
        else if constexpr (I == 7) return (m7);
        else if constexpr (I == 8) return (m8);
        else if constexpr (I == 9) return (m9);
        else if constexpr (I == 10) return (m10);
    } else if constexpr (N == 12) {
        auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
        else if constexpr (I == 6) return (m6);
        else if constexpr (I == 7) return (m7);
        else if constexpr (I == 8) return (m8);
        else if constexpr (I == 9) return (m9);
        else if constexpr (I == 10) return (m10);
        else if constexpr (I == 11) return (m11);
    } else if constexpr (N == 13) {
        auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
        else if constexpr (I == 6) return (m6);
        else if constexpr (I == 7) return (m7);
        else if constexpr (I == 8) return (m8);
        else if constexpr (I == 9) return (m9);
        else if constexpr (I == 10) return (m10);
        else if constexpr (I == 11) return (m11);
        else if constexpr (I == 12) return (m12);
    } else if constexpr (N == 14) {
        auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
        else if constexpr (I == 6) return (m6);
        else if constexpr (I == 7) return (m7);
        else if constexpr (I == 8) return (m8);
        else if constexpr (I == 9) return (m9);
        else if constexpr (I == 10) return (m10);
        else if constexpr (I == 11) return (m11);
        else if constexpr (I == 12) return (m12);
        else if constexpr (I == 13) return (m13);
    } else if constexpr (N == 15) {
        auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
        else if constexpr (I == 6) return (m6);
        else if constexpr (I == 7) return (m7);
        else if constexpr (I == 8) return (m8);
        else if constexpr (I == 9) return (m9);
        else if constexpr (I == 10) return (m10);
        else if constexpr (I == 11) return (m11);
        else if constexpr (I == 12) return (m12);
        else if constexpr (I == 13) return (m13);
        else if constexpr (I == 14) return (m14);
    } else if constexpr (N == 16) {
        auto& [m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15] = obj;
        if constexpr (I == 0) return (m0);
        else if constexpr (I == 1) return (m1);
        else if constexpr (I == 2) return (m2);
        else if constexpr (I == 3) return (m3);
        else if constexpr (I == 4) return (m4);
        else if constexpr (I == 5) return (m5);
        else if constexpr (I == 6) return (m6);
        else if constexpr (I == 7) return (m7);
        else if constexpr (I == 8) return (m8);
        else if constexpr (I == 9) return (m9);
        else if constexpr (I == 10) return (m10);
        else if constexpr (I == 11) return (m11);
        else if constexpr (I == 12) return (m12);
        else if constexpr (I == 13) return (m13);
        else if constexpr (I == 14) return (m14);
        else if constexpr (I == 15) return (m15);
    } else {
        static_assert(N <= 16, "Registry backend supports up to 16 fields. Use PFR for more.");
    }
}

template <std::size_t I, typename T>
using member_type = std::remove_cvref_t<decltype(get_member<I>(std::declval<T&>()))>;

template <typename T, typename F, std::size_t... Is>
constexpr void for_each_member_impl(T& obj, F&& fn, std::index_sequence<Is...>) {
    (fn(get_member<Is>(obj), std::integral_constant<std::size_t, Is>{}), ...);
}

template <typename T, typename F>
constexpr void for_each_member(T& obj, F&& fn) {
    constexpr auto N = struct_info<std::remove_cvref_t<T>>().names.size();
    for_each_member_impl(obj, std::forward<F>(fn), std::make_index_sequence<N>{});
}

}  // namespace def_type::registry

// ════════════════════════════════════════════════════════════════════════════
// Section 3 — Exported API (dispatch logic, reflection, concepts, helpers)
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
    consteval bool any_member_is_field(std::index_sequence<Is...>) {
        return (is_field<member_type<Is, T>> || ...);
    }

    template <typename T>
    consteval bool has_any_field_member() {
        if constexpr (!std::is_aggregate_v<T>) {
            return false;
        } else {
            constexpr auto N = dispatch_field_count<T>();
            if constexpr (N == 0) {
                return false;
            } else {
                return any_member_is_field<T>(std::make_index_sequence<N>{});
            }
        }
    }

    // Count only Field<> members (not all struct members)
    template <typename T, std::size_t... Is>
    consteval std::size_t count_field_members(std::index_sequence<Is...>) {
        return (0 + ... + (is_field<member_type<Is, T>> ? 1 : 0));
    }

    template <typename T>
    concept reflected_struct =
        std::is_aggregate_v<T>
        && has_any_field_member<T>();

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

    // Collect only Field member names into an array
    template <typename T, std::size_t... Is>
    constexpr auto collect_field_names(std::index_sequence<Is...>) {
        constexpr std::size_t total = sizeof...(Is);
        // First pass: count Field members
        constexpr std::size_t field_n = (0 + ... + (is_field<member_type<Is, T>> ? 1 : 0));
        // Second pass: collect their names
        std::array<std::string_view, field_n> result{};
        std::size_t idx = 0;
        ((is_field<member_type<Is, T>>
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
        static_assert(is_field<member_t>, "meta<M>() requires a field<> member");
        using with_type = std::remove_cvref_t<decltype(std::declval<member_t>().with)>;
        static_assert(std::is_base_of_v<M, with_type>,
            "meta<M>() called with a meta type not present on this field's with<>");
        T instance{};
        return static_cast<const M&>(detail::dispatch_get_member<I>(instance).with);
    }

    constexpr auto value() const {
        T instance{};
        return detail::dispatch_get_member<I>(instance).value;
    }

    constexpr auto with() const {
        T instance{};
        return detail::dispatch_get_member<I>(instance).with;
    }
};

}  // namespace def_type
