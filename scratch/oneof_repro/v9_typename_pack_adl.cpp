// V9: pure typename pack — does ADL propagate ALL alt namespaces?
// Tests the actual new design shape: template <typename... Args> class oneof.
//
//   cl /std:c++latest /EHsc v9_typename_pack_adl.cpp

#include <cstdio>

namespace ns_a { struct Apple {}; template <typename T> void hello(const T&) { std::puts("ns_a::hello"); } }
namespace ns_b { struct Banana {}; template <typename T> void hi(const T&)    { std::puts("ns_b::hi"); } }

template <typename... Args>
struct oneof {};

int main() {
    oneof<ns_a::Apple, ns_b::Banana> o;
    hello(o);  // ADL via ns_a::Apple
    hi(o);     // ADL via ns_b::Banana
    return 0;
}
