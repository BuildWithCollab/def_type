// Claim under test: a bare string literal cannot be passed as an `auto`
// non-type template parameter.

template <auto V>
struct X {};

int main() {
    X<"hello"> x;
    (void)x;
    return 0;
}
