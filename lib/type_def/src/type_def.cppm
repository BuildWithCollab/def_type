module;

#include <any>
#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

#include <nameof.hpp>
#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>
#include <ankerl/unordered_dense.h>

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

export module collab.core:type_def;

import :field;
import :field_reflect;
import :meta;

export namespace collab::model {

// ═══════════════════════════════════════════════════════════════════════
// Validation types
// ═══════════════════════════════════════════════════════════════════════
//
// validation_error and validator_pack are defined in field.cppm.
// validation_result and parse_result live here.

struct validation_result {
    explicit operator bool() const { return errors_.empty(); }
    bool ok() const { return errors_.empty(); }

    const std::vector<validation_error>& errors() const { return errors_; }
    std::size_t error_count() const { return errors_.size(); }

    auto begin() const { return errors_.begin(); }
    auto end() const { return errors_.end(); }

    // Builder — used internally by validate()
    void add(validation_error error) { errors_.push_back(std::move(error)); }

private:
    std::vector<validation_error> errors_;
};

// ═══════════════════════════════════════════════════════════════════════
// parse_result — deserialization result with reporting
// ═══════════════════════════════════════════════════════════════════════
//
// Always contains the object AND the report. Not std::expected.
// extra_keys and missing_fields are informational — only validation
// errors affect valid().

template <typename T>
struct parse_result {
    T value;

    // --- Error checks ---
    bool valid() const { return validation_errors_.empty(); }
    bool has_extra_keys() const { return !extra_keys_.empty(); }
    bool has_missing_fields() const { return !missing_fields_.empty(); }

    // --- Data access ---
    const std::vector<std::string>& extra_keys() const { return extra_keys_; }
    const std::vector<std::string>& missing_fields() const { return missing_fields_; }
    const std::vector<validation_error>& validation_errors() const { return validation_errors_; }

    // --- Value access ---
    T& operator*() { return value; }
    const T& operator*() const { return value; }
    T* operator->() { return &value; }
    const T* operator->() const { return &value; }

    T& checked_value() {
        if (!valid())
            throw std::logic_error("parse_result: validation errors exist");
        return value;
    }
    const T& checked_value() const {
        if (!valid())
            throw std::logic_error("parse_result: validation errors exist");
        return value;
    }

    // --- Builder (internal) ---
    std::vector<std::string> extra_keys_;
    std::vector<std::string> missing_fields_;
    std::vector<validation_error> validation_errors_;
};

// ═══════════════════════════════════════════════════════════════════════
// parse_options — configurable strictness for parse()
// ═══════════════════════════════════════════════════════════════════════

struct parse_options {
    bool reject_extra_keys  = false;
    bool require_all_fields = false;
    bool require_valid      = false;
    bool strict             = false;  // sets all three to true
};

// ═══════════════════════════════════════════════════════════════════════
// parse_error — structured exception from strict parsing
// ═══════════════════════════════════════════════════════════════════════

class parse_error : public std::logic_error {
public:
    parse_error(std::string message,
                std::vector<std::string> extra_keys,
                std::vector<std::string> missing_fields,
                std::vector<validation_error> validation_errors)
        : std::logic_error(std::move(message))
        , extra_keys_(std::move(extra_keys))
        , missing_fields_(std::move(missing_fields))
        , validation_errors_(std::move(validation_errors)) {}

    const std::vector<std::string>& extra_keys() const { return extra_keys_; }
    const std::vector<std::string>& missing_fields() const { return missing_fields_; }
    const std::vector<validation_error>& validation_errors() const { return validation_errors_; }

private:
    std::vector<std::string> extra_keys_;
    std::vector<std::string> missing_fields_;
    std::vector<validation_error> validation_errors_;
};

// ═══════════════════════════════════════════════════════════════════════
// Validator infrastructure (continued from field.cppm)
// ═══════════════════════════════════════════════════════════════════════

namespace detail {
    template <typename T>
    inline constexpr bool is_validator_pack_v = false;

