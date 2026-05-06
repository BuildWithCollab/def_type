// V6: control test. Foo is a NON-template that inherits from carrier<Bar>.
// If ADL finds ns_x::hello here, normal base-class ADL works on this compiler.
// If it doesn't, then ADL-through-bases is broken even without NTTPs involved.
//
//   cl /std:c++latest /EHsc v6_non_template_inherits_carrier.cpp

#include <cstdio>

namespace ns_x {
    struct Bar {};
    template <typename T> void hello(const T&) { std::puts("ns_x::hello via ADL"); }
}

template <typename... Ts> struct carrier {};

struct Foo : carrier<ns_x::Bar> {};

int main() {
    Foo f{};
    hello(f);   // unqualified — ADL through carrier<Bar> base
    return 0;
}
