#pragma once

#include <nlohmann/json.hpp>

#include <ankerl/unordered_dense.h>

#include <magic_enum/magic_enum.hpp>

#include <any>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <def_type/field.hpp>
#include <def_type/field_reflect.hpp>
#include <def_type/meta.hpp>
#include <def_type/type_def.hpp>

namespace def_type {

// ── Shared type traits ──────────────────────────────────────────────────

namespace detail {

    template <typename T>
    inline constexpr bool is_optional_v = false;

    template <typename T>
    inline constexpr bool is_optional_v<std::optional<T>> = true;

    template <typename T>
    inline constexpr bool is_iterable_array_v = false;

    template <typename T>
    inline constexpr bool is_iterable_array_v<std::vector<T>> = true;

    template <typename T, typename Cmp, typename Alloc>
    inline constexpr bool is_iterable_array_v<std::set<T, Cmp, Alloc>> = true;

    template <typename T, typename Hash, typename Eq, typename Alloc>
    inline constexpr bool is_iterable_array_v<std::unordered_set<T, Hash, Eq, Alloc>> = true;

    template <typename T, typename Hash, typename Eq>
    inline constexpr bool is_iterable_array_v<ankerl::unordered_dense::set<T, Hash, Eq>> = true;

    template <typename T>
    inline constexpr bool is_map_v = false;

    template <typename V, typename Cmp, typename Alloc>
    inline constexpr bool is_map_v<std::map<std::string, V, Cmp, Alloc>> = true;

    template <typename V, typename Hash, typename Eq, typename Alloc>
    inline constexpr bool is_map_v<std::unordered_map<std::string, V, Hash, Eq, Alloc>> = true;

    template <typename V, typename Hash, typename Eq>
    inline constexpr bool is_map_v<ankerl::unordered_dense::map<std::string, V, Hash, Eq>> = true;

    // ── Extract inner type from containers ──────────────────────────

    template <typename T>
    struct array_element;

    template <typename T>
    struct array_element<std::vector<T>> { using type = T; };

    template <typename T, typename Cmp, typename Alloc>
    struct array_element<std::set<T, Cmp, Alloc>> { using type = T; };

    template <typename T, typename Hash, typename Eq, typename Alloc>
    struct array_element<std::unordered_set<T, Hash, Eq, Alloc>> { using type = T; };

    template <typename T, typename Hash, typename Eq>
    struct array_element<ankerl::unordered_dense::set<T, Hash, Eq>> { using type = T; };

    template <typename T>
    using array_element_t = typename array_element<T>::type;

    template <typename T>
    struct map_value;

    template <typename V, typename Cmp, typename Alloc>
    struct map_value<std::map<std::string, V, Cmp, Alloc>> { using type = V; };

    template <typename V, typename Hash, typename Eq, typename Alloc>
    struct map_value<std::unordered_map<std::string, V, Hash, Eq, Alloc>> { using type = V; };

    template <typename V, typename Hash, typename Eq>
    struct map_value<ankerl::unordered_dense::map<std::string, V, Hash, Eq>> { using type = V; };

    template <typename T>
    using map_value_t = typename map_value<T>::type;

    // ── Optional inner type ──────────────────────────────────────────

    template <typename T>
    struct optional_inner;

    template <typename T>
    struct optional_inner<std::optional<T>> { using type = T; };

    template <typename T>
    using optional_inner_t = typename optional_inner<T>::type;

}  // namespace detail

// ── value_to_json / value_from_json ─────────────────────────────────────

namespace detail {

