#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <def_type.hpp>

using namespace def_type;

// ── Domain-specific extension structs ───────────────────────────────────

struct posix_options {
    char short_flag = '\0';
    bool from_stdin = false;
    bool positional = false;
};

struct render_options {
    const char* style = "";
    int         width = 0;
};

struct posix_meta  { posix_options posix{}; };
struct render_meta { render_options render{}; };

// ── struct_info fallbacks (non-PFR builds) ──────────────────────────────

struct WeatherArgs;
struct OnlyFields;
struct MixedStruct;
struct LoginResponse;

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<WeatherArgs>() {
    return def_type::field_info<WeatherArgs>("city", "days", "verbose", "tags");
}

template <>
constexpr auto def_type::struct_info<OnlyFields>() {
    return def_type::field_info<OnlyFields>("a", "b");
}

template <>
constexpr auto def_type::struct_info<MixedStruct>() {
    return def_type::field_info<MixedStruct>("a", "plain", "b", "helper");
}

template <>
constexpr auto def_type::struct_info<LoginResponse>() {
    return def_type::field_info<LoginResponse>("session_id", "user_name");
}
#endif

// ── Basic instantiation and defaults ────────────────────────────────────

TEST_CASE("Field default construction", "[field]") {
    field<int> fi;
    REQUIRE(fi.value == 0);

    field<std::string> fs;
    REQUIRE(fs.value.empty());

    field<bool> fb;
    REQUIRE(fb.value == false);
}

// ── Transparent conversion ──────────────────────────────────────────────

TEST_CASE("Field transparent conversion", "[field][conversion]") {
    field<int> f;
    f = 42;
    int y = f;
    REQUIRE(y == 42);

    field<std::string> g;
    g = "hello";
    const std::string& ref = g;
    REQUIRE(ref == "hello");
}

TEST_CASE("Field round-trips via implicit conversion", "[field][conversion]") {
    field<int> f;
    f = 7;
    int via_conv = f;
    f = via_conv + 1;
    REQUIRE(f.value == 8);
}

// ── operator-> passthrough ──────────────────────────────────────────────

TEST_CASE("Field operator->", "[field][arrow]") {
    field<std::string> f;
    f = "hello";
    REQUIRE(f->size() == 5);

    field<std::string> e;
    REQUIRE(e->empty());

    field<std::vector<int>> v;
    v.value = {1, 2, 3};
    REQUIRE(v->size() == 3);
}

// ── with<> extensions ───────────────────────────────────────────────────

TEST_CASE("Field with single extension", "[field][with]") {
    struct Args {
        field<std::string, with<posix_meta>> query {
            .with = {{.posix = {.from_stdin = true}}}
        };
    };

    Args a{};
    REQUIRE(a.query.with.posix.from_stdin == true);
    REQUIRE(a.query.with.posix.short_flag == '\0');
}

TEST_CASE("Field with multiple extensions", "[field][with]") {
    struct Args {
        field<bool, with<posix_meta, render_meta>> verbose {
            .with = {{.posix = {.short_flag = 'v'}}, {.render = {.style = "dimmed"}}}
        };
    };

    Args a{};
    REQUIRE(a.verbose.with.posix.short_flag == 'v');
    REQUIRE(std::string_view{a.verbose.with.render.style} == "dimmed");
}

// ── Outer struct aggregate + designated init ────────────────────────────

struct WeatherArgs {
    field<std::string>                       city;
    field<int>                               days    {.value = 7};
    field<bool, with<posix_meta>>            verbose {.with = {{.posix = {.short_flag = 'v'}}}};
    field<std::vector<std::string>, with<posix_meta, render_meta>> tags {
        .with = {{.posix = {.short_flag = 't'}}, {.render = {.width = 20}}}
    };
};

TEST_CASE("Field in outer struct", "[field][aggregate]") {
    STATIC_REQUIRE(std::is_aggregate_v<WeatherArgs>);

    WeatherArgs w{};
    REQUIRE(w.city.value.empty());
    REQUIRE(w.days.value == 7);
    REQUIRE(w.verbose.value == false);
    REQUIRE(w.verbose.with.posix.short_flag == 'v');
    REQUIRE(w.tags.with.render.width == 20);
}

