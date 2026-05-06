#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include <nlohmann/json.hpp>

#include <def_type/oneof.hpp>
#include <def_type/reflect.hpp>

// ── ADL probes ──────────────────────────────────────────────────────────────
//
// Lives in its own bare namespace so ordinary unqualified lookup finds
// nothing — leaving ADL as the only path. With `oneof<typename... Args>`
// the alt types travel as TYPE template arguments, which the standard says
// (and MSVC empirically confirms — see scratch/oneof_repro/v9_*) propagate
// through to ADL associated namespaces. So a user-supplied
// `to_json(json&, const Variant&)` next to one of the alt types is found
// via unqualified ADL on the variant value.

namespace def_type_oneof_adl {
    template <typename V>
    concept has_user_to_json = requires (nlohmann::json& j, const V& v) {
        to_json(j, v);
    };

    template <typename V>
    concept has_user_from_json = requires (const nlohmann::json& j, V& v) {
        from_json(j, v);
    };
}

namespace def_type::detail {

    template <typename V>
    inline constexpr bool has_user_oneof_to_json =
        ::def_type_oneof_adl::has_user_to_json<V>;

    template <typename V>
    inline constexpr bool has_user_oneof_from_json =
        ::def_type_oneof_adl::has_user_from_json<V>;

    // ── cascade forward declarations ────────────────────────────────

    template <typename T>
    nlohmann::json value_to_json(const T& v);

    template <typename T>
    void value_from_json(const nlohmann::json& j, T& out);

    // ── tuple type-list iteration helper ───────────────────────────

    template <typename Tup, typename F, std::size_t... Is>
    constexpr bool for_each_tuple_type_until_impl(F&& fn, std::index_sequence<Is...>) {
        bool stopped = false;
        (..., (stopped = stopped || fn.template operator()<
            std::tuple_element_t<Is, Tup>>(Is)));
        return stopped;
    }

    template <typename Tup, typename F>
    constexpr bool for_each_tuple_type_until(F&& fn) {
        return for_each_tuple_type_until_impl<Tup>(std::forward<F>(fn),
            std::make_index_sequence<std::tuple_size_v<Tup>>{});
    }

    // ── small render helpers for error messages ─────────────────────

    inline std::string render_label_list(
            const std::string_view* items, std::size_t n) {
        std::string out;
        for (std::size_t i = 0; i < n; ++i) {
            if (i > 0) out += i + 1 == n ? ", or " : ", ";
            out += '\'';
            out += items[i];
            out += '\'';
        }
        return out;
    }

    template <typename Variant>
    inline std::string render_known_labels() {
        return render_label_list(Variant::labels.data(), Variant::num_alts);
    }

    template <typename Tup, std::size_t... Is>
    inline std::string render_alt_names_impl(std::index_sequence<Is...>) {
        std::array<std::string_view, sizeof...(Is)> names{
            type_name<std::tuple_element_t<Is, Tup>>()...
        };
        return render_label_list(names.data(), names.size());
    }

    template <typename Tup>
    inline std::string render_alt_names() {
        return render_alt_names_impl<Tup>(
            std::make_index_sequence<std::tuple_size_v<Tup>>{});
    }

    // ── shape-only fitting ─────────────────────────────────────────

    template <typename T>
    bool json_keys_are_subset_of_alt_fields(const nlohmann::json& j) {
        if constexpr (reflected_struct<T>) {
            auto fields = field_names<T>();
            for (auto it = j.begin(); it != j.end(); ++it) {
                bool ok = false;
                for (auto fname : fields)
                    if (fname == it.key()) { ok = true; break; }
                if (!ok) return false;
            }
            return true;
        } else {
            return false;
        }
    }

    template <typename OneofT>
    void shape_only_from_json(const nlohmann::json& j, OneofT& out) {
        if (!j.is_object())
            throw std::logic_error("oneof: expected JSON object");

        using Alts = typename OneofT::alts_tuple;
        constexpr std::size_t N = std::tuple_size_v<Alts>;

        std::array<bool, N> fits{};
        std::size_t fit_count = 0;

        for_each_tuple_type_until<Alts>([&]<typename T>(std::size_t idx) {
            bool ok = json_keys_are_subset_of_alt_fields<T>(j);
            fits[idx] = ok;
            if (ok) ++fit_count;
            return false;
        });

        if (fit_count == 0) {
            std::string offending;
            for (auto it = j.begin(); it != j.end(); ++it) {
                bool found_anywhere = false;
                for_each_tuple_type_until<Alts>([&]<typename T>(std::size_t) {
                    if constexpr (reflected_struct<T>) {
                        for (auto f : field_names<T>())
                            if (f == it.key()) { found_anywhere = true; break; }
                    }
                    return false;
                });
                if (!found_anywhere) { offending = it.key(); break; }
            }
            throw std::logic_error(
                "oneof shape-only: no alternative matches JSON shape; "
                "key '" + offending + "' belongs to no alternative ("
                + render_alt_names<Alts>() + ").");
        }

        if (fit_count > 1) {
            std::string tied;
            std::size_t shown = 0;
            for_each_tuple_type_until<Alts>([&]<typename T>(std::size_t idx) {
                if (fits[idx]) {
                    if (shown > 0) tied += ", ";
                    tied += type_name<T>();
                    ++shown;
                }
                return false;
            });
            throw std::logic_error(
                "oneof shape-only: JSON shape matches multiple alternatives ("
                + tied + ").");
        }

        for_each_tuple_type_until<Alts>([&]<typename T>(std::size_t idx) {
            if (!fits[idx]) return false;
            T tmp{};
            value_from_json(j, tmp);
            out = OneofT(std::move(tmp));
            return true;
        });
    }

