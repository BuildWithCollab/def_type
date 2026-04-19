#pragma once

#include <cstddef>
#include <string_view>

#ifdef DEF_TYPE_HAS_PFR
#include <boost/pfr.hpp>
#endif

#include <def_type/field.hpp>

// ════════════════════════════════════════════════════════════════════════════
// PFR backend (opt-in via DEF_TYPE_HAS_PFR)
// ════════════════════════════════════════════════════════════════════════════

#ifdef DEF_TYPE_HAS_PFR

namespace def_type::pfr_impl {

template <typename T>
consteval std::size_t field_count() {
    return boost::pfr::tuple_size_v<T>;
}

template <std::size_t I, typename T>
consteval std::string_view field_name() {
    return boost::pfr::get_name<I, T>();
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
        boost::pfr::detail::make_clang_wrapper(std::addressof(boost::pfr::detail::sequence_tuple::get<I>(
            boost::pfr::detail::tie_as_tuple(boost::pfr::detail::fake_object<T>())
        )))
    >();
}
#endif

template <std::size_t I, typename T>
using member_type = boost::pfr::tuple_element_t<I, T>;

template <std::size_t I, typename T>
constexpr decltype(auto) get_member(T& obj) {
    return boost::pfr::get<I>(obj);
}

template <typename T, typename F>
constexpr void for_each_member(T& obj, F&& fn) {
    boost::pfr::for_each_field(obj, std::forward<F>(fn));
}

}  // namespace def_type::pfr_impl

#endif
