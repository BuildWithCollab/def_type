#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

#include <def_type.hpp>

using namespace def_type;

// ═════════════════════════════════════════════════════════════════════════
// Test structs
// ═════════════════════════════════════════════════════════════════════════

struct SchemaTestDog {
    field<std::string> name;
    field<int>         age;
};

struct PlainCat {
    std::string name;
    int         age = 0;
};

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<SchemaTestDog>() {
    return def_type::field_info<SchemaTestDog>("name", "age");
}

template <>
constexpr auto def_type::struct_info<PlainCat>() {
    return def_type::field_info<PlainCat>("name", "age");
}
#endif

// ═════════════════════════════════════════════════════════════════════════
// Static assertions: all three modes satisfy type_definition
// ═════════════════════════════════════════════════════════════════════════

static_assert(type_definition<type_def<SchemaTestDog>>,
    "typed type_def<T> must satisfy type_definition");

static_assert(type_definition<type_def<detail::dynamic_tag>>,
    "dynamic type_def must satisfy type_definition");

// ═════════════════════════════════════════════════════════════════════════
// Tests: generic function constrained by type_definition
// ═════════════════════════════════════════════════════════════════════════

std::string schema_summary(const type_definition auto& t) {
    std::string result = std::string(t.name()) + ": ";
    auto names = t.field_names();
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) result += ", ";
        result += names[i];
    }
    return result;
}

TEST_CASE("type_definition works with typed type_def", "[type_definition]") {
    type_def<SchemaTestDog> t;
    auto summary = schema_summary(t);
    REQUIRE(summary == "SchemaTestDog: name, age");
}

TEST_CASE("type_definition works with dynamic type_def", "[type_definition]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count");
    auto summary = schema_summary(t);
    REQUIRE(summary == "Event: title, count");
}

TEST_CASE("type_definition works with hybrid type_def", "[type_definition]") {
    auto t = type_def<PlainCat>()
        .field(&PlainCat::name, "name")
        .field(&PlainCat::age, "age");
    auto summary = schema_summary(t);
    REQUIRE(summary == "PlainCat: name, age");
}

TEST_CASE("type_definition has_field works generically", "[type_definition]") {
    auto check_has = [](const type_definition auto& t, std::string_view name) {
        return t.has_field(name);
    };

    type_def<SchemaTestDog> typed;
    REQUIRE(check_has(typed, "name"));
    REQUIRE(!check_has(typed, "nope"));

    auto dynamic = type_def("Thing")
        .field<int>("x");
    REQUIRE(check_has(dynamic, "x"));
    REQUIRE(!check_has(dynamic, "nope"));

    auto hybrid = type_def<PlainCat>()
        .field(&PlainCat::name, "name");
    REQUIRE(check_has(hybrid, "name"));
    REQUIRE(!check_has(hybrid, "nope"));
}

TEST_CASE("type_definition has_meta works generically", "[type_definition]") {
    struct test_meta { const char* info = ""; };

    auto check_meta = [](const type_definition auto& t) {
        return t.template has_meta<test_meta>();
    };

    // Dynamic with meta
    auto with_meta = type_def("Event")
        .meta<test_meta>({.info = "hello"});
    REQUIRE(check_meta(with_meta));

    // Dynamic without meta
    auto without_meta = type_def("Plain")
        .field<int>("x");
    REQUIRE(!check_meta(without_meta));
}

TEST_CASE("type_definition field() query works generically", "[type_definition]") {
    auto get_field = [](const type_definition auto& t, std::string_view name) {
        return t.field(name);
    };

    type_def<SchemaTestDog> typed;
    REQUIRE(get_field(typed, "name").name() == "name");

    auto dynamic = type_def("Event")
        .field<std::string>("title");
    REQUIRE(get_field(dynamic, "title").name() == "title");
}
