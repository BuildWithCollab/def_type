// V7: control test. Foo is a TYPE-parameter template (not auto NTTPs) that
// inherits from carrier<T>. If ADL works here but fails in V5, the failure
// is specifically tied to the NTTP path.
//
//   cl /std:c++latest /EHsc v7_template_type_param.cpp

#include <cstdio>

namespace ns_x {
    struct Bar {};
    template <typename T> void hello(const T&) { std::puts("ns_x::hello via ADL"); }
}

template <typename... Ts> struct carrier {};

template <typename T>
struct Foo : carrier<T> {};

int main() {
    Foo<ns_x::Bar> f{};
    hello(f);
    return 0;
}
