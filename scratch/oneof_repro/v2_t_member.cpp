// V2: alt_tag carries a T value member.
//
//   cl /std:c++latest /EHsc v2_t_member.cpp

#include <cstdio>

namespace ns_x {
    struct Bar {};
    template <typename T> void hello(const T&) { std::puts("ns_x::hello via ADL"); }
}

template <typename T> struct alt_tag { T value{}; };      // T as data member
template <typename T> inline constexpr alt_tag<T> alt{};

template <auto... Args> struct Foo {};

int main() {
    Foo<alt<ns_x::Bar>> f{};
    hello(f);
    return 0;
}