    template <typename T>
    nlohmann::json value_to_json(const T& v) {
        if constexpr (is_optional_v<T>) {
            if (v.has_value()) return value_to_json(*v);
            return nlohmann::json(nullptr);
        } else if constexpr (is_iterable_array_v<T>) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& elem : v) arr.push_back(value_to_json(elem));
            return arr;
        } else if constexpr (is_map_v<T>) {
            nlohmann::json obj = nlohmann::json::object();
            for (const auto& [k, val] : v) obj[k] = value_to_json(val);
            return obj;
        } else if constexpr (reflected_struct<T>) {
            nlohmann::json j = nlohmann::json::object();
            type_def<T>{}.for_each_field(v, [&](std::string_view name, const auto& value) {
                j[std::string(name)] = value_to_json(value);
            });
            return j;
        } else if constexpr (std::is_same_v<T, type_instance>) {
            return v.to_json();
        } else if constexpr (std::is_enum_v<T>) {
            std::string_view name = magic_enum::enum_name(v);
            if (name.empty())
                return nlohmann::json(static_cast<std::underlying_type_t<T>>(v));
            return nlohmann::json(std::string(name));
        } else {
            return nlohmann::json(v);
        }
    }

    template <typename T>
    void value_from_json(const nlohmann::json& j, T& out) {
        if constexpr (is_optional_v<T>) {
            using Inner = optional_inner_t<T>;
            if (j.is_null()) { out = std::nullopt; }
            else { Inner inner{}; value_from_json(j, inner); out = std::move(inner); }
        } else if constexpr (is_iterable_array_v<T>) {
            if (!j.is_array()) throw std::logic_error("from_json: expected array");
            using Elem = array_element_t<T>;
            out.clear();
            for (const auto& elem : j) {
                Elem e{}; value_from_json(elem, e);
                if constexpr (requires { out.push_back(e); }) out.push_back(std::move(e));
                else out.insert(std::move(e));
            }
        } else if constexpr (is_map_v<T>) {
            if (!j.is_object()) throw std::logic_error("from_json: expected object for map");
            using V = map_value_t<T>;
            out.clear();
            for (auto& [key, val] : j.items()) {
                V v{}; value_from_json(val, v); out[key] = std::move(v);
            }
        } else if constexpr (reflected_struct<T>) {
            if (!j.is_object()) throw std::logic_error("from_json: expected object for struct");
            type_def<T>{}.for_each_field(out, [&](std::string_view name, auto& value) {
                std::string key(name);
                if (j.contains(key)) value_from_json(j[key], value);
            });
        } else if constexpr (std::is_same_v<T, type_instance>) {
            if (!j.is_object()) throw std::logic_error("from_json: expected object for type_instance");
            out.load_json(j);
        } else if constexpr (std::is_enum_v<T>) {
            if (j.is_string()) {
                auto str = j.get<std::string>();
                bool found = false;
                for (auto val : magic_enum::enum_values<T>()) {
                    if (magic_enum::enum_name(val) == str) { out = val; found = true; break; }
                }
                if (!found) throw std::logic_error(
                    "from_json: unknown enum value '" + str + "'");
            } else if (j.is_number()) {
                out = static_cast<T>(j.get<std::underlying_type_t<T>>());
            } else {
                throw std::logic_error("from_json: expected string or number for enum");
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (!j.is_string()) throw std::logic_error("from_json: expected string");
            out = j.get<std::string>();
        } else if constexpr (std::is_same_v<T, bool>) {
            if (!j.is_boolean()) throw std::logic_error("from_json: expected boolean");
            out = j.get<bool>();
        } else if constexpr (std::is_integral_v<T>) {
            if (!j.is_number()) throw std::logic_error("from_json: expected number");
            out = j.get<T>();
        } else if constexpr (std::is_floating_point_v<T>) {
            if (!j.is_number()) throw std::logic_error("from_json: expected number");
            out = j.get<T>();
        } else {
            out = j.get<T>();
        }
    }

}  // namespace detail

// ── init_json_codec — populates to_json_fn / from_json_fn ───────────────
//
// Definition of the template forward-declared in type_def.hpp.
// Called lazily the first time to_json()/load_json() is invoked.

namespace detail {

    template <typename V>
    void init_json_codec(const dynamic_field_def& fd) {
        fd.to_json_fn = [](const std::any& a) -> std::any {
            return std::any(value_to_json(*std::any_cast<V>(&a)));
        };
        fd.from_json_fn = [](std::any& a, const std::any& j_any) {
            const auto& j = *std::any_cast<nlohmann::json>(&j_any);
            V val{}; value_from_json(j, val); a = std::move(val);
        };
    }

}  // namespace detail

