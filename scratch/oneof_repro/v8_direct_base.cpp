// V8: control. Foo (non-template) inherits directly from Bar.
//
//   cl /std:c++latest /EHsc v8_direct_base.cpp

#include <cstdio>

namespace ns_x {
    struct Bar {};
    template <typename T> void hello(const T&) { std::puts("ns_x::hello via ADL"); }
}

struct Foo : ns_x::Bar {};

int main() {
    Foo f{};
    hello(f);
    return 0;
}