    template <typename... Vs>
    inline constexpr bool is_validator_pack_v<validator_pack<Vs...>> = true;

    template <typename T>
    inline constexpr bool is_with_v = false;

    template <typename... Exts>
    inline constexpr bool is_with_v<with<Exts...>> = true;

}

// ═══════════════════════════════════════════════════════════════════════
// Built-in validators (collab::model::validations)
// ═══════════════════════════════════════════════════════════════════════

namespace validations {

    struct not_empty {
        std::optional<std::string> operator()(const std::string& value) const {
            if (!value.empty()) return std::nullopt;
            return "must not be empty";
        }
    };

    struct max_length {
        std::size_t limit;
        std::optional<std::string> operator()(const std::string& value) const {
            if (value.size() <= limit) return std::nullopt;
            return "length " + std::to_string(value.size()) +
                   " exceeds maximum of " + std::to_string(limit);
        }
    };

    struct positive {
        template <typename T>
        std::optional<std::string> operator()(const T& value) const {
            if (value > 0) return std::nullopt;
            return std::to_string(value) + " must be positive";
        }
    };

}  // namespace validations

namespace detail {
    // Sentinel type for the non-templated type_def. Users never see this —
    // CTAD deduces type_def("Event") → type_def<detail::dynamic_tag>.
    struct dynamic_tag {};
}  // namespace detail

class field_def;  // forward declaration for concept

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

namespace detail {

    // ── Discovery helpers ────────────────────────────────────────────

    // Count meta<M> members for a specific M
    template <typename T, typename M, std::size_t... Is>
    consteval std::size_t count_meta_of_impl(std::index_sequence<Is...>) {
        return (0 + ... + (is_meta_of_v<
            std::remove_cvref_t<collab::model::detail::member_type<Is, T>>, M> ? 1 : 0));
    }

    template <typename T, typename M>
    consteval std::size_t count_meta_of() {
        constexpr auto N = collab::model::detail::dispatch_field_count<T>();
        return count_meta_of_impl<T, M>(std::make_index_sequence<N>{});
    }

    // Count all meta<> members (any type)
    template <typename T, std::size_t... Is>
    consteval std::size_t count_all_metas_impl(std::index_sequence<Is...>) {
        return (0 + ... + (collab::model::is_meta<
            collab::model::detail::member_type<Is, T>> ? 1 : 0));
    }

    template <typename T>
    consteval std::size_t count_all_metas() {
        constexpr auto N = collab::model::detail::dispatch_field_count<T>();
        return count_all_metas_impl<T>(std::make_index_sequence<N>{});
    }

    // ── Meta extraction helpers ──────────────────────────────────────

    template <std::size_t I, typename Obj, typename M>
    constexpr void try_extract_meta(Obj& obj, M& result, bool& found) {
        if (found) return;
        using T = std::remove_cvref_t<Obj>;
        using member_t = std::remove_cvref_t<collab::model::detail::member_type<I, T>>;
        if constexpr (is_meta_of_v<member_t, M>) {
            result = collab::model::detail::dispatch_get_member<I>(obj).value;
            found = true;
        }
    }

    template <typename Obj, typename M, std::size_t... Is>
    constexpr M extract_first_meta(Obj& obj, std::index_sequence<Is...>) {
        M result{};
        bool found = false;
        (try_extract_meta<Is>(obj, result, found), ...);
        return result;
    }

    template <typename Obj, typename M, std::size_t... Is>
    constexpr std::vector<M> extract_all_metas(Obj& obj, std::index_sequence<Is...>) {
        std::vector<M> results;
        auto collect = [&]<std::size_t I>(std::integral_constant<std::size_t, I>) {
            using T = std::remove_cvref_t<Obj>;
            using member_t = std::remove_cvref_t<collab::model::detail::member_type<I, T>>;
            if constexpr (is_meta_of_v<member_t, M>) {
                results.push_back(
                    collab::model::detail::dispatch_get_member<I>(obj).value);
            }
        };
        (collect(std::integral_constant<std::size_t, Is>{}), ...);
        return results;
    }

