// Claim under test: even if mixed packs work, can we instantiate with
// fully interleaved type/value/type/value/... arguments — i.e. the literal
// spec syntax `oneof<"type", TextContent, "text", ImageContent, "image">`?

#include <cstddef>

struct TextContent {};
struct ImageContent {};

template <std::size_t N>
struct fixed_string {
    char data[N]{};
    constexpr fixed_string(const char (&s)[N]) {
        for (std::size_t i = 0; i < N; ++i) data[i] = s[i];
    }
};
template <std::size_t N> fixed_string(const char (&)[N]) -> fixed_string<N>;

// Variant A: leading fixed_string, then mixed pack (won't compile: 2 packs)
template <fixed_string Disc, typename... Ts, auto... Vs>
struct OneofInside {};

int main() {
    // The actual spec syntax wanted by ONEOF.md:
    OneofInside<"type", TextContent, "text", ImageContent, "image"> o;
    (void)o;
    return 0;
}