    template <typename OneofT>
    nlohmann::json shape_only_to_json(const OneofT& v) {
        return std::visit([](const auto& alt_value) {
            return value_to_json(alt_value);
        }, v._internal_variant());
    }

    // ── inside-object form ────────────────────────────────────────

    template <typename OneofT>
    nlohmann::json oneof_by_field_to_json(const OneofT& v) {
        nlohmann::json j;
        std::size_t idx = v._internal_variant().index();
        std::visit([&](const auto& alt_value) {
            j = value_to_json(alt_value);
        }, v._internal_variant());
        if (!j.is_object()) j = nlohmann::json::object();
        j[std::string(OneofT::discriminator_key.view())] =
            std::string(OneofT::labels[idx]);
        return j;
    }

    template <typename OneofT>
    void oneof_by_field_from_json(const nlohmann::json& j, OneofT& out) {
        if (!j.is_object())
            throw std::logic_error("oneof_by_field: expected JSON object");

        std::string disc_key(OneofT::discriminator_key.view());
        if (!j.contains(disc_key))
            throw std::logic_error(
                "oneof_by_field: discriminator key '" + disc_key
                + "' missing; known labels are " + render_known_labels<OneofT>() + ".");

        auto disc = j.at(disc_key).template get<std::string>();
        std::size_t idx = OneofT::alt_for_label(disc);
        if (idx == static_cast<std::size_t>(-1))
            throw std::logic_error(
                "oneof_by_field: discriminator '" + disc
                + "' does not match any known label ("
                + render_known_labels<OneofT>() + ").");

        using Alts = typename OneofT::alts_tuple;
        for_each_tuple_type_until<Alts>([&]<typename T>(std::size_t i) {
            if (i != idx) return false;
            T tmp{};
            value_from_json(j, tmp);
            out = OneofT(std::move(tmp));
            return true;
        });
    }

    // ── outside-object form ───────────────────────────────────────

    template <typename OneofT>
    nlohmann::json oneof_by_parent_field_to_json(const OneofT& v) {
        // Just emit the active alt's nested JSON; the parent struct's
        // serializer overwrites the discriminator JSON value separately
        // (see Strategy B in json.hpp's reflected_struct branch).
        return std::visit([](const auto& alt_value) {
            return value_to_json(alt_value);
        }, v._internal_variant());
    }

    template <typename OneofT>
    void oneof_by_parent_field_from_json_with_disc(
            const nlohmann::json& j_alt, OneofT& out, std::string_view disc) {
        std::size_t idx = OneofT::alt_for_label(disc);
        if (idx == static_cast<std::size_t>(-1))
            throw std::logic_error(
                "oneof_by_parent_field: discriminator '" + std::string(disc)
                + "' does not match any known label ("
                + render_known_labels<OneofT>() + ").");

        if (!j_alt.is_object())
            throw std::logic_error(
                "oneof_by_parent_field: expected JSON object for variant field");

        using Alts = typename OneofT::alts_tuple;
        for_each_tuple_type_until<Alts>([&]<typename T>(std::size_t i) {
            if (i != idx) return false;
            T tmp{};
            value_from_json(j_alt, tmp);
            out = OneofT(std::move(tmp));
            return true;
        });
    }

    template <typename OneofT>
    std::string_view active_label(const OneofT& v) {
        return OneofT::labels[v._internal_variant().index()];
    }

    // ── top-level dispatch ────────────────────────────────────────

    template <typename OneofT>
    nlohmann::json oneof_dispatch_to_json(const OneofT& v) {
        if constexpr (has_user_oneof_to_json<OneofT>) {
            nlohmann::json j;
            to_json(j, v);   // unqualified — ADL
            return j;
        } else if constexpr (is_oneof_v<OneofT>) {
            return shape_only_to_json(v);
        } else if constexpr (is_oneof_by_field_v<OneofT>) {
            return oneof_by_field_to_json(v);
        } else {
            return oneof_by_parent_field_to_json(v);
        }
    }

    template <typename OneofT>
    void oneof_dispatch_from_json(const nlohmann::json& j, OneofT& out) {
        if constexpr (has_user_oneof_from_json<OneofT>) {
            from_json(j, out);   // unqualified — ADL
        } else if constexpr (is_oneof_v<OneofT>) {
            shape_only_from_json(j, out);
        } else if constexpr (is_oneof_by_field_v<OneofT>) {
            oneof_by_field_from_json(j, out);
        } else {
            // outside-object: cannot deserialize without enclosing parent
            // struct (the discriminator lives on a sibling field). The
            // reflected_struct branch in value_from_json handles this case
            // before reaching here.
            throw std::logic_error(
                "oneof_by_parent_field: cannot deserialize without enclosing "
                "parent struct (the discriminator lives on a sibling field).");
        }
    }

}  // namespace def_type::detail