    // ── Field iteration helpers ──────────────────────────────────────

    template <std::size_t I, typename Obj, typename F>
    constexpr void visit_field_value(Obj& obj, F&& fn) {
        using T = std::remove_cvref_t<Obj>;
        using member_t = collab::model::detail::member_type<I, T>;
        if constexpr (collab::model::is_field<member_t>) {
            auto& member = collab::model::detail::dispatch_get_member<I>(obj);
            fn(collab::model::detail::dispatch_field_name_rt<I, T>(), member.value);
        }
    }

    template <typename Obj, typename F, std::size_t... Is>
    constexpr void for_each_field_value(Obj& obj, F&& fn, std::index_sequence<Is...>) {
        (visit_field_value<Is>(obj, fn), ...);
    }

    // ── Raw field iteration (passes field<T> wrapper, not unwrapped value) ──

    template <std::size_t I, typename Obj, typename F>
    constexpr void visit_raw_field(Obj& obj, F&& fn) {
        using T = std::remove_cvref_t<Obj>;
        using member_t = collab::model::detail::member_type<I, T>;
        if constexpr (collab::model::is_field<member_t>) {
            auto& member = collab::model::detail::dispatch_get_member<I>(obj);
            fn(collab::model::detail::dispatch_field_name_rt<I, T>(), member);
        }
    }

    template <typename Obj, typename F, std::size_t... Is>
    constexpr void for_each_raw_field(Obj& obj, F&& fn, std::index_sequence<Is...>) {
        (visit_raw_field<Is>(obj, fn), ...);
    }

    // ── Schema-only field iteration ──────────────────────────────────

    template <typename T, std::size_t I, typename F>
    constexpr void visit_field_descriptor(F&& fn) {
        using member_t = collab::model::detail::member_type<I, T>;
        if constexpr (collab::model::is_field<member_t>) {
            fn(collab::model::field_descriptor<T, I>{});
        }
    }

    template <typename T, typename F, std::size_t... Is>
    constexpr void for_each_field_descriptor(F&& fn, std::index_sequence<Is...>) {
        (visit_field_descriptor<T, Is>(fn), ...);
    }

    // ── Meta iteration helpers ───────────────────────────────────────

    template <std::size_t I, typename Obj, typename F>
    constexpr void visit_meta_value(Obj& obj, F&& fn) {
        using T = std::remove_cvref_t<Obj>;
        using member_t = std::remove_cvref_t<collab::model::detail::member_type<I, T>>;
        if constexpr (collab::model::is_meta<member_t>) {
            fn(collab::model::detail::dispatch_get_member<I>(obj).value);
        }
    }

    template <typename Obj, typename F, std::size_t... Is>
    constexpr void for_each_meta_value(Obj& obj, F&& fn, std::index_sequence<Is...>) {
        (visit_meta_value<Is>(obj, fn), ...);
    }

    // ── Get by name ──────────────────────────────────────────────────

    template <std::size_t I, typename Obj, typename F>
    constexpr void try_get_field(Obj& obj, std::string_view name, F&& fn, bool& found) {
        if (found) return;
        using T = std::remove_cvref_t<Obj>;
        using member_t = collab::model::detail::member_type<I, T>;
        // field_indices_ guarantees only field<> member indices arrive here
        auto& member = collab::model::detail::dispatch_get_member<I>(obj);
        if (collab::model::detail::dispatch_field_name_rt<I, T>() == name) {
            fn(collab::model::detail::dispatch_field_name_rt<I, T>(), member.value);
            found = true;
        }
    }

    template <typename Obj, typename F, std::size_t... Is>
    constexpr bool get_field_by_name(Obj& obj, std::string_view name, F&& fn,
                                     std::index_sequence<Is...>) {
        bool found = false;
        (try_get_field<Is>(obj, name, fn, found), ...);
        return found;
    }

