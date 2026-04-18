module;

#include <nlohmann/json.hpp>

#include <ankerl/unordered_dense.h>

#include <magic_enum/magic_enum.hpp>

#include <any>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

export module collab.core:field_json;

import :field;
import :field_reflect;
import :meta;
import :type_def;

export namespace collab::model {

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
// Definition of the template forward-declared in :type_def.
// Called lazily from field_json.cpp the first time to_json()/load_json()
// is invoked — NOT at .field<V>() time — so VS 2022 never triggers
// json template instantiation from user code.

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




}  // namespace collab::model