// ── to_json ────────────────────────────────────────────────────────────

template <detail::reflected_struct T>
nlohmann::json to_json(const T& obj) {
    return detail::value_to_json(obj);
}

template <detail::reflected_struct T>
std::string to_json_string(const T& obj, int indent = -1) {
    auto j = detail::value_to_json(obj);
    return indent < 0 ? j.dump() : j.dump(indent);
}

// ── from_json ──────────────────────────────────────────────────────────

template <detail::reflected_struct T>
T from_json(const nlohmann::json& j) {
    T result{};
    detail::value_from_json(j, result);
    return result;
}

template <detail::reflected_struct T>
T from_json(const std::string& json_str) {
    auto j = nlohmann::json::parse(json_str);
    return from_json<T>(j);
}

// ── Hybrid to_json / from_json — plain structs with type_def ────────────
//
// For structs without field<> members, registered via the hybrid path:
//   auto t = type_def<PlainDog>().field(&PlainDog::name, "name");
//   auto j = to_json(dog, t);
//   auto dog2 = from_json<PlainDog>(j, t);

template <typename T, typename... Regs>
nlohmann::json to_json(const T& obj, const type_def<T, Regs...>& typedef_schema) {
    nlohmann::json j = nlohmann::json::object();
    typedef_schema.for_each_field(obj, [&](std::string_view name, const auto& value) {
        j[std::string(name)] = detail::value_to_json(value);
    });
    return j;
}

template <typename T, typename... Regs>
std::string to_json_string(const T& obj, const type_def<T, Regs...>& typedef_schema, int indent = -1) {
    auto j = to_json(obj, typedef_schema);
    return indent < 0 ? j.dump() : j.dump(indent);
}

template <typename T, typename... Regs>
    requires std::is_aggregate_v<T>
T from_json(const nlohmann::json& j, const type_def<T, Regs...>& typedef_schema) {
    if (!j.is_object()) throw std::logic_error("from_json: expected JSON object");
    T result{};
    typedef_schema.for_each_field(result, [&](std::string_view name, auto& value) {
        std::string key(name);
        if (j.contains(key)) detail::value_from_json(j[key], value);
    });
    return result;
}

// ═══════════════════════════════════════════════════════════════════════
// Inline definitions from field_json.cpp (merged into header)
// ═══════════════════════════════════════════════════════════════════════

namespace detail {

    inline void ensure_codec(const dynamic_field_def& fd) {
        if (fd.to_json_fn) return;
        if (fd._json_init) fd._json_init(fd);
    }

}  // namespace detail

// ── type_instance::load_json ─────────────────────────────────────────────

inline void type_instance::load_json(const nlohmann::json& j) {
    if (!j.is_object())
        throw std::logic_error("load_json: expected JSON object");
    auto& fields = type_->fields_;
    for (auto& [key, val] : j.items()) {
        int idx = find_field_index(key);
        if (idx < 0) continue;
        auto& fd = fields[idx];
        detail::ensure_codec(fd);
        fd.from_json_fn(values_[idx], std::any(val));
    }
}

// ── type_instance::to_json ───────────────────────────────────────────────

inline nlohmann::json type_instance::to_json() const {
    nlohmann::json j = nlohmann::json::object();
    auto& fields = type_->fields_;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        auto& fd = fields[i];
        detail::ensure_codec(fd);
        auto result = fd.to_json_fn(values_[i]);
        j[fd.name] = *std::any_cast<nlohmann::json>(&result);
    }
    return j;
}

// ── type_instance::to_json_string ────────────────────────────────────────

inline std::string type_instance::to_json_string(int indent) const {
    return to_json().dump(indent);
}

// ── type_def<dynamic_tag>::create(json) ──────────────────────────────────

inline type_instance
type_def<detail::dynamic_tag>::create(const nlohmann::json& j) const {
    auto obj = create();
    obj.load_json(j);
    return obj;
}

