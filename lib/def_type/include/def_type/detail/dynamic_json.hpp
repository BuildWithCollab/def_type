#pragma once

#include <any>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <def_type/field.hpp>
#include <def_type/parse.hpp>
#include <def_type/detail/discovery.hpp>
#include <def_type/typed_type_def.hpp>
#include <def_type/dynamic_type_def.hpp>
#include <def_type/type_instance.hpp>
#include <def_type/json.hpp>

namespace def_type {

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

// ═══════════════════════════════════════════════════════════════════════
// type_def<T>::parse() — defined here so it can use value_from_json
// for complex types (enums, vectors/maps/optionals of reflected structs)
// ═══════════════════════════════════════════════════════════════════════

template <typename T, typename... Regs>
parse_result<T> type_def<T, Regs...>::parse(const nlohmann::json& j) const {
    if (!j.is_object()) throw std::logic_error("parse: expected JSON object");

    parse_result<T> result{.value = T{}};

    // Populate using raw field iteration
    detail::for_each_raw_field(result.value, [&](std::string_view name, auto& raw_field) {
        std::string key(name);
        if (j.contains(key)) {
            using InnerType = std::remove_cvref_t<decltype(raw_field.value)>;
            if constexpr (detail::reflected_struct<InnerType>) {
                // Nested struct — recurse via parse so each inner field
                // gets its own try-catch (graceful per-field defaults)
                if (j[key].is_object()) {
                    auto nested_result = type_def<InnerType>{}.parse(j[key]);
                    raw_field.value = std::move(nested_result.value);
                }
            } else {
                try {
                    detail::value_from_json(j[key], raw_field.value);
                } catch (...) {
                    // Type mismatch — keep default
                }
            }
        }
    }, indices_{});

    // Also populate hybrid-registered fields via value_from_json
    std::apply([&](const auto&... regs) {
        (([&] {
            if (j.contains(regs.name)) {
                try {
                    detail::value_from_json(j[regs.name], result.value.*(regs.member));
                } catch (...) {}
            }
        }()), ...);
    }, typed_regs_);

    // Extra keys
    auto schema_names_vec = field_names();
    for (auto& [key, val] : j.items()) {
        bool found = false;
        for (auto& schema_name : schema_names_vec)
            if (key == schema_name) { found = true; break; }
        if (!found)
            result.extra_keys_.push_back(key);
    }

    // Missing fields
    for (auto& schema_name : schema_names_vec) {
        if (!j.contains(schema_name))
            result.missing_fields_.push_back(schema_name);
    }

    // Validation
    auto validation_check = validate(result.value);
    for (auto& error : validation_check.errors())
        result.validation_errors_.push_back(
            validation_error{error.path, error.message, error.constraint});

    return result;
}

template <typename T, typename... Regs>
parse_result<T> type_def<T, Regs...>::parse(
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

}  // namespace def_type
