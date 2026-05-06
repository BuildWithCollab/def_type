#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <def_type/detail/oneof_parsing.hpp>

namespace def_type {

// ─── oneof_type<T, "label"> — alt wrapper used inside the helper templates ─
//
// `oneof_type` ties an alt's C++ type to its JSON label. It is only used as
// a template argument to the discriminator-aware helpers `oneof_by_field`
// and `oneof_by_parent_field`. The primary `oneof<...>` form takes alt
// types directly and has no use for labels.

template <typename T, detail::fixed_string Label>
struct oneof_type {
    using alt_type = T;
    static constexpr auto label = Label;
};

namespace detail {

    template <typename U>
    struct unwrap_oneof_type;

    template <typename T, fixed_string L>
    struct unwrap_oneof_type<oneof_type<T, L>> {
        using type = T;
        static constexpr auto label = L;
    };

    template <typename U>
    inline constexpr bool is_oneof_type_v = false;

    template <typename T, fixed_string L>
    inline constexpr bool is_oneof_type_v<oneof_type<T, L>> = true;

    // ── overload helper for std::visit ──

    template <typename... Ts> struct overload : Ts... { using Ts::operator()...; };
    template <typename... Ts> overload(Ts...) -> overload<Ts...>;

    // ── shared variant-holder base ──
    //
    // All three public oneof forms share the same value-level interface
    // (deleted default ctor, converting ctor, is/as/match, copy/move).
    // The base centralises that.

    template <typename... Alts>
    class oneof_impl_base {
        std::variant<Alts...> v_;

      public:
        oneof_impl_base() = delete;

        template <typename T>
            requires ((std::is_same_v<std::remove_cvref_t<T>, Alts> || ...) &&
                      !std::is_same_v<std::remove_cvref_t<T>, oneof_impl_base>)
        constexpr oneof_impl_base(T&& val) : v_(std::forward<T>(val)) {}

        constexpr oneof_impl_base(const oneof_impl_base&) = default;
        constexpr oneof_impl_base(oneof_impl_base&&)      = default;
        constexpr oneof_impl_base& operator=(const oneof_impl_base&) = default;
        constexpr oneof_impl_base& operator=(oneof_impl_base&&)      = default;

        template <typename T>
            requires ((std::is_same_v<std::remove_cvref_t<T>, Alts> || ...) &&
                      !std::is_same_v<std::remove_cvref_t<T>, oneof_impl_base>)
        constexpr oneof_impl_base& operator=(T&& val) {
            v_ = std::forward<T>(val);
            return *this;
        }

        template <typename T>
            requires ((std::is_same_v<T, Alts> || ...))
        constexpr bool is() const noexcept {
            return std::holds_alternative<T>(v_);
        }

        template <typename T>
            requires ((std::is_same_v<T, Alts> || ...))
        constexpr T* as() noexcept { return std::get_if<T>(&v_); }

        template <typename T>
            requires ((std::is_same_v<T, Alts> || ...))
        constexpr const T* as() const noexcept { return std::get_if<T>(&v_); }

        template <typename... Fs>
        constexpr decltype(auto) match(Fs&&... fs) {
            return std::visit(overload{std::forward<Fs>(fs)...}, v_);
        }

        template <typename... Fs>
        constexpr decltype(auto) match(Fs&&... fs) const {
            return std::visit(overload{std::forward<Fs>(fs)...}, v_);
        }

        constexpr const std::variant<Alts...>& _internal_variant() const noexcept { return v_; }
        constexpr       std::variant<Alts...>& _internal_variant()       noexcept { return v_; }
    };

}  // namespace detail

// ─── oneof<typename... Args> — primary, shape-only form ────────────────────
//
//     using Resource = oneof<TextResource, BlobResource>;
//
// JSON serialization: shape-only fitting algorithm (no discriminator;
// alt is selected by which fields are present).

template <typename... Args>
class oneof : public detail::oneof_impl_base<Args...> {
    using base = detail::oneof_impl_base<Args...>;

  public:
    using base::base;
    using base::operator=;
    using alts_tuple = std::tuple<Args...>;
    static constexpr std::size_t num_alts = sizeof...(Args);

    static_assert(num_alts > 0,
        "oneof: at least one alternative is required.");