    // ── Set by name ──────────────────────────────────────────────────

    template <std::size_t I, typename Obj, typename V>
    constexpr void try_set_field(Obj& obj, std::string_view name, V&& val,
                                 bool& set_ok, bool& name_matched) {
        if (set_ok) return;
        using T = std::remove_cvref_t<Obj>;
        using member_t = collab::model::detail::member_type<I, T>;
        // field_indices_ guarantees only field<> member indices arrive here
        if (collab::model::detail::dispatch_field_name_rt<I, T>() == name) {
            name_matched = true;
            using value_t = typename member_t::value_type;
            if constexpr (std::is_constructible_v<value_t, V>) {
                collab::model::detail::dispatch_get_member<I>(obj).value =
                    std::forward<V>(val);
                set_ok = true;
            }
        }
    }

    template <typename Obj, typename V, std::size_t... Is>
    constexpr void set_field_by_name(Obj& obj, std::string_view name, V&& val,
                                     std::index_sequence<Is...>,
                                     bool& name_matched, bool& set_ok) {
        (try_set_field<Is>(obj, name, std::forward<V>(val), set_ok, name_matched), ...);
    }

    // ── JSON codec initializer — forward-declared, defined in :field_json
    //
    // .field<V>() stores a function pointer to this template. The pointer
    // is NOT called at registration time — only when to_json()/load_json()
    // is first invoked, from field_json.cpp where json.hpp is available.

    struct dynamic_field_def;

    template <typename V>
    void init_json_codec(const dynamic_field_def& fd);

    // ── Type-erased meta entry ───────────────────────────────────────

    struct meta_entry {
        std::type_index type{typeid(void)};
        std::any        value;
    };

    // ── Type-erased field definition (for dynamic type_def) ──────────

    struct dynamic_field_def {
        std::string              name;
        std::type_index          type{typeid(void)};
        std::any                 default_value;
        bool                     has_default = false;
        std::vector<meta_entry>  metas;

        std::function<bool(std::any&, std::any&&)> setter;
        std::function<std::any()> make_default;

        // JSON codec — lazily initialized. _json_init stores a function
        // that calls init_json_codec<V> to populate to_json_fn/from_json_fn.
        // The std::function wraps a generic lambda whose body is NOT
        // instantiated until called (from field_json.cpp).
        // Mutable so lazy init works even on const type_def instances.
        mutable std::function<void(const dynamic_field_def&)> _json_init;
        mutable std::function<std::any(const std::any&)>        to_json_fn;
        mutable std::function<void(std::any&, const std::any&)> from_json_fn;

        // Validation — type-erased function that runs all attached validators.
        // Returns validation errors for this field (empty if all pass).
        std::function<std::vector<validation_error>(
            const std::any& value, std::string_view field_name)> validate_fn;
    };

    // ── Extract metas from a with<Exts...> ───────────────────────────

    template <typename... Exts>
    void extract_with_metas(std::vector<meta_entry>& out, const with<Exts...>& w) {
        (out.push_back(
            {typeid(Exts), std::any(static_cast<const Exts&>(w))}), ...);
    }

    // ── Build type-erased validate_fn from a validator_pack ──────────
    //
    // extract_short_validator_name is defined in field.cppm.

    template <typename FieldType, typename... Validators>
    auto make_validate_fn(const validator_pack<Validators...>& pack) {
        return [captured_validators = pack.packed](
                const std::any& value, std::string_view field_name)
            -> std::vector<validation_error>
        {
            std::vector<validation_error> errors;
            const auto& typed_value = *std::any_cast<FieldType>(&value);
            std::apply([&](const auto&... each_validator) {
                (([&] {
                    auto result = each_validator(typed_value);
                    if (result.has_value()) {
                        using validator_type = std::remove_cvref_t<decltype(each_validator)>;
                        errors.push_back({
                            std::string(field_name),
                            std::move(*result),
                            detail::extract_short_validator_name(NAMEOF_TYPE(validator_type))
                        });
                    }
                }()), ...);
            }, captured_validators);
            return errors;
        };
    }

