// V1: empty alt_tag — ADL through NTTP value's class-template TYPE arg only.
// Matches the current oneof code's alt_tag<T> shape (no member, no base).
//
//   cl /std:c++latest /EHsc v1_empty_alt.cpp

#include <cstdio>

namespace ns_x {
    struct Bar {};
    template <typename T> void hello(const T&) { std::puts("ns_x::hello via ADL"); }
}

template <typename T> struct alt_tag {};                  // empty
template <typename T> inline constexpr alt_tag<T> alt{};

template <auto... Args> struct Foo {};

int main() {
    Foo<alt<ns_x::Bar>> f{};
    hello(f);   // unqualified — ADL only
    return 0;
}
