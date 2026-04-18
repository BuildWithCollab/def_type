#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
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
// Automatically discovers field<> and meta<> members of T via PFR or
// the struct_info<T>() registry. Provides runtime access to field values,
// type-level metadata, and schema information.
//
// Usage:
//   type_def<Dog> dog_t;
//   dog_t.name();                     // "Dog"
//   dog_t.field_count();              // 3
//   dog_t.has_meta<endpoint_info>();  // true
//   dog_t.meta<endpoint_info>().path; // "/dogs"
//   dog_t.for_each_field(rex, [](std::string_view name, auto& value) { ... });

template <typename T = detail::dynamic_tag, typename... Regs>
class type_def {
    static constexpr auto total_members_ = def_type::detail::dispatch_field_count<T>();
    using indices_ = std::make_index_sequence<total_members_>;
    using field_indices_ = decltype(detail::make_filtered_sequence<T, detail::is_field_at>(indices_{}));

    // Typed hybrid registrations — each Reg is a typed_field_reg<T, MemT>
    // preserving the real member type for real typed references in for_each.
    std::tuple<Regs...> typed_regs_;

    // Allow other type_def instantiations to access private constructor
    template <typename U, typename... Rs>
    friend class type_def;

    // Private constructor for builder chain
    explicit type_def(std::tuple<Regs...> regs) : typed_regs_(std::move(regs)) {}

public:
    type_def() = default;

    // ── Type name ────────────────────────────────────────────────────

    constexpr std::string_view name() const {
        return def_type::detail::type_name<T>();
    }

    // ── Field queries ────────────────────────────────────────────────

    std::size_t field_count() const {
        return def_type::detail::field_count<T>() + sizeof...(Regs);
    }

    std::vector<std::string> field_names() const {
        auto discovered = def_type::detail::field_names<T>();
        std::vector<std::string> result(discovered.begin(), discovered.end());
        std::apply([&](const auto&... regs) {
            (result.push_back(regs.name), ...);
        }, typed_regs_);
        return result;
    }

    // ── Field builder (hybrid registration) ──────────────────────────
    //
    // Returns a NEW type_def with the member type preserved in the
    // template parameters. Each .field() call produces a new type.

    template <typename MemT, typename... Withs>
    auto field(MemT T::* member, std::string_view fname, Withs... withs) {
        auto new_reg = detail::make_typed_reg<T>(member, fname, withs...);
        auto new_tuple = std::tuple_cat(
            std::move(typed_regs_),
            std::make_tuple(std::move(new_reg)));
        return type_def<T, Regs..., detail::typed_field_reg<T, MemT>>(
            std::move(new_tuple));
    }

    // ── Field queries by name ────────────────────────────────────────

    bool has_field(std::string_view fname) const {
        auto discovered = def_type::detail::field_names<T>();
        for (auto& n : discovered)
            if (n == fname) return true;
        bool found = false;
        std::apply([&](const auto&... regs) {
            ((regs.name == fname && (found = true, true)), ...);
        }, typed_regs_);
        return found;
    }

