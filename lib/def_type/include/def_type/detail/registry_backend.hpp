#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

#include <def_type/field.hpp>

// ════════════════════════════════════════════════════════════════════════════
// Registry backend (structured-binding decomposition)
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