    // ── Process variadic args: extract metas OR validators ───────────

    template <typename FieldType, typename Arg>
    void process_field_arg(dynamic_field_def& fd, const Arg& arg) {
        using ArgType = std::remove_cvref_t<Arg>;
        if constexpr (is_validator_pack_v<ArgType>) {
            fd.validate_fn = make_validate_fn<FieldType>(arg);
        } else {
            extract_with_metas(fd.metas, arg);
        }
    }

    // ── Typed hybrid field registration (for type_def<T, Regs...>) ───
    //
    // Preserves the real member type MemT in the template parameter so
    // for_each can give the callback real typed references.

    template <typename T, typename MemT>
    struct typed_field_reg {
        MemT T::*               member;
        std::string             name;
        std::vector<meta_entry> metas;
        detail::field_validator_fn<MemT> validate_fn;
    };

    // Process a single arg in the hybrid .field() builder:
    // either a with<> (extract metas) or a validator_pack (store validate_fn)
    template <typename T, typename MemT, typename Arg>
    void process_hybrid_field_arg(typed_field_reg<T, MemT>& reg, const Arg& arg) {
        using ArgType = std::remove_cvref_t<Arg>;
        if constexpr (is_validator_pack_v<ArgType>) {
            // Convert validator_pack to detail::field_validator_fn<MemT>
            reg.validate_fn = static_cast<detail::field_validator_fn<MemT>>(arg);
        } else {
            extract_with_metas(reg.metas, arg);
        }
    }

    template <typename T, typename MemT, typename... Args>
    typed_field_reg<T, MemT> make_typed_reg(
            MemT T::* member, std::string_view fname, Args... args) {
        typed_field_reg<T, MemT> reg{member, std::string(fname), {}, {}};
        (process_hybrid_field_arg(reg, args), ...);
        return reg;
    }

    // ── Build dynamic_field_def from auto-discovered field by name ──

    template <std::size_t I, typename TT>
    void try_build_discovered_field_def(
            dynamic_field_def& out, std::string_view target, bool& found) {
        if (found) return;
        using member_t = collab::model::detail::member_type<I, TT>;
        if constexpr (collab::model::is_field<member_t>) {
            if (collab::model::detail::dispatch_field_name_rt<I, TT>() == target) {
                out.name = std::string(target);
                out.type = typeid(typename member_t::value_type);
                out.has_default = false;
                out.metas.clear();
                TT instance{};
                extract_with_metas(out.metas,
                    collab::model::detail::dispatch_get_member<I>(instance).with);
                found = true;
            }
        }
    }

    template <typename TT, std::size_t... Is>
    void build_discovered_field_def(
            dynamic_field_def& out, std::string_view target,
            bool& found, std::index_sequence<Is...>) {
        (try_build_discovered_field_def<Is, TT>(out, target, found), ...);
    }

}  // namespace detail

// ── field_def — read-only view into a dynamic field ─────────────

class field_def {
    const detail::dynamic_field_def* def_;

public:
    explicit field_def(const detail::dynamic_field_def* d) : def_(d) {}

    std::string_view name() const { return def_->name; }

    bool has_default() const { return def_->has_default; }

    template <typename V>
    V default_value() const {
        auto* p = std::any_cast<V>(&def_->default_value);
        if (!p) throw std::logic_error(
            "field '" + std::string(name()) + "': default_value type mismatch");
        return *p;
    }

    template <typename M>
    bool has_meta() const {
        for (auto& e : def_->metas)
            if (e.type == typeid(M)) return true;
        return false;
    }

    template <typename M>
    M meta() const {
        for (auto& e : def_->metas)
            if (e.type == typeid(M))
                return *std::any_cast<M>(&e.value);
        throw std::logic_error(
            "field '" + std::string(name()) + "': no meta of requested type");
    }

