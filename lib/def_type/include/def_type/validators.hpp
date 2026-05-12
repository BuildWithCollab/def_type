#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace def_type {

// ── type_validators<Vs...> — struct-level validator marker member ────────
//
// Attach whole-struct validators by making it a member of a reflected struct:
//
//   struct SignupForm {
//       type_validators<passwords_match, terms_accepted> rules {
//           passwords_match{},
//           terms_accepted{.required = true}
//       };
//       field<std::string> password;
//       field<std::string> confirm;
//   };
//
// Discovery treats type_validators<> members the same way it treats meta<>:
// they are excluded from field_count(), field_names(), to_json, set/get,
// for_each_field — invisible to everything except validation.

template <typename... Vs>
struct type_validators {
    std::tuple<Vs...> rules{};

    constexpr type_validators() = default;

    template <typename... Args>
        requires (sizeof...(Args) == sizeof...(Vs)) && (sizeof...(Args) > 0)
    constexpr type_validators(Args&&... args)
        : rules{std::forward<Args>(args)...} {}
};

// ── is_type_validators concept ───────────────────────────────────────────

namespace detail {

    template <typename U>
    inline constexpr bool is_type_validators_v = false;

    template <typename... Vs>
    inline constexpr bool is_type_validators_v<type_validators<Vs...>> = true;

    // Count of validators stored in a single type_validators<> member.
    template <typename U>
    inline constexpr std::size_t type_validators_size_v = 0;

    template <typename... Vs>
    inline constexpr std::size_t type_validators_size_v<type_validators<Vs...>> = sizeof...(Vs);

}  // namespace detail

template <typename T>
concept is_type_validators = detail::is_type_validators_v<std::remove_cvref_t<T>>;

}  // namespace def_type
