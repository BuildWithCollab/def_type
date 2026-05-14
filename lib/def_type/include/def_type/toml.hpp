#pragma once

#include <toml.hpp>

#include <magic_enum/magic_enum.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <def_type/field.hpp>
#include <def_type/meta.hpp>
#include <def_type/reflect.hpp>
#include <def_type/typed_type_def.hpp>
#include <def_type/json.hpp>   // reuse is_optional_v / is_iterable_array_v / is_map_v / element traits

namespace def_type {

// ── value_to_toml — struct/value → ::toml::value ────────────────────────

namespace detail {

    template <typename T>
    ::toml::value value_to_toml(const T& v) {
        if constexpr (is_optional_v<T>) {
            if (v.has_value()) return value_to_toml(*v);
            return ::toml::value{};   // caller (struct emitter) decides to omit
        } else if constexpr (is_iterable_array_v<T>) {
            ::toml::array arr;
            for (const auto& elem : v) arr.push_back(value_to_toml(elem));
            return ::toml::value(std::move(arr));
        } else if constexpr (is_map_v<T>) {
            ::toml::table tbl;
            for (const auto& [k, val] : v) tbl[k] = value_to_toml(val);
            return ::toml::value(std::move(tbl));
        } else if constexpr (reflected_struct<T>) {
            ::toml::table tbl;
            type_def<T>{}.for_each_field(v, [&](std::string_view name, const auto& value) {
                using V = std::remove_cvref_t<decltype(value)>;
                if constexpr (is_optional_v<V>) {
                    if (value.has_value())
                        tbl[std::string(name)] = value_to_toml(*value);
                } else {
                    tbl[std::string(name)] = value_to_toml(value);
                }
            });
            return ::toml::value(std::move(tbl));
        } else if constexpr (std::is_enum_v<T>) {
            std::string_view name = magic_enum::enum_name(v);
            if (name.empty())
                return ::toml::value(static_cast<std::int64_t>(v));
            return ::toml::value(std::string(name));
        } else if constexpr (std::is_same_v<T, std::string>) {
            return ::toml::value(v);
        } else if constexpr (std::is_same_v<T, bool>) {
            return ::toml::value(v);
        } else if constexpr (std::is_integral_v<T>) {
            return ::toml::value(static_cast<std::int64_t>(v));
        } else if constexpr (std::is_floating_point_v<T>) {
            return ::toml::value(static_cast<double>(v));
        } else {
            return ::toml::value(v);
        }
    }

}  // namespace detail

// ── value_from_toml — ::toml::value → struct/value ──────────────────────

namespace detail {

    template <typename T>
    void value_from_toml(const ::toml::value& v, T& out) {
        if constexpr (is_optional_v<T>) {
            using Inner = optional_inner_t<T>;
            Inner inner = construct_default<Inner>();
            value_from_toml(v, inner);
            out = std::move(inner);
        } else if constexpr (is_iterable_array_v<T>) {
            if (!v.is_array())
                throw std::logic_error("from_toml: expected array");
            using Elem = array_element_t<T>;
            out.clear();
            for (const auto& elem : v.as_array()) {
                Elem e = construct_default<Elem>();
                value_from_toml(elem, e);
                if constexpr (requires { out.push_back(e); }) out.push_back(std::move(e));
                else out.insert(std::move(e));
            }
        } else if constexpr (is_map_v<T>) {
            if (!v.is_table())
                throw std::logic_error("from_toml: expected table for map");
            using V = map_value_t<T>;
            out.clear();
            for (const auto& [key, val] : v.as_table()) {
                V vv = construct_default<V>();
                value_from_toml(val, vv);
                out.insert_or_assign(key, std::move(vv));
            }
        } else if constexpr (reflected_struct<T>) {
            if (!v.is_table())
                throw std::logic_error("from_toml: expected table for struct");
            type_def<T>{}.for_each_field(out, [&](std::string_view name, auto& value) {
                std::string key(name);
                if (v.contains(key))
                    value_from_toml(v.at(key), value);
            });
        } else if constexpr (std::is_enum_v<T>) {
            if (v.is_string()) {
                const auto& str = v.as_string();
                bool found = false;
                for (auto val : magic_enum::enum_values<T>()) {
                    if (magic_enum::enum_name(val) == str) { out = val; found = true; break; }
                }
                if (!found)
                    throw std::logic_error("from_toml: unknown enum value '" + std::string(str) + "'");
            } else if (v.is_integer()) {
                out = static_cast<T>(v.as_integer());
            } else {
                throw std::logic_error("from_toml: expected string or integer for enum");
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (!v.is_string())
                throw std::logic_error("from_toml: expected string");
            out = v.as_string();
        } else if constexpr (std::is_same_v<T, bool>) {
            if (!v.is_boolean())
                throw std::logic_error("from_toml: expected boolean");
            out = v.as_boolean();
        } else if constexpr (std::is_integral_v<T>) {
            if (!v.is_integer())
                throw std::logic_error("from_toml: expected integer");
            out = static_cast<T>(v.as_integer());
        } else if constexpr (std::is_floating_point_v<T>) {
            if (v.is_floating())      out = static_cast<T>(v.as_floating());
            else if (v.is_integer())  out = static_cast<T>(v.as_integer());
            else throw std::logic_error("from_toml: expected number");
        } else {
            out = v.as<T>();
        }
    }

}  // namespace detail

// ── merge_into_toml — write current struct values into an existing
//    ::toml::value, preserving comments on existing nodes ───────────────

namespace detail {