    template <typename M>
    std::size_t meta_count() const {
        std::size_t n = 0;
        for (auto& e : def_->metas)
            if (e.type == typeid(M)) ++n;
        return n;
    }

    template <typename M>
    std::vector<M> metas() const {
        std::vector<M> result;
        for (auto& e : def_->metas)
            if (e.type == typeid(M)) {
                auto* p = std::any_cast<M>(&e.value);
                if (!p) throw std::logic_error(
                    "field '" + std::string(name()) + "': meta storage corrupted");
                result.push_back(*p);
            }
        return result;
    }
};

// ── metadata — typed access to a type-erased meta ────────────────────
//
// Wraps std::any internally but never exposes it. Users access the
// underlying meta struct through .as<M>(), .try_as<M>(), or .is<M>().
// Metas are always read-only — they belong to the schema, not the instance.

class metadata {
    const std::any* value_;
    std::type_index type_;

public:
    // Constructor is public but useless without raw pointers to internals.
    // Only type_def<detail::dynamic_tag> has those.
    explicit metadata(const std::any* v, const std::type_index& t)
        : value_(v), type_(t) {}

    template <typename M>
    const M& as() const {
        auto* p = std::any_cast<M>(value_);
        if (!p) throw std::logic_error("metadata: type mismatch in as<>()");
        return *p;
    }

    template <typename M>
    const M* try_as() const { return std::any_cast<M>(value_); }

    template <typename M>
    bool is() const { return type_ == typeid(M); }
};

// ── field_value — typed access to a type-erased value ─────────────────
//
// Wraps std::any internally but never exposes it. Users access values
// through .as<T>() or .try_as<T>().

class field_value {
    std::any* value_;
    friend class type_instance;
    explicit field_value(std::any* v) : value_(v) {}

public:
    template <typename V>
    V& as() {
        auto* p = std::any_cast<V>(value_);
        if (!p) throw std::logic_error("field_value: type mismatch in as<>()");
        return *p;
    }

    template <typename V>
    const V& as() const {
        auto* p = std::any_cast<V>(value_);
        if (!p) throw std::logic_error("field_value: type mismatch in as<>()");
        return *p;
    }

    template <typename V>
    V* try_as() { return std::any_cast<V>(value_); }

    template <typename V>
    const V* try_as() const { return std::any_cast<V>(value_); }
};

class const_field_value {
    const std::any* value_;
    friend class type_instance;
    explicit const_field_value(const std::any* v) : value_(v) {}

public:
    template <typename V>
    const V& as() const {
        auto* p = std::any_cast<V>(value_);
        if (!p) throw std::logic_error("const_field_value: type mismatch in as<>()");
        return *p;
    }

    template <typename V>
    const V* try_as() const { return std::any_cast<V>(value_); }
};

namespace detail {

// ── Compile-time index filter: keep only indices where Pred<I, T> is true ──
template <typename T, template<std::size_t, typename> class Pred, std::size_t... Is>
consteval auto filter_indices(std::index_sequence<Is...>) {
    constexpr auto N = (... + (Pred<Is, T>::value ? 1 : 0));
    std::array<std::size_t, N> arr{};
    std::size_t pos = 0;
    ((Pred<Is, T>::value ? (arr[pos++] = Is, 0) : 0), ...);
    return arr;
}

template <typename T, std::size_t... Is>
consteval auto array_to_index_seq(std::array<std::size_t, sizeof...(Is)>, std::index_sequence<Is...>);

template <typename T, template<std::size_t, typename> class Pred, std::size_t... AllIs>
auto make_filtered_sequence(std::index_sequence<AllIs...>) {
    constexpr auto arr = filter_indices<T, Pred>(std::index_sequence<AllIs...>{});
    return [&]<std::size_t... Js>(std::index_sequence<Js...>) {
        return std::index_sequence<arr[Js]...>{};
    }(std::make_index_sequence<arr.size()>{});
}

template <std::size_t I, typename T>
struct is_field_at : std::bool_constant<collab::model::is_field<
    collab::model::detail::member_type<I, T>>> {};

}  // namespace detail

template <typename T = detail::dynamic_tag, typename... Regs>
class type_def {
    static constexpr auto total_members_ = collab::model::detail::dispatch_field_count<T>();
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
        return collab::model::detail::type_name<T>();
    }

