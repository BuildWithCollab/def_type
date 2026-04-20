#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include <def_type/field.hpp>
#include <def_type/meta.hpp>
#include <def_type/reflect.hpp>
#include <def_type/validation.hpp>
#include <def_type/parse.hpp>
#include <def_type/detail/discovery.hpp>
#include <def_type/field_def.hpp>
#include <def_type/type_definition.hpp>

namespace def_type {

// ── type_def<T> — typed runtime schema with auto-discovery ───────────────
//
// Automatically discovers all non-meta<> members of T via PFR or
// the struct_info<T>() registry. field<> members get full support
// (validators, with<> metas). Plain members are reflected as-is.
//
// Usage:
//   type_def<Dog> dog_t;
//   dog_t.name();                     // "Dog"
//   dog_t.field_count();              // 3
//   dog_t.has_meta<endpoint_info>();  // true
//   dog_t.meta<endpoint_info>().path; // "/dogs"
//   dog_t.for_each_field(rex, [](std::string_view name, auto& value) { ... });

template <typename T = detail::dynamic_tag>
class type_def {
    static constexpr auto total_members_ = def_type::detail::dispatch_field_count<T>();
    using indices_ = std::make_index_sequence<total_members_>;
    using field_indices_ = decltype(detail::make_filtered_sequence<T, detail::is_field_at>(indices_{}));

    // Allow other type_def instantiations to access validate_with_prefix
    template <typename U>
    friend class type_def;

public:
    type_def() = default;

    // ── Type name ────────────────────────────────────────────────────

    constexpr std::string_view name() const {
        return def_type::detail::type_name<T>();
    }

    // ── Field queries ────────────────────────────────────────────────

    std::size_t field_count() const {
        return def_type::detail::field_count<T>();
    }

    std::vector<std::string> field_names() const {
        auto discovered = def_type::detail::field_names<T>();
        std::vector<std::string> result(discovered.begin(), discovered.end());
        return result;
    }

    // ── Field queries by name ────────────────────────────────────────

    bool has_field(std::string_view fname) const {
        auto discovered = def_type::detail::field_names<T>();
        for (auto& n : discovered)
            if (n == fname) return true;
        return false;
    }

    field_def field(std::string_view fname) const {
        thread_local detail::dynamic_field_def discovered;
        bool found = false;
        detail::build_discovered_field_def<T>(
            discovered, fname, found, indices_{});
        if (found) return field_def(&discovered);
        throw std::logic_error(
            "type_def '" + std::string(name()) + "': no field named '" +
            std::string(fname) + "'");
    }

    // ── Meta queries ─────────────────────────────────────────────────

    template <typename M>
    constexpr bool has_meta() const {
        return detail::count_meta_of<T, M>() > 0;
    }

    template <typename M>
    M meta() const {
        const T instance{};
        return detail::extract_first_meta<const T, M>(instance, indices_{});
    }

    template <typename M>
    constexpr std::size_t meta_count() const {
        return detail::count_meta_of<T, M>();
    }

    template <typename M>
    std::vector<M> metas() const {
        const T instance{};
        return detail::extract_all_metas<const T, M>(instance, indices_{});
    }

    // ── Instance field iteration ────────────────────────────────────
    //
    // Callback signature: fn(std::string_view name, auto& value)
    // field<> members give the unwrapped value. Plain members give direct references.

    template <typename F>
    void for_each_field(T& obj, F&& fn) const {
        detail::for_each_field_value(obj, std::forward<F>(fn), indices_{});
    }

    template <typename F>
    void for_each_field(const T& obj, F&& fn) const {
        detail::for_each_field_value(obj, std::forward<F>(fn), indices_{});
    }

    // ── Schema-only field iteration ──────────────────────────────────

    template <typename F>
    void for_each_field(F&& fn) const {
        detail::for_each_field_descriptor<T>(std::forward<F>(fn), indices_{});
    }

    // ── Meta iteration ───────────────────────────────────────────────

    template <typename F>
    void for_each_meta(F&& fn) const {
        const T instance{};
        detail::for_each_meta_value(instance, std::forward<F>(fn), indices_{});
    }

    template <typename F>
    constexpr void for_each_meta(const T& obj, F&& fn) const {
        detail::for_each_meta_value(obj, std::forward<F>(fn), indices_{});
    }

    // ── Get field by runtime name (callback) ────────────────────────

    template <typename F>
    void get(T& obj, std::string_view fname, F&& fn) const {
        if (detail::get_field_by_name(obj, fname, std::forward<F>(fn), field_indices_{}))
            return;
        throw std::logic_error(
            "type_def '" + std::string(name()) + "': no field named '" +
            std::string(fname) + "'");
    }

