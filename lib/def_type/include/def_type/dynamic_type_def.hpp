#pragma once

#include <any>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <vector>

#include <nlohmann/json.hpp>

#include <def_type/field.hpp>
#include <def_type/validation.hpp>
#include <def_type/parse.hpp>
#include <def_type/detail/discovery.hpp>
#include <def_type/field_def.hpp>
#include <def_type/type_definition.hpp>

namespace def_type {

// Forward declaration for type_instance (defined in type_instance.hpp)
class type_instance;

// ── type_def<detail::dynamic_tag> — the non-templated dynamic type_def ───────────

template <>
class type_def<detail::dynamic_tag> {
    friend class type_instance;

    std::string                           name_;
    std::vector<detail::dynamic_field_def> fields_;
    std::vector<detail::meta_entry>        type_metas_;

public:
    explicit type_def(std::string_view name) : name_(name) {}

    // ── Schema queries ───────────────────────────────────────────────

    std::string_view name() const { return name_; }

    std::size_t field_count() const { return fields_.size(); }

    std::vector<std::string> field_names() const {
        std::vector<std::string> result;
        result.reserve(fields_.size());
        for (auto& f : fields_) result.push_back(f.name);
        return result;
    }

    // ── Field builder ────────────────────────────────────────────────

    template <typename V>
    type_def& field(std::string_view fname) {
        auto setter = [](std::any& target, std::any&& incoming) -> bool {
            if (auto* p = std::any_cast<V>(&incoming)) {
                target = std::move(*p);
                return true;
            }
            return false;
        };
        auto factory = []() -> std::any { return std::any(V{}); };
        auto json_init = [](const detail::dynamic_field_def& fd) {
            detail::init_json_codec<V>(fd);
        };
        fields_.push_back({std::string(fname), typeid(V), {}, false, {},
                           std::move(setter), std::move(factory),
                           std::move(json_init), {}, {}});
        return *this;
    }

    // .field<V>("name", validators(...)) or .field<V>("name", with<>(...))
    // No default value — just name + validators/metas.
    template <typename V, typename First, typename... Rest>
        requires (detail::is_validator_pack_v<std::remove_cvref_t<First>>
              || detail::is_with_v<std::remove_cvref_t<First>>)
    type_def& field(std::string_view fname, First first, Rest... rest) {
        auto setter = [](std::any& target, std::any&& incoming) -> bool {
            if (auto* p = std::any_cast<V>(&incoming)) {
                target = std::move(*p);
                return true;
            }
            return false;
        };
        auto factory = []() -> std::any { return std::any(V{}); };
        auto json_init = [](const detail::dynamic_field_def& fd) {
            detail::init_json_codec<V>(fd);
        };
        detail::dynamic_field_def fd{
            std::string(fname), typeid(V),
            {}, false, {},
            std::move(setter), std::move(factory),
            std::move(json_init), {}, {}};
        detail::process_field_arg<V>(fd, first);
        (detail::process_field_arg<V>(fd, rest), ...);
        fields_.push_back(std::move(fd));
        return *this;
    }

    template <typename V, typename... Withs>
    type_def& field(std::string_view fname, V default_value, Withs... withs) {
        auto setter = [](std::any& target, std::any&& incoming) -> bool {
            if (auto* p = std::any_cast<V>(&incoming)) {
                target = std::move(*p);
                return true;
            }
            return false;
        };
        auto factory = []() -> std::any { return std::any(V{}); };
        auto json_init = [](const detail::dynamic_field_def& fd) {
            detail::init_json_codec<V>(fd);
        };
        detail::dynamic_field_def fd{
            std::string(fname), typeid(V),
            std::any(std::move(default_value)), true, {},
            std::move(setter), std::move(factory),
            std::move(json_init), {}, {}};
        (detail::process_field_arg<V>(fd, withs), ...);
        fields_.push_back(std::move(fd));
        return *this;
    }

    // ── Nested type_def field builder ───────────────────────────────
    //
    // .field("address", address_type) — nests a dynamic type_def.
    // The field stores type_instance values backed by nested_type.

    inline type_def& field(std::string_view fname,
                    const type_def& nested_type);

    // ── Meta builder (type-level) ────────────────────────────────────

    template <typename M>
    type_def& meta(M value) {
        type_metas_.push_back({typeid(M), std::any(std::move(value))});
        return *this;
    }

    // ── Field queries ────────────────────────────────────────────────

    bool has_field(std::string_view fname) const {
        for (auto& f : fields_)
            if (f.name == fname) return true;
        return false;
    }

    field_def field(std::string_view fname) const {
        for (auto& f : fields_)
            if (f.name == fname) return field_def(&f);
        throw std::logic_error(
            "type_def '" + std::string(name_) + "': no field named '" +
            std::string(fname) + "'");
    }

    // ── Field iteration (schema-only) ────────────────────────────────

    template <typename F>
    void for_each_field(F&& fn) const {
        for (auto& f : fields_)
            fn(field_def(&f));
    }

    // ── Type-level meta queries ──────────────────────────────────────

    template <typename M>
    bool has_meta() const {
        for (auto& e : type_metas_)
            if (e.type == typeid(M)) return true;
        return false;
    }

    template <typename M>
    M meta() const {
        for (auto& e : type_metas_)
            if (e.type == typeid(M))
                return *std::any_cast<M>(&e.value);
        throw std::logic_error(
            "type_def '" + std::string(name_) + "': no meta of requested type");
    }

    template <typename M>
    std::size_t meta_count() const {
        std::size_t n = 0;
        for (auto& e : type_metas_)
            if (e.type == typeid(M)) ++n;
        return n;
    }

    template <typename M>
    std::vector<M> metas() const {
        std::vector<M> result;
        for (auto& e : type_metas_)
            if (e.type == typeid(M))
                result.push_back(*std::any_cast<M>(&e.value));
        return result;
    }

    // ── Meta iteration ─────────────────────────────────────────────

    template <typename F>
    void for_each_meta(F&& fn) const {
        for (auto& e : type_metas_)
            fn(metadata(&e.value, e.type));
    }

    // ── Create instance ─────────────────────────────────────────────

    type_instance create() const;

    // ── Create instance from JSON ────────────────────────────────────

    inline type_instance create(const nlohmann::json& j) const;

    // ── Parse JSON with reporting ───────────────────────────────────

    inline parse_result<type_instance> parse(const nlohmann::json& j) const;
    inline parse_result<type_instance> parse(const nlohmann::json& j, parse_options options) const;
};

// ── CTAD: type_def("Event") deduces to type_def<detail::dynamic_tag> ─────────────

type_def(const char*) -> type_def<detail::dynamic_tag>;
type_def(std::string_view) -> type_def<detail::dynamic_tag>;

}  // namespace def_type