    field_def field(std::string_view fname) const {
        // Check hybrid registered fields
        thread_local detail::dynamic_field_def temp;
        bool found = false;
        std::apply([&](const auto&... regs) {
            ((regs.name == fname && !found && (
                temp.name = regs.name,
                temp.type = typeid(std::remove_reference_t<
                    decltype(std::declval<T>().*(regs.member))>),
                temp.has_default = false,
                temp.metas = regs.metas,
                found = true, true)), ...);
        }, typed_regs_);
        if (found) return field_def(&temp);
        // Check auto-discovered field<> members
        thread_local detail::dynamic_field_def discovered;
        found = false;
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
    // Auto-discovered field<> members give typed references.
    // Hybrid-registered fields also give real typed references.

    template <typename F>
    void for_each_field(T& obj, F&& fn) const {
        detail::for_each_field_value(obj, std::forward<F>(fn), indices_{});
        std::apply([&](const auto&... regs) {
            (fn(std::string_view(regs.name), obj.*(regs.member)), ...);
        }, typed_regs_);
    }

    template <typename F>
    void for_each_field(const T& obj, F&& fn) const {
        detail::for_each_field_value(obj, std::forward<F>(fn), indices_{});
        std::apply([&](const auto&... regs) {
            (fn(std::string_view(regs.name), obj.*(regs.member)), ...);
        }, typed_regs_);
    }

    // ── Schema-only field iteration ──────────────────────────────────
    //
    // Auto-discovered field<> members: callback receives field_descriptor<T, I>.
    // Hybrid-registered fields: callback receives field_def.
    // Both expose .name() and .has_meta<M>() / .meta<M>().

    template <typename F>
    void for_each_field(F&& fn) const {
        detail::for_each_field_descriptor<T>(std::forward<F>(fn), indices_{});
        std::apply([&](const auto&... regs) {
            ((void)[&] {
                detail::dynamic_field_def temp{
                    regs.name,
                    typeid(std::remove_reference_t<
                        decltype(std::declval<T>().*(regs.member))>),
                    {}, false, regs.metas, {}, {}};
                fn(field_def(&temp));
            }(), ...);
        }, typed_regs_);
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
        bool found = false;
        std::apply([&](const auto&... regs) {
            ((regs.name == fname && !found &&
                (fn(std::string_view(regs.name), obj.*(regs.member)),
                 found = true, true)), ...);
        }, typed_regs_);
        if (!found)
            throw std::logic_error(
                "type_def '" + std::string(name()) + "': no field named '" +
                std::string(fname) + "'");
    }

    template <typename F>
    void get(const T& obj, std::string_view fname, F&& fn) const {
        if (!has_field(fname)) {
            bool is_hybrid = false;
            std::apply([&](const auto&... regs) {
                ((regs.name == fname && (is_hybrid = true, true)), ...);
            }, typed_regs_);
            if (!is_hybrid)
                throw std::logic_error(
                    "type_def '" + std::string(name()) + "': no field named '" +
                    std::string(fname) + "'");
        }
        if (detail::get_field_by_name(obj, fname, std::forward<F>(fn), field_indices_{}))
            return;
        bool found = false;
        std::apply([&](const auto&... regs) {
            ((regs.name == fname && !found &&
                (fn(std::string_view(regs.name), obj.*(regs.member)),
                 found = true, true)), ...);
        }, typed_regs_);
        if (!found)
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
        bool name_found = field_found;
        std::apply([&](const auto&... regs) {
            ((regs.name == fname && !result.has_value() && [&] {
                name_found = true;
                using MemT = std::remove_reference_t<
                    decltype(std::declval<T>().*(regs.member))>;
                if constexpr (std::is_same_v<MemT, V>)
                    result = obj.*(regs.member);
                return true;
            }()), ...);
        }, typed_regs_);
        if (result) return *result;
        if (name_found)
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
        std::apply([&](const auto&... regs) {
            ((regs.name == fname && !set_ok && [&] {
                name_matched = true;
                using MemT = std::remove_reference_t<
                    decltype(std::declval<T>().*(regs.member))>;
                if constexpr (std::is_constructible_v<MemT, V>) {
                    obj.*(regs.member) = MemT(std::forward<V>(val));
                    set_ok = true;
                }
                return true;
            }()), ...);
        }, typed_regs_);
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
        // Check auto-discovered field<T> members
        detail::for_each_raw_field(obj, [&](std::string_view name, const auto& raw_field) {
            if (!is_valid) return;
            if (raw_field.validators) {
                auto errors = raw_field.validators(raw_field.value, name);
                if (!errors.empty()) { is_valid = false; return; }
            }
            // Recurse into nested reflected structs
            using InnerType = std::remove_cvref_t<decltype(raw_field.value)>;
            if constexpr (detail::reflected_struct<InnerType>) {
                if (!type_def<InnerType>{}.valid(raw_field.value))
                    is_valid = false;
            }
        }, indices_{});
        if (!is_valid) return false;
        // Check hybrid-registered members
        std::apply([&](const auto&... regs) {
            (([&] {
                if (!is_valid) return;
                if (regs.validate_fn) {
                    auto errors = regs.validate_fn(obj.*(regs.member), regs.name);
                    if (!errors.empty()) is_valid = false;
                }
            }()), ...);
        }, typed_regs_);
        return is_valid;
    }

    validation_result validate(const T& obj) const {
        return validate_with_prefix(obj, "");
    }

  private:
    validation_result validate_with_prefix(const T& obj, std::string_view prefix) const {
        validation_result result;
        // Check auto-discovered field<T> members
        detail::for_each_raw_field(obj, [&](std::string_view name, const auto& raw_field) {
            auto full_path = prefix.empty()
                ? std::string(name)
                : std::string(prefix) + "." + std::string(name);

            if (raw_field.validators) {
                auto errors = raw_field.validators(raw_field.value, full_path);
                for (auto& error : errors)
                    result.add(std::move(error));
            }
            // Recurse into nested reflected structs
            using InnerType = std::remove_cvref_t<decltype(raw_field.value)>;
            if constexpr (detail::reflected_struct<InnerType>) {
                auto nested_result = type_def<InnerType>{}.validate_with_prefix(
                    raw_field.value, full_path);
                for (auto& error : nested_result.errors())
                    result.add(validation_error{error.path, error.message, error.constraint});
            }
        }, indices_{});
        // Check hybrid-registered members
        std::apply([&](const auto&... regs) {
            (([&] {
                auto full_path = prefix.empty()
                    ? regs.name
                    : std::string(prefix) + "." + regs.name;

                if (regs.validate_fn) {
                    auto errors = regs.validate_fn(obj.*(regs.member), full_path);
                    for (auto& error : errors)
                        result.add(std::move(error));
                }
            }()), ...);
        }, typed_regs_);
        return result;
    }

  public:

    // ── Create instance ──────────────────────────────────────────────

    T create() const { return T{}; }

    // ── Parse JSON with reporting ───────────────────────────────────
    //
    // Declared here, defined in detail/dynamic_json.hpp (after json.hpp)
    // so parse can use value_from_json for complex types (enums,
    // vectors/maps/optionals of reflected structs).

    inline parse_result<T> parse(const nlohmann::json& j) const;
    inline parse_result<T> parse(const nlohmann::json& j, parse_options options) const;
};

}  // namespace def_type
