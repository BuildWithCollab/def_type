#pragma once

#include <any>
#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

#include <nameof.hpp>

#include <def_type/field.hpp>
#include <def_type/meta.hpp>
#include <def_type/reflect.hpp>
#include <def_type/validation.hpp>

namespace def_type::detail {

    // ── Meta counting helpers ───────────────────────────────────────

    // Count meta<M> members for a specific M
    template <typename T, typename M, std::size_t... Is>
    consteval std::size_t count_meta_of_impl(std::index_sequence<Is...>) {
        return (0 + ... + (is_meta_of_v<
            std::remove_cvref_t<def_type::detail::member_type<Is, T>>, M> ? 1 : 0));
    }

    template <typename T, typename M>
    consteval std::size_t count_meta_of() {
        constexpr auto N = def_type::detail::dispatch_field_count<T>();
        return count_meta_of_impl<T, M>(std::make_index_sequence<N>{});
    }

    // Count all meta<> members (any type)
    template <typename T, std::size_t... Is>
    consteval std::size_t count_all_metas_impl(std::index_sequence<Is...>) {
        return (0 + ... + (def_type::is_meta<
            def_type::detail::member_type<Is, T>> ? 1 : 0));
    }

    template <typename T>
    consteval std::size_t count_all_metas() {
        constexpr auto N = def_type::detail::dispatch_field_count<T>();
        return count_all_metas_impl<T>(std::make_index_sequence<N>{});
    }

    // ── Meta extraction helpers ──────────────────────────────────────

    template <std::size_t I, typename Obj, typename M>
    constexpr void try_extract_meta(Obj& obj, M& result, bool& found) {
        if (found) return;
        using T = std::remove_cvref_t<Obj>;
        using member_t = std::remove_cvref_t<def_type::detail::member_type<I, T>>;
        if constexpr (is_meta_of_v<member_t, M>) {
            result = def_type::detail::dispatch_get_member<I>(obj).value;
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
            using member_t = std::remove_cvref_t<def_type::detail::member_type<I, T>>;
            if constexpr (is_meta_of_v<member_t, M>) {
                results.push_back(
                    def_type::detail::dispatch_get_member<I>(obj).value);
            }
        };
        (collect(std::integral_constant<std::size_t, Is>{}), ...);
        return results;
    }

    // ── Field iteration helpers ──────────────────────────────────────

    template <std::size_t I, typename Obj, typename F>
    constexpr void visit_field_value(Obj& obj, F&& fn) {
        using T = std::remove_cvref_t<Obj>;
        using member_t = def_type::detail::member_type<I, T>;
        if constexpr (def_type::is_field<member_t>) {
            auto& member = def_type::detail::dispatch_get_member<I>(obj);
            fn(def_type::detail::dispatch_field_name_rt<I, T>(), member.value);
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
        using member_t = def_type::detail::member_type<I, T>;
        if constexpr (def_type::is_field<member_t>) {
            auto& member = def_type::detail::dispatch_get_member<I>(obj);
            fn(def_type::detail::dispatch_field_name_rt<I, T>(), member);
        }
    }

    template <typename Obj, typename F, std::size_t... Is>
    constexpr void for_each_raw_field(Obj& obj, F&& fn, std::index_sequence<Is...>) {
        (visit_raw_field<Is>(obj, fn), ...);
    }

    // ── Schema-only field iteration ──────────────────────────────────

    template <typename T, std::size_t I, typename F>
    constexpr void visit_field_descriptor(F&& fn) {
        using member_t = def_type::detail::member_type<I, T>;
        if constexpr (def_type::is_field<member_t>) {
            fn(def_type::field_descriptor<T, I>{});
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
        using member_t = std::remove_cvref_t<def_type::detail::member_type<I, T>>;
        if constexpr (def_type::is_meta<member_t>) {
            fn(def_type::detail::dispatch_get_member<I>(obj).value);
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
        using member_t = def_type::detail::member_type<I, T>;
        // field_indices_ guarantees only field<> member indices arrive here
        auto& member = def_type::detail::dispatch_get_member<I>(obj);
        if (def_type::detail::dispatch_field_name_rt<I, T>() == name) {
            fn(def_type::detail::dispatch_field_name_rt<I, T>(), member.value);
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
        using member_t = def_type::detail::member_type<I, T>;
        // field_indices_ guarantees only field<> member indices arrive here
        if (def_type::detail::dispatch_field_name_rt<I, T>() == name) {
            name_matched = true;
            using value_t = typename member_t::value_type;
            if constexpr (std::is_constructible_v<value_t, V>) {
                def_type::detail::dispatch_get_member<I>(obj).value =
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

    // ── JSON codec initializer — forward-declared, defined in field_json.hpp
    //
    // .field<V>() stores a function pointer to this template. The pointer
    // is NOT called at registration time — only when to_json()/load_json()
    // is first invoked, from field_json.hpp where json.hpp is available.

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
        // instantiated until called (from field_json.hpp).
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
    // extract_short_validator_name is defined in field.hpp.

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

        // Nested schema validation — populated when a nested type_def is
        // passed as an arg to .field(&T::member, "name", nested_type_def).
        std::function<validation_result(const MemT&, std::string_view)> nested_validate_fn;
        std::function<bool(const MemT&)>                                nested_valid_fn;
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
        using member_t = def_type::detail::member_type<I, TT>;
        if constexpr (def_type::is_field<member_t>) {
            if (def_type::detail::dispatch_field_name_rt<I, TT>() == target) {
                out.name = std::string(target);
                out.type = typeid(typename member_t::value_type);
                out.has_default = false;
                out.metas.clear();
                TT instance{};
                extract_with_metas(out.metas,
                    def_type::detail::dispatch_get_member<I>(instance).with);
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
    struct is_field_at : std::bool_constant<def_type::is_field<
        def_type::detail::member_type<I, T>>> {};

}  // namespace def_type::detail