    template <typename T>
    void merge_into_toml(::toml::value& src, const T& v) {
        if constexpr (is_optional_v<T>) {
            if (v.has_value()) merge_into_toml(src, *v);
            // Empty optional: leave src untouched. (Removal is a later concern.)
        } else if constexpr (is_iterable_array_v<T>) {
            // Rebuild the array from scratch — element-position comment
            // alignment in arrays is fragile; we preserve comments on the
            // array as a whole but not on individual elements for v1.
            src = value_to_toml(v);
        } else if constexpr (is_map_v<T>) {
            // Update keys in place; add new keys, remove vanished keys
            auto& tbl = src.as_table();
            for (const auto& [k, val] : v) {
                auto it = tbl.find(k);
                if (it != tbl.end()) merge_into_toml(it->second, val);
                else tbl[k] = value_to_toml(val);
            }
            // Drop keys present in src but not in v
            for (auto it = tbl.begin(); it != tbl.end(); ) {
                if (v.find(it->first) == v.end()) it = tbl.erase(it);
                else ++it;
            }
        } else if constexpr (reflected_struct<T>) {
            if (!src.is_table()) src = ::toml::value(::toml::table{});
            auto& tbl = src.as_table();
            type_def<T>{}.for_each_field(v, [&](std::string_view name, const auto& field_value) {
                using V = std::remove_cvref_t<decltype(field_value)>;
                std::string key(name);
                if constexpr (is_optional_v<V>) {
                    if (field_value.has_value()) {
                        auto it = tbl.find(key);
                        if (it != tbl.end()) merge_into_toml(it->second, *field_value);
                        else tbl[key] = value_to_toml(*field_value);
                    }
                } else {
                    auto it = tbl.find(key);
                    if (it != tbl.end()) merge_into_toml(it->second, field_value);
                    else tbl[key] = value_to_toml(field_value);
                }
            });
        } else if constexpr (std::is_enum_v<T>) {
            std::string_view name = magic_enum::enum_name(v);
            if (!name.empty() && src.is_string())
                src.as_string() = std::string(name);
            else
                src = value_to_toml(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (src.is_string()) src.as_string() = v;
            else                 src = ::toml::value(v);
        } else if constexpr (std::is_same_v<T, bool>) {
            if (src.is_boolean()) src.as_boolean() = v;
            else                  src = ::toml::value(v);
        } else if constexpr (std::is_integral_v<T>) {
            if (src.is_integer()) src.as_integer() = static_cast<std::int64_t>(v);
            else                  src = ::toml::value(static_cast<std::int64_t>(v));
        } else if constexpr (std::is_floating_point_v<T>) {
            if (src.is_floating()) src.as_floating() = static_cast<double>(v);
            else                   src = ::toml::value(static_cast<double>(v));
        } else {
            src = value_to_toml(v);
        }
    }

}  // namespace detail

// ── toml_doc<T> — typed view + source AST for comment-preserving round-trip

template <typename T>
struct toml_doc {
    T              value{};
    ::toml::value  source{};

    T&        operator*()        { return value; }
    const T&  operator*()  const { return value; }
    T*        operator->()       { return &value; }
    const T*  operator->() const { return &value; }
};

// ── Public API ──────────────────────────────────────────────────────────

// from_toml — parse a TOML string into a typed doc with source AST retained
template <detail::reflected_struct T>
toml_doc<T> from_toml(const std::string& toml_str) {
    auto source = ::toml::parse_str(toml_str);
    T value{};
    detail::value_from_toml(source, value);
    return toml_doc<T>{ std::move(value), std::move(source) };
}

// to_toml — struct → ::toml::value (no comment machinery)
template <detail::reflected_struct T>
::toml::value to_toml(const T& obj) {
    return detail::value_to_toml(obj);
}

// to_toml_string — plain struct → TOML string (no comments)
template <detail::reflected_struct T>
std::string to_toml_string(const T& obj) {
    return ::toml::format(detail::value_to_toml(obj));
}

// to_toml_string — doc → TOML string (comments / extra keys preserved by
// merging current struct values into the held source AST)
template <detail::reflected_struct T>
std::string to_toml_string(const toml_doc<T>& doc) {
    ::toml::value out = doc.source;
    detail::merge_into_toml(out, doc.value);
    return ::toml::format(out);
}

}  // namespace def_type