// ── Field names via reflect API ──────────────────────────────────────────

TEST_CASE("field_name extracts field names", "[field][reflect]") {
    constexpr auto n0 = detail::field_name<0, WeatherArgs>();
    constexpr auto n1 = detail::field_name<1, WeatherArgs>();
    constexpr auto n2 = detail::field_name<2, WeatherArgs>();
    constexpr auto n3 = detail::field_name<3, WeatherArgs>();

    REQUIRE(n0 == "city");
    REQUIRE(n1 == "days");
    REQUIRE(n2 == "verbose");
    REQUIRE(n3 == "tags");
}

// ── LNK1179 repro: same spec, multiple structs, one TU ─────────────────

struct LoginResponse {
    field<std::string> session_id;
    field<std::string> user_name;
};

struct LogoutResponse {
    field<std::string> session_id;
};

struct WhoAmI {
    field<std::string> session_id;
    field<std::string> display_name;
};

TEST_CASE("Same field<std::string> across multiple structs — no LNK1179", "[field][lnk1179]") {
    LoginResponse  login{};
    LogoutResponse logout{};
    WhoAmI         whoami{};

    REQUIRE(login.session_id.value.empty());
    REQUIRE(logout.session_id.value.empty());
    REQUIRE(whoami.session_id.value.empty());
}

// Same spec WITH extensions across multiple structs:
struct SearchArgs {
    field<std::string, with<posix_meta>> query {
        .with = {{.posix = {.from_stdin = true}}}
    };
};

struct GrepArgs {
    field<std::string, with<posix_meta>> query {
        .with = {{.posix = {.from_stdin = true}}}
    };
};

TEST_CASE("Same field<std::string, with<posix_meta>> across structs — no LNK1179", "[field][lnk1179]") {
    SearchArgs search{};
    GrepArgs   grep{};

    REQUIRE(search.query.with.posix.from_stdin == true);
    REQUIRE(grep.query.with.posix.from_stdin == true);
}

// ── is_field concept ─────────────────────────────────────────────────────

TEST_CASE("is_field detects Field specializations", "[field][concept]") {
    STATIC_REQUIRE(is_field<field<int>>);
    STATIC_REQUIRE(is_field<field<std::string>>);
    STATIC_REQUIRE(is_field<field<std::string, with<posix_meta>>>);
    STATIC_REQUIRE(!is_field<int>);
    STATIC_REQUIRE(!is_field<std::string>);
}

// ── reflected_struct concept ─────────────────────────────────────────────

struct OnlyFields {
    field<int>         a;
    field<std::string> b;
};

struct MixedStruct {
    field<int>           a;
    int                  plain = 0;
    field<std::string>   b;
    std::unique_ptr<int> helper;
};

struct EmptyStruct {};

struct NoFieldsOnlyPlain {
    int x = 0;
    int y = 0;
};

struct NonAggregate {
    NonAggregate() {}
    field<int> x;
};

TEST_CASE("reflected_struct accepts aggregates with Field members", "[field][concept]") {
    STATIC_REQUIRE(detail::reflected_struct<OnlyFields>);
    STATIC_REQUIRE(detail::reflected_struct<MixedStruct>);
    STATIC_REQUIRE(detail::reflected_struct<WeatherArgs>);
    STATIC_REQUIRE(detail::reflected_struct<LoginResponse>);
}

TEST_CASE("reflected_struct rejects non-qualifying types", "[field][concept]") {
    // int and NonAggregate fail is_aggregate_v, so they short-circuit
    // without ever hitting the PFR/registry fallback.
    STATIC_REQUIRE(!detail::reflected_struct<int>);
    STATIC_REQUIRE(!detail::reflected_struct<NonAggregate>);

#ifdef DEF_TYPE_HAS_PFR
    // EmptyStruct has zero members — not reflectable
    STATIC_REQUIRE(!detail::reflected_struct<EmptyStruct>);
#endif
}

TEST_CASE("reflected_struct accepts plain-member aggregates", "[field][concept]") {
#ifdef DEF_TYPE_HAS_PFR
    // Plain members (no field<>) ARE reflectable — only meta<> is skipped
    STATIC_REQUIRE(detail::reflected_struct<NoFieldsOnlyPlain>);
#endif
}