    template <typename F>
    void get(const T& obj, std::string_view fname, F&& fn) const {
        if (detail::get_field_by_name(obj, fname, std::forward<F>(fn), field_indices_{}))
            return;
        throw std::logic_error(
            "type_def '" + std::string(name()) + "': no field named '" +
            std::string(fname) + "'");
    }

    // ── Get field by runtime name (typed) ────────────────────────────

    template <typename V>
    V get(const T& obj, std::string_view fname) const {
        std::optional<V> result;
        bool field_found = detail::get_field_by_name(obj, fname,
            [&](std::string_view, const auto& value) {
                if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, V>)
                    result = value;
            }, field_indices_{});
        if (result) return *result;
        if (field_found)
            throw std::logic_error(
                "type_def '" + std::string(name()) + "': field '" +
                std::string(fname) + "' type mismatch");
        throw std::logic_error(
            "type_def '" + std::string(name()) + "': no field named '" +
            std::string(fname) + "'");
    }

    // ── Set field by runtime name ────────────────────────────────────

    template <typename V>
    void set(T& obj, std::string_view fname, V&& val) const {
        bool name_matched = false;
        bool set_ok = false;
        detail::set_field_by_name(
            obj, fname, std::forward<V>(val), field_indices_{},
            name_matched, set_ok);
        if (set_ok) return;
        if (name_matched)
            throw std::logic_error(
                "type_def '" + std::string(name()) + "': field '" +
                std::string(fname) + "' type mismatch");
        throw std::logic_error(
            "type_def '" + std::string(name()) + "': no field named '" +
            std::string(fname) + "'");
    }

    // ── Validation ────────────────────────────────────────────────────
    //
    // valid(instance) — fast bool, short-circuits on first failure.
    // validate(instance) — collects all errors.

    bool valid(const T& obj) const {
        bool is_valid = true;
        detail::for_each_raw_field(obj, [&](std::string_view name, const auto& raw_member) {
            if (!is_valid) return;
            using RawT = std::remove_cvref_t<decltype(raw_member)>;
            if constexpr (is_field<RawT>) {
                if (raw_member.validators) {
                    auto errors = raw_member.validators(raw_member.value, name);
                    if (!errors.empty()) { is_valid = false; return; }
                }
                // Recurse into nested reflected structs
                using InnerType = std::remove_cvref_t<decltype(raw_member.value)>;
                if constexpr (detail::reflected_struct<InnerType>) {
                    if (!type_def<InnerType>{}.valid(raw_member.value))
                        is_valid = false;
                }
            } else {
                // Plain member — recurse if it's a reflected struct
                if constexpr (detail::reflected_struct<RawT>) {
                    if (!type_def<RawT>{}.valid(raw_member))
                        is_valid = false;
                }
            }
        }, indices_{});
        return is_valid;
    }

    validation_result validate(const T& obj) const {
        return validate_with_prefix(obj, "");
    }

  private:
    validation_result validate_with_prefix(const T& obj, std::string_view prefix) const {
        validation_result result;
        detail::for_each_raw_field(obj, [&](std::string_view name, const auto& raw_member) {
            auto full_path = prefix.empty()
                ? std::string(name)
                : std::string(prefix) + "." + std::string(name);

            using RawT = std::remove_cvref_t<decltype(raw_member)>;
            if constexpr (is_field<RawT>) {
                if (raw_member.validators) {
                    auto errors = raw_member.validators(raw_member.value, full_path);
                    for (auto& error : errors)
                        result.add(std::move(error));
                }
                // Recurse into nested reflected structs
                using InnerType = std::remove_cvref_t<decltype(raw_member.value)>;
                if constexpr (detail::reflected_struct<InnerType>) {
                    auto nested_result = type_def<InnerType>{}.validate_with_prefix(
                        raw_member.value, full_path);
                    for (auto& error : nested_result.errors())
                        result.add(validation_error{error.path, error.message, error.constraint});
                }
            } else {
                // Plain member — recurse if it's a reflected struct
                if constexpr (detail::reflected_struct<RawT>) {
                    auto nested_result = type_def<RawT>{}.validate_with_prefix(
                        raw_member, full_path);
                    for (auto& error : nested_result.errors())
                        result.add(validation_error{error.path, error.message, error.constraint});
                }
            }
        }, indices_{});
        return result;
    }

  public:

    // ── Create instance ──────────────────────────────────────────────

    T create() const { return T{}; }

    // ── Parse JSON with reporting ───────────────────────────────────

    inline parse_result<T> parse(const nlohmann::json& j) const;
    inline parse_result<T> parse(const nlohmann::json& j, parse_options options) const;
};

}  // namespace def_type
