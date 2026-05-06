// Test: does using the same `alt<T, "label">` (CNTTP form) more than once in
// a single TU cause issues — duplicate COMDAT, link errors, ODR weirdness,
// or just work fine?
//
// Header-only context (no modules). T is std::string (non-trivially-
// destructible).
//
//   cl /std:c++latest /EHsc nttp_repeat.cpp

#include <cstddef>
#include <string>

template <std::size_t N>
struct fixed_string {
    char data[N]{};
    constexpr fixed_string(const char (&s)[N]) {
        for (std::size_t i = 0; i < N; ++i) data[i] = s[i];
    }
};
template <std::size_t N> fixed_string(const char (&)[N]) -> fixed_string<N>;

struct TextContent { std::string text; };
struct ImageContent { std::string url; };

// alt is a class template with a TYPE param AND a class-type NTTP.
template <typename T, fixed_string Label>
struct alt {
    T value{};
    static constexpr auto label = Label;
};

// Use the same specialization alt<TextContent, "text"> in multiple places
// in the same TU.
struct A { alt<TextContent, "text"> a; };
struct B { alt<TextContent, "text"> b; };
struct C { alt<TextContent, "text"> c; };

// Multiple direct mentions in the same function:
void use_them() {
    alt<TextContent, "text"> x1;
    alt<TextContent, "text"> x2;
    alt<TextContent, "text"> x3;
    (void)x1; (void)x2; (void)x3;
}

int main() {
    A a; B b; C c;
    use_them();
    return 0;
}
