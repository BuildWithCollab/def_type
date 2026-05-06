#pragma once

#include <cstddef>
#include <string_view>

namespace def_type::detail {

    // ── fixed_string — minimal CNTTP literal ───────────────────────────────
    //
    // Used for label NTTPs on `oneof_type<T, "label">` and for the
    // discriminator key on `oneof_by_field<"key", ...>`.

    template <std::size_t N>
    struct fixed_string {
        char data_[N]{};

        constexpr fixed_string(const char (&s)[N]) {
            for (std::size_t i = 0; i < N; ++i) data_[i] = s[i];
        }

        constexpr std::string_view view() const noexcept {
            return std::string_view(data_, N > 0 ? N - 1 : 0);
        }
    };

    template <std::size_t N>
    fixed_string(const char (&)[N]) -> fixed_string<N>;

}  // namespace def_type::detail
