// V4: empty alt_tag, Foo inherits from each alt_tag<T> using `::type` extraction.
//
//   cl /std:c++latest /EHsc v4_foo_inherits_alt_tag.cpp

#include <cstdio>
#include <type_traits>

namespace ns_x {
    struct Bar {};
    template <typename T> void hello(const T&) { std::puts("ns_x::hello via ADL"); }
}

template <typename T> struct alt_tag { using type = T; };
template <typename T> inline constexpr alt_tag<T> alt{};

template <auto... Args>
struct Foo : alt_tag<typename std::remove_cvref_t<decltype(Args)>::type>... {};

int main() {
    Foo<alt<ns_x::Bar>> f{};
    hello(f);
    return 0;
}