    // Library-internal: returns a oneof default-initialised to its first alt.
    // Used by container deserialisation paths (vector<oneof>, map, optional).
    static constexpr oneof _make_default() {
        return oneof(std::tuple_element_t<0, alts_tuple>{});
    }
};

// ─── oneof_by_field<"key", oneof_type<T, "label">, ...> — inside-object ────
//
//     using Content = oneof_by_field<"type",
//         oneof_type<TextContent,  "text">,
//         oneof_type<ImageContent, "image">>;
//
// JSON: discriminator key lives inside the same object as the alt's data.

template <detail::fixed_string Disc, typename... Args>
class oneof_by_field
    : public detail::oneof_impl_base<typename detail::unwrap_oneof_type<Args>::type...>
{
    using base = detail::oneof_impl_base<typename detail::unwrap_oneof_type<Args>::type...>;

    static_assert((detail::is_oneof_type_v<Args> && ...),
        "oneof_by_field: every alt must be `oneof_type<T, \"label\">`.");

  public:
    using base::base;
    using base::operator=;
    using alts_tuple = std::tuple<typename detail::unwrap_oneof_type<Args>::type...>;
    static constexpr std::size_t num_alts = sizeof...(Args);
    static constexpr auto discriminator_key = Disc;

    static_assert(num_alts > 0,
        "oneof_by_field: at least one alternative is required.");

    static constexpr std::array<std::string_view, num_alts> labels = {
        detail::unwrap_oneof_type<Args>::label.view()...
    };

    // Compile-time uniqueness: direct byte comparison on NTTP storage.
    static constexpr bool labels_unique = []() {
        constexpr auto N = num_alts;
        constexpr std::array<std::string_view, N> ls = labels;
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = i + 1; j < N; ++j) {
                if (ls[i].size() != ls[j].size()) continue;
                bool eq = true;
                for (std::size_t k = 0; k < ls[i].size(); ++k)
                    if (ls[i][k] != ls[j][k]) { eq = false; break; }
                if (eq) return false;
            }
        return true;
    }();
    static_assert(labels_unique,
        "oneof_by_field: labels must be unique.");

    static constexpr std::size_t alt_for_label(std::string_view s) noexcept {
        for (std::size_t i = 0; i < num_alts; ++i) {
            const auto& l = labels[i];
            if (l.size() != s.size()) continue;
            bool eq = true;
            for (std::size_t k = 0; k < l.size(); ++k)
                if (l[k] != s[k]) { eq = false; break; }
            if (eq) return i;
        }
        return static_cast<std::size_t>(-1);
    }

    static constexpr oneof_by_field _make_default() {
        return oneof_by_field(std::tuple_element_t<0, alts_tuple>{});
    }
};

// ─── oneof_by_parent_field<&P::field, oneof_type<...>, ...> — outside-object
//
//     struct Message {
//         std::string sender;
//         std::string content_type;
//
//         oneof_by_parent_field<&Message::content_type,
//             oneof_type<TextContent,  "text">,
//             oneof_type<ImageContent, "image">> content;
//     };
//
// JSON: discriminator value lives on the parent struct (a sibling field
// declared *before* the variant field).

template <auto MemberPtr, typename... Args>
class oneof_by_parent_field
    : public detail::oneof_impl_base<typename detail::unwrap_oneof_type<Args>::type...>
{
    using base = detail::oneof_impl_base<typename detail::unwrap_oneof_type<Args>::type...>;

    static_assert(std::is_member_object_pointer_v<decltype(MemberPtr)>,
        "oneof_by_parent_field: first arg must be a pointer-to-member-object.");
    static_assert((detail::is_oneof_type_v<Args> && ...),
        "oneof_by_parent_field: every alt must be `oneof_type<T, \"label\">`.");

  public:
    using base::base;
    using base::operator=;
    using alts_tuple = std::tuple<typename detail::unwrap_oneof_type<Args>::type...>;
    static constexpr std::size_t num_alts = sizeof...(Args);
    static constexpr auto discriminator_member = MemberPtr;

    static_assert(num_alts > 0,
        "oneof_by_parent_field: at least one alternative is required.");

    static constexpr std::array<std::string_view, num_alts> labels = {
        detail::unwrap_oneof_type<Args>::label.view()...
    };

    static constexpr bool labels_unique = []() {
        constexpr auto N = num_alts;
        constexpr std::array<std::string_view, N> ls = labels;
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = i + 1; j < N; ++j) {
                if (ls[i].size() != ls[j].size()) continue;
                bool eq = true;
                for (std::size_t k = 0; k < ls[i].size(); ++k)
                    if (ls[i][k] != ls[j][k]) { eq = false; break; }
                if (eq) return false;
            }
        return true;
    }();
    static_assert(labels_unique,
        "oneof_by_parent_field: labels must be unique.");

    static constexpr std::size_t alt_for_label(std::string_view s) noexcept {
        for (std::size_t i = 0; i < num_alts; ++i) {
            const auto& l = labels[i];
            if (l.size() != s.size()) continue;
            bool eq = true;
            for (std::size_t k = 0; k < l.size(); ++k)
                if (l[k] != s[k]) { eq = false; break; }
            if (eq) return i;
        }
        return static_cast<std::size_t>(-1);
    }

    static constexpr oneof_by_parent_field _make_default() {
        return oneof_by_parent_field(std::tuple_element_t<0, alts_tuple>{});
    }
};

// ─── trait probes ──────────────────────────────────────────────────────────

namespace detail {

    template <typename T> inline constexpr bool is_oneof_v = false;
    template <typename... Args>
    inline constexpr bool is_oneof_v<oneof<Args...>> = true;

    template <typename T> inline constexpr bool is_oneof_by_field_v = false;
    template <fixed_string D, typename... A>
    inline constexpr bool is_oneof_by_field_v<oneof_by_field<D, A...>> = true;

    template <typename T> inline constexpr bool is_oneof_by_parent_field_v = false;
    template <auto P, typename... A>
    inline constexpr bool is_oneof_by_parent_field_v<oneof_by_parent_field<P, A...>> = true;

    template <typename T>
    inline constexpr bool is_any_oneof_v =
        is_oneof_v<T> || is_oneof_by_field_v<T> || is_oneof_by_parent_field_v<T>;

}  // namespace detail
}  // namespace def_type
