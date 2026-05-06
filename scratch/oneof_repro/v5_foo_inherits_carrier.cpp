// V5: empty alt_tag, Foo inherits from carrier<Ts...> templated on alt TYPES.
//
//   cl /std:c++latest /EHsc v5_foo_inherits_carrier.cpp

#include <cstdio>
#include <type_traits>

namespace ns_x {
    struct Bar {};
    template <typename T> void hello(const T&) { std::puts("ns_x::hello via ADL"); }
}

template <typename T> struct alt_tag { using type = T; };
template <typename T> inline constexpr alt_tag<T> alt{};

template <typename... Ts> struct carrier {};

template <auto... Args>
struct Foo : carrier<typename std::remove_cvref_t<decltype(Args)>::type...> {};

int main() {
    Foo<alt<ns_x::Bar>> f{};
    hello(f);
    return 0;
}