// ── type_def<dynamic_tag>::parse(json) ────────────────────────────────────

inline parse_result<type_instance>
type_def<detail::dynamic_tag>::parse(const nlohmann::json& j) const {
    if (!j.is_object())
        throw std::logic_error("parse: expected JSON object");

    parse_result<type_instance> result{.value = create()};

    // Load fields one at a time, catching type mismatches gracefully
    std::vector<std::string> json_keys;
    for (auto& [key, val] : j.items()) {
        json_keys.push_back(key);
        int idx = result.value.find_field_index(key);
        if (idx < 0) continue;
        auto& fd = fields_[idx];
        detail::ensure_codec(fd);
        try {
            fd.from_json_fn(result.value.values_[idx], std::any(val));
        } catch (...) {
            // Type mismatch — keep default value
        }
    }

    // Find extra keys (in JSON but not in schema)
    for (auto& key : json_keys) {
        if (!has_field(key))
            result.extra_keys_.push_back(key);
    }

    // Find missing fields (in schema but not in JSON)
    for (auto& field_name : field_names()) {
        bool found = false;
        for (auto& key : json_keys) {
            if (key == field_name) { found = true; break; }
        }
        if (!found)
            result.missing_fields_.push_back(field_name);
    }

    // Run validation
    auto validation = result.value.validate();
    for (auto& error : validation.errors())
        result.validation_errors_.push_back(
            validation_error{error.path, error.message, error.constraint});

    return result;
}

// ── type_def<dynamic_tag>::parse(json, options) ──────────────────────────

inline parse_result<type_instance>
type_def<detail::dynamic_tag>::parse(
        const nlohmann::json& j, parse_options options) const {
    if (options.strict) {
        options.reject_extra_keys = true;
        options.require_all_fields = true;
        options.require_valid = true;
    }

    auto result = parse(j);

    if (options.reject_extra_keys && result.has_extra_keys())
        throw parse_error("parse: extra keys in JSON",
            result.extra_keys_, result.missing_fields_, result.validation_errors_);
    if (options.require_all_fields && result.has_missing_fields())
        throw parse_error("parse: missing fields in JSON",
            result.extra_keys_, result.missing_fields_, result.validation_errors_);
    if (options.require_valid && !result.valid())
        throw parse_error("parse: validation errors",
            result.extra_keys_, result.missing_fields_, result.validation_errors_);

    return result;
}

// ── type_def<dynamic_tag>::field(name, nested_type_def) ──────────────────

inline type_def<detail::dynamic_tag>&
type_def<detail::dynamic_tag>::field(
        std::string_view fname,
        const type_def& nested_type) {
    const auto* nested_ptr = &nested_type;

    auto setter = [](std::any& target, std::any&& incoming) -> bool {
        if (auto* p = std::any_cast<type_instance>(&incoming)) {
            target = std::move(*p);
            return true;
        }
        return false;
    };
    auto factory = [nested_ptr]() -> std::any {
        return std::any(nested_ptr->create());
    };

    // JSON codec — to_json calls type_instance::to_json(),
    // from_json creates a fresh instance and calls load_json().
    auto json_init = [nested_ptr](const detail::dynamic_field_def& fd) {
        fd.to_json_fn = [](const std::any& a) -> std::any {
            const auto& instance = *std::any_cast<type_instance>(&a);
            return std::any(instance.to_json());
        };
        fd.from_json_fn = [nested_ptr](std::any& a, const std::any& j_any) {
            const auto& j = *std::any_cast<nlohmann::json>(&j_any);
            auto instance = nested_ptr->create();
            instance.load_json(j);
            a = std::move(instance);
        };
    };

    detail::dynamic_field_def fd{};
    fd.name = std::string(fname);
    fd.type = typeid(type_instance);
    fd.default_value = nested_type.create();
    fd.has_default = true;
    fd.setter = std::move(setter);
    fd.make_default = std::move(factory);
    fd._json_init = std::move(json_init);
    fields_.push_back(std::move(fd));
    return *this;
}

}  // namespace def_type
