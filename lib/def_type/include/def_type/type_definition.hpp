#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <def_type/field_def.hpp>

namespace def_type {

namespace detail {
    // Sentinel type for the non-templated type_def. Users never see this —
    // CTAD deduces type_def("Event") → type_def<detail::dynamic_tag>.
    struct dynamic_tag {};
}  // namespace detail

// ── type_definition — concept satisfied by every type_def mode ───────────
//
// The constraint that says "this thing describes a type's shape."
// type_def<T>, type_def("Event"), and type_def<T>().field(...) all satisfy it.

namespace detail {
    struct concept_sentinel_meta {};
}

template <typename T>
concept type_definition = requires(const T& t, std::string_view sv) {
    // Schema queries
    { t.name() } -> std::convertible_to<std::string_view>;
    { t.field_count() } -> std::convertible_to<std::size_t>;
    { t.field_names() } -> std::same_as<std::vector<std::string>>;
    { t.has_field(sv) } -> std::same_as<bool>;
    // Field query by name
    { t.field(sv) } -> std::same_as<field_def>;
    // Meta queries
    { t.template has_meta<detail::concept_sentinel_meta>() } -> std::same_as<bool>;
    { t.template meta<detail::concept_sentinel_meta>() } -> std::same_as<detail::concept_sentinel_meta>;
    { t.template meta_count<detail::concept_sentinel_meta>() } -> std::convertible_to<std::size_t>;
    { t.template metas<detail::concept_sentinel_meta>() } -> std::same_as<std::vector<detail::concept_sentinel_meta>>;
    // Schema iteration
    t.for_each_field([](auto) {});
    t.for_each_meta([](auto) {});
    // Create instance
    t.create();
};

}  // namespace def_type
