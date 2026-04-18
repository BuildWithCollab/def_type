#pragma once

#include <any>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <vector>

#include <nlohmann/json.hpp>

#include <def_type/field.hpp>
#include <def_type/parse.hpp>
#include <def_type/field_def.hpp>
#include <def_type/dynamic_type_def.hpp>

namespace def_type {

// ═══════════════════════════════════════════════════════════════════════
// type_instance — instance of a dynamic type_def
// ═══════════════════════════════════════════════════════════════════════

class type_instance {
    friend class type_def<detail::dynamic_tag>;

    const type_def<detail::dynamic_tag>* type_;
    std::vector<std::any>        values_;

    int find_field_index(std::string_view name) const {
        auto& fields = type_->fields_;
        for (int i = 0; i < static_cast<int>(fields.size()); ++i)
            if (fields[i].name == name) return i;
        return -1;
    }

public:
    explicit type_instance(const type_def<detail::dynamic_tag>& t) : type_(&t) {
        values_.reserve(t.fields_.size());
        for (auto& fd : t.fields_) {
            if (fd.has_default)
                values_.push_back(fd.default_value);
            else
                values_.push_back(fd.make_default());
        }
    }

    // ── Set ──────────────────────────────────────────────────────────

    template <typename V>
    void set(std::string_view name, V&& value) {
        int idx = find_field_index(name);
        if (idx < 0)
            throw std::logic_error(
                "type_instance (type '" + std::string(type_->name_) +
                "'): no field named '" + std::string(name) + "'");
        auto& fd = type_->fields_[idx];
        std::any wrapped(std::forward<V>(value));
        if (fd.setter(values_[idx], std::move(wrapped))) return;
        // Fallback: try string conversion for string-like types
        if constexpr (std::is_constructible_v<std::string, V> &&
                      !std::is_same_v<std::remove_cvref_t<V>, std::string>) {
            std::any str(std::string(std::forward<V>(value)));
            if (fd.setter(values_[idx], std::move(str))) return;
        }
        throw std::logic_error(
            "type_instance (type '" + std::string(type_->name_) +
            "'): field '" + std::string(name) + "' type mismatch");
    }

    // ── Get ──────────────────────────────────────────────────────────

    template <typename V>
    V get(std::string_view name) const {
        int idx = find_field_index(name);
        if (idx < 0)
            throw std::logic_error(
                "type_instance (type '" + std::string(type_->name_) +
                "'): no field named '" + std::string(name) + "'");
        if (auto* p = std::any_cast<V>(&values_[idx]))
            return *p;
        throw std::logic_error(
            "type_instance (type '" + std::string(type_->name_) +
            "'): field '" + std::string(name) + "' type mismatch");
    }

    // ── Has ──────────────────────────────────────────────────────────

    bool has(std::string_view name) const {
        return find_field_index(name) >= 0;
    }

    // ── Type access ──────────────────────────────────────────────────

    const type_def<detail::dynamic_tag>& type() const { return *type_; }

    // ── Validation ──────────────────────────────────────────────────
    //
    // Stateless — results computed on demand, never cached.
    // valid() short-circuits on first failure. validate() collects all.

    bool valid() const {
        auto& fields = type_->fields_;
        for (std::size_t i = 0; i < fields.size(); ++i) {
            if (fields[i].validate_fn) {
                auto errors = fields[i].validate_fn(values_[i], fields[i].name);
                if (!errors.empty()) return false;
            }
            // Recurse into nested type_instances
            if (fields[i].type == typeid(type_instance)) {
                if (const auto* nested = std::any_cast<type_instance>(&values_[i])) {
                    if (!nested->valid()) return false;
                }
            }
        }
        return true;
    }

    validation_result validate() const {
        return validate_with_prefix("");
    }

  private:
    validation_result validate_with_prefix(std::string_view prefix) const {
        validation_result result;
        auto& fields = type_->fields_;
        for (std::size_t i = 0; i < fields.size(); ++i) {
            auto full_path = prefix.empty()
                ? fields[i].name
                : std::string(prefix) + "." + fields[i].name;

            if (fields[i].validate_fn) {
                auto errors = fields[i].validate_fn(values_[i], full_path);
                for (auto& error : errors)
                    result.add(std::move(error));
            }
            // Recurse into nested type_instances
            if (fields[i].type == typeid(type_instance)) {
                if (const auto* nested = std::any_cast<type_instance>(&values_[i])) {
                    auto nested_result = nested->validate_with_prefix(full_path);
                    for (auto& error : nested_result.errors())
                        result.add(validation_error{error.path, error.message, error.constraint});
                }
            }
        }
        return result;
    }

  public:

    // ── Field iteration ─────────────────────────────────────────────
    //
    // Callback receives (std::string_view name, const_field_value value)
    // or (std::string_view name, field_value value).
    // Access typed values via value.as<T>() or value.try_as<T>().

    template <typename F>
    void for_each_field(F&& fn) const {
        auto& fields = type_->fields_;
        for (std::size_t i = 0; i < fields.size(); ++i)
            fn(std::string_view(fields[i].name),
               const_field_value(&values_[i]));
    }

    template <typename F>
    void for_each_field(F&& fn) {
        auto& fields = type_->fields_;
        for (std::size_t i = 0; i < fields.size(); ++i)
            fn(std::string_view(fields[i].name),
               field_value(&values_[i]));
    }

    // ── JSON deserialization ─────────────────────────────────────────
    //
    // Overlay semantics: missing keys keep their current/default values.
    // Extra JSON keys are silently ignored. Type mismatches throw.

    inline void load_json(const nlohmann::json& j);

    // ── JSON serialization ───────────────────────────────────────────

    inline nlohmann::json to_json() const;
    inline std::string to_json_string(int indent = -1) const;
};

// ── type_def<detail::dynamic_tag>::create() ──────────────────────────────────────

inline type_instance type_def<detail::dynamic_tag>::create() const {
    return type_instance(*this);
}

// ═══════════════════════════════════════════════════════════════════════
// field<type_def<>> — nests a runtime-defined type inside a struct
// ═══════════════════════════════════════════════════════════════════════

template <typename WithPack>
struct field<type_def<detail::dynamic_tag>, WithPack> {
    using value_type = type_instance;

    WithPack      with{};
    type_instance value;

    // Construct from a type_def — creates a default instance
    field(const type_def<detail::dynamic_tag>& typedef_schema)
        : value(typedef_schema.create()) {}

    // Construct with extensions + type_def
    field(WithPack with_pack, const type_def<detail::dynamic_tag>& typedef_schema)
        : with(std::move(with_pack)), value(typedef_schema.create()) {}

    // Access the instance
    type_instance* operator->() { return &value; }
    const type_instance* operator->() const { return &value; }

    operator type_instance&() { return value; }
    operator const type_instance&() const { return value; }
};

}  // namespace def_type