    // ── Field queries ────────────────────────────────────────────────

    std::size_t field_count() const {
        return collab::model::detail::field_count<T>() + sizeof...(Regs);
    }

    std::vector<std::string> field_names() const {
        auto discovered = collab::model::detail::field_names<T>();
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
        auto discovered = collab::model::detail::field_names<T>();
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

    parse_result<T> parse(const nlohmann::json& j) const {
        if (!j.is_object()) throw std::logic_error("parse: expected JSON object");

        parse_result<T> result{.value = T{}};

        // Populate using raw field iteration — handles nested structs recursively
        detail::for_each_raw_field(result.value, [&](std::string_view name, auto& raw_field) {
            std::string key(name);
            if (j.contains(key)) {
                using InnerType = std::remove_cvref_t<decltype(raw_field.value)>;
                if constexpr (detail::reflected_struct<InnerType>) {
                    // Nested struct — recurse via parse
                    if (j[key].is_object()) {
                        auto nested_result = type_def<InnerType>{}.parse(j[key]);
                        raw_field.value = std::move(nested_result.value);
                    }
                } else {
                    try {
                        raw_field.value = j[key].template get<InnerType>();
                    } catch (...) {
                        // Type mismatch — keep default
                    }
                }
            }
        }, indices_{});

        // Also populate hybrid-registered fields
        std::apply([&](const auto&... regs) {
            (([&] {
                if (j.contains(regs.name)) {
                    using MemT = std::remove_cvref_t<
                        decltype(result.value.*(regs.member))>;
                    try {
                        result.value.*(regs.member) = j[regs.name].template get<MemT>();
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

    parse_result<T> parse(const nlohmann::json& j, parse_options options) const {
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
};

// ═══════════════════════════════════════════════════════════════════════
// Dynamic type_def — runtime builder, no backing struct
// ═══════════════════════════════════════════════════════════════════════

// Forward declaration for type_instance (defined below)
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
    // Defined in field_json.cpp because it needs type_instance to be complete.

    type_def& field(std::string_view fname,
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
    // Defined in field_json.cpp (module implementation unit).

    type_instance create(const nlohmann::json& j) const;

    // ── Parse JSON with reporting ───────────────────────────────────
    // Returns parse_result<type_instance> — always contains the object
    // plus extra_keys, missing_fields, and validation_errors.
    // Defined in field_json.cpp (module implementation unit).

    parse_result<type_instance> parse(const nlohmann::json& j) const;
    parse_result<type_instance> parse(const nlohmann::json& j, parse_options options) const;
};

// ── CTAD: type_def("Event") deduces to type_def<detail::dynamic_tag> ─────────────

type_def(const char*) -> type_def<detail::dynamic_tag>;
type_def(std::string_view) -> type_def<detail::dynamic_tag>;

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
    // Defined in field_json.cpp (module implementation unit).

    void load_json(const nlohmann::json& j);

    // ── JSON serialization ───────────────────────────────────────────
    //
    // Defined in field_json.cpp (module implementation unit).

    nlohmann::json to_json() const;
    std::string to_json_string(int indent = -1) const;
};

// ── type_def<detail::dynamic_tag>::create() ──────────────────────────────────────

inline type_instance type_def<detail::dynamic_tag>::create() const {
    return type_instance(*this);
}

// ═══════════════════════════════════════════════════════════════════════
// field<type_def<>> — nests a runtime-defined type inside a struct
//
//   auto details = type_def("Details").field<int>("x");
//   struct Container {
//       field<std::string>  name;
//       field<type_def<>>   details {details_type};
//   };
//
// Internally stores a type_instance. The type_def is passed at
// construction time and used to create the default instance.
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

}  // namespace collab::model
