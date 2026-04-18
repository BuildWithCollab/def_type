#pragma once

#include <type_traits>

namespace def_type {

// ── meta<T> — type-level metadata wrapper ────────────────────────────────
//
// Attach metadata to a struct by making it a member:
//
//   struct Dog {
//       meta<endpoint_info> endpoint{{.path = "/dogs", .method = "POST"}};
//       field<std::string>  name;
//       field<int>          age;
//   };
//
// The auto-discovery system (type_def<T>) categorizes meta<T> members as
// type-level metadata, separate from field<T> data members.

template <typename T>
struct meta {
    T value{};

    constexpr const T* operator->() const noexcept { return &value; }
    constexpr       T* operator->()       noexcept { return &value; }
};

// ── is_meta concept ──────────────────────────────────────────────────────

namespace detail {

    template <typename U>
    inline constexpr bool is_meta_v = false;

    template <typename T>
    inline constexpr bool is_meta_v<meta<T>> = true;

}  // namespace detail

template <typename T>
concept is_meta = detail::is_meta_v<std::remove_cvref_t<T>>;

// ── meta type helpers ────────────────────────────────────────────────────

namespace detail {

    // Extract inner type: meta_inner_t<meta<endpoint_info>> = endpoint_info
    template <typename U>
    struct meta_inner;

    template <typename T>
    struct meta_inner<meta<T>> {
        using type = T;
    };

    template <typename U>
    using meta_inner_t = typename meta_inner<std::remove_cvref_t<U>>::type;

    // Check if a meta wraps a specific type: is_meta_of_v<meta<Foo>, Foo> = true
    template <typename MetaWrapper, typename M>
    inline constexpr bool is_meta_of_v = false;

    template <typename M>
    inline constexpr bool is_meta_of_v<meta<M>, M> = true;

}  // namespace detail

}  // namespace def_type
