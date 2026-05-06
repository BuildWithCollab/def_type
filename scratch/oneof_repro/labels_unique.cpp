// Test: can we compare two fixed_string NTTPs byte-by-byte at constexpr without
// going through std::string_view?
//
//   cl /std:c++latest /EHsc labels_unique.cpp

#include <cstddef>

template <std::size_t N>
struct fixed_string {
    char data[N]{};
    constexpr fixed_string(const char (&s)[N]) {
        for (std::size_t i = 0; i < N; ++i) data[i] = s[i];
    }
};
template <std::size_t N> fixed_string(const char (&)[N]) -> fixed_string<N>;

template <auto V> struct vbox { static constexpr auto value = V; };

template <std::size_t I, auto... Args>
constexpr const auto& pack_value() {
    return vbox<__builtin_FUNCTION>::value;  // placeholder, will replace
}

// Real impl:
#include <tuple>
template <std::size_t I, auto... Args>
constexpr const auto& pack_value_real() {
    return std::tuple_element_t<I, std::tuple<vbox<Args>...>>::value;
}

template <auto A, auto B>
constexpr bool fixed_string_equal_v = []{
    using TA = decltype(A);
    using TB = decltype(B);
    if constexpr (sizeof(A.data) != sizeof(B.data)) {
        return false;
    } else {
        for (std::size_t i = 0; i < sizeof(A.data); ++i)
            if (A.data[i] != B.data[i]) return false;
        return true;
    }
}();

int main() {
    static_assert(  fixed_string_equal_v<fixed_string{"text"},  fixed_string{"text"}>);
    static_assert(! fixed_string_equal_v<fixed_string{"text"},  fixed_string{"image"}>);
    static_assert(! fixed_string_equal_v<fixed_string{"text"},  fixed_string{"foo"}>);
    static_assert(! fixed_string_equal_v<fixed_string{"text"},  fixed_string{"abcd"}>);
    return 0;
}
