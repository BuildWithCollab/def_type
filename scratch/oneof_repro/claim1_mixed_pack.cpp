// Claim under test: a single class-template parameter pack cannot mix types
// and values. Test whether `template <typename... Ts, auto... Vs>` is
// even a valid class-template parameter list, and whether you can call it
// as `Y<int, double, 1, 2>`.

template <typename... Ts, auto... Vs>
struct Y {};

int main() {
    Y<int, double, 1, 2> y;   // compiler picks where types end and values begin?
    (void)y;
    return 0;
}
