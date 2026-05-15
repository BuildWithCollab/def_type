#pragma once

#include <yaml-cpp/yaml.h>

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
#include <def_type/json.hpp>   // shared traits: is_optional_v, is_iterable_array_v, is_map_v, …

namespace def_type {

// ── value_to_yaml — struct/value → YAML::Node ───────────────────────────

namespace detail {

    template <typename T>
    YAML::Node value_to_yaml(const T& v) {
        if constexpr (is_optional_v<T>) {
            if (v.has_value()) return value_to_yaml(*v);
            return YAML::Node();   // caller (struct emitter) decides to omit
        } else if constexpr (is_iterable_array_v<T>) {
            YAML::Node seq(YAML::NodeType::Sequence);
            for (const auto& elem : v) seq.push_back(value_to_yaml(elem));
            return seq;
        } else if constexpr (is_map_v<T>) {
            YAML::Node map(YAML::NodeType::Map);
            for (const auto& [k, val] : v) map[k] = value_to_yaml(val);
            return map;
        } else if constexpr (reflected_struct<T>) {
            YAML::Node map(YAML::NodeType::Map);
            type_def<T>{}.for_each_field(v, [&](std::string_view name, const auto& value) {
                using V = std::remove_cvref_t<decltype(value)>;
                if constexpr (is_optional_v<V>) {
                    if (value.has_value())
                        map[std::string(name)] = value_to_yaml(*value);
                } else {
                    map[std::string(name)] = value_to_yaml(value);
                }
            });
            return map;
        } else if constexpr (std::is_enum_v<T>) {
            std::string_view name = magic_enum::enum_name(v);
            if (name.empty())
                return YAML::Node(static_cast<std::int64_t>(v));
            return YAML::Node(std::string(name));
        } else if constexpr (std::is_same_v<T, std::string>) {
            return YAML::Node(v);
        } else if constexpr (std::is_same_v<T, bool>) {
            return YAML::Node(v);
        } else if constexpr (std::is_integral_v<T>) {
            return YAML::Node(static_cast<std::int64_t>(v));
        } else if constexpr (std::is_floating_point_v<T>) {
            return YAML::Node(static_cast<double>(v));
        } else {
            static_assert(!is_non_string_keyed_map_v<T>,
                "def_type: map keys must be std::string for serialization. "
                "YAML mapping keys (like JSON object keys / TOML table keys) "
                "are strings; non-string-keyed maps have no representation.");
            return YAML::Node(v);
        }
    }

}  // namespace detail

// ── value_from_yaml — YAML::Node → struct/value ─────────────────────────

namespace detail {

    template <typename T>
    void value_from_yaml(const YAML::Node& n, T& out) {
        if constexpr (is_optional_v<T>) {
            using Inner = optional_inner_t<T>;
            if (!n || n.IsNull()) { out = std::nullopt; }
            else {
                Inner inner = construct_default<Inner>();
                value_from_yaml(n, inner);
                out = std::move(inner);
            }
        } else if constexpr (is_iterable_array_v<T>) {
            if (!n.IsSequence())
                throw std::logic_error("from_yaml: expected sequence");
            using Elem = array_element_t<T>;
            out.clear();
            for (const auto& elem : n) {
                Elem e = construct_default<Elem>();
                value_from_yaml(elem, e);
                if constexpr (requires { out.push_back(e); }) out.push_back(std::move(e));
                else out.insert(std::move(e));
            }
        } else if constexpr (is_map_v<T>) {
            if (!n.IsMap())
                throw std::logic_error("from_yaml: expected mapping for map");
            using V = map_value_t<T>;
            out.clear();
            for (auto it = n.begin(); it != n.end(); ++it) {
                V vv = construct_default<V>();
                value_from_yaml(it->second, vv);
                out.insert_or_assign(it->first.as<std::string>(), std::move(vv));
            }
        } else if constexpr (reflected_struct<T>) {
            if (!n.IsMap())
                throw std::logic_error("from_yaml: expected mapping for struct");
            type_def<T>{}.for_each_field(out, [&](std::string_view name, auto& value) {
                std::string key(name);
                if (n[key]) value_from_yaml(n[key], value);
            });
        } else if constexpr (std::is_enum_v<T>) {
            if (n.IsScalar()) {
                auto str = n.as<std::string>();
                // Try string-name match first
                bool found = false;
                for (auto val : magic_enum::enum_values<T>()) {
                    if (magic_enum::enum_name(val) == str) { out = val; found = true; break; }
                }
                if (!found) {
                    // Fall back to integer value
                    try {
                        out = static_cast<T>(n.as<std::int64_t>());
                    } catch (...) {
                        throw std::logic_error("from_yaml: unknown enum value '" + str + "'");
                    }
                }
            } else {
                throw std::logic_error("from_yaml: expected scalar for enum");
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (!n.IsScalar())
                throw std::logic_error("from_yaml: expected scalar for string");
            out = n.as<std::string>();
        } else if constexpr (std::is_same_v<T, bool>) {
            if (!n.IsScalar())
                throw std::logic_error("from_yaml: expected scalar for bool");
            out = n.as<bool>();
        } else if constexpr (std::is_integral_v<T>) {
            if (!n.IsScalar())
                throw std::logic_error("from_yaml: expected scalar for integer");
            out = n.as<T>();
        } else if constexpr (std::is_floating_point_v<T>) {
            if (!n.IsScalar())
                throw std::logic_error("from_yaml: expected scalar for floating point");
            out = n.as<T>();
        } else {
            static_assert(!is_non_string_keyed_map_v<T>,
                "def_type: map keys must be std::string for serialization. "
                "YAML mapping keys (like JSON object keys / TOML table keys) "
                "are strings; non-string-keyed maps have no representation.");
            out = n.as<T>();
        }
    }

}  // namespace detail

// ── Public API ──────────────────────────────────────────────────────────
//
// YAML support is data-only. Unlike toml11, yaml-cpp's parser discards
// comments at the source-text level — there's no AST representation of
// them to round-trip — so we don't carry a yaml_doc<T> wrapper. If your
// use case needs comment-preserving config editing, TOML is the format.

// from_yaml — parse a YAML string into a typed struct
template <detail::reflected_struct T>
T from_yaml(const std::string& yaml_str) {
    YAML::Node n = YAML::Load(yaml_str);
    T result{};
    detail::value_from_yaml(n, result);
    return result;
}

// to_yaml — struct → YAML::Node
template <detail::reflected_struct T>
YAML::Node to_yaml(const T& obj) {
    return detail::value_to_yaml(obj);
}

// to_yaml_string — struct → YAML string
template <detail::reflected_struct T>
std::string to_yaml_string(const T& obj) {
    return YAML::Dump(detail::value_to_yaml(obj));
}

}  // namespace def_type
