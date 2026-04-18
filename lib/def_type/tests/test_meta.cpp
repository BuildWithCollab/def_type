#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <type_traits>

#include <def_type/def_type.hpp>

using namespace def_type;

// ── Test metadata types ──────────────────────────────────────────────────

struct endpoint_info {
    const char* path   = "";
    const char* method = "GET";
};

struct help_info {
    const char* summary = "";
};

struct tag_info {
    const char* value = "";
};

// ── Default construction ─────────────────────────────────────────────────

TEST_CASE("meta default construction", "[meta]") {
    meta<endpoint_info> ep;
    REQUIRE(std::string_view{ep.value.path} == "");
    REQUIRE(std::string_view{ep.value.method} == "GET");
}

TEST_CASE("meta default construction with int", "[meta]") {
    meta<int> m;
    REQUIRE(m.value == 0);
}

// ── Aggregate initialization ─────────────────────────────────────────────

TEST_CASE("meta aggregate init with designated initializer", "[meta][init]") {
    meta<endpoint_info> ep{.value = {.path = "/dogs", .method = "POST"}};
    REQUIRE(std::string_view{ep.value.path} == "/dogs");
    REQUIRE(std::string_view{ep.value.method} == "POST");
}

TEST_CASE("meta aggregate init with brace elision", "[meta][init]") {
    meta<endpoint_info> ep{{.path = "/cats", .method = "PUT"}};
    REQUIRE(std::string_view{ep.value.path} == "/cats");
    REQUIRE(std::string_view{ep.value.method} == "PUT");
}

TEST_CASE("meta aggregate init with simple type", "[meta][init]") {
    meta<int> m{42};
    REQUIRE(m.value == 42);
}

// ── Value access ─────────────────────────────────────────────────────────

TEST_CASE("meta value read and write", "[meta][access]") {
    meta<help_info> h{{.summary = "A good boy"}};
    REQUIRE(std::string_view{h.value.summary} == "A good boy");

    h.value.summary = "The best boy";
    REQUIRE(std::string_view{h.value.summary} == "The best boy");
}

// ── operator-> ───────────────────────────────────────────────────────────

TEST_CASE("meta operator-> gives pointer to inner value", "[meta][arrow]") {
    meta<endpoint_info> ep{{.path = "/events", .method = "DELETE"}};
    REQUIRE(std::string_view{ep->path} == "/events");
    REQUIRE(std::string_view{ep->method} == "DELETE");
}

TEST_CASE("meta const operator-> works", "[meta][arrow]") {
    const meta<endpoint_info> ep{{.path = "/users", .method = "PATCH"}};
    REQUIRE(std::string_view{ep->path} == "/users");
    REQUIRE(std::string_view{ep->method} == "PATCH");
}

TEST_CASE("meta operator-> allows mutation", "[meta][arrow]") {
    meta<tag_info> t{{.value = "old"}};
    t->value = "new";
    REQUIRE(std::string_view{t->value} == "new");
}

// ── is_meta concept ──────────────────────────────────────────────────────

TEST_CASE("is_meta detects meta specializations", "[meta][concept]") {
    STATIC_REQUIRE(is_meta<meta<endpoint_info>>);
    STATIC_REQUIRE(is_meta<meta<help_info>>);
    STATIC_REQUIRE(is_meta<meta<int>>);
    STATIC_REQUIRE(is_meta<meta<std::string>>);
}

TEST_CASE("is_meta rejects non-meta types", "[meta][concept]") {
    STATIC_REQUIRE(!is_meta<int>);
    STATIC_REQUIRE(!is_meta<std::string>);
    STATIC_REQUIRE(!is_meta<endpoint_info>);
    STATIC_REQUIRE(!is_meta<def_type::field<int>>);
    STATIC_REQUIRE(!is_meta<def_type::field<std::string>>);
}

TEST_CASE("is_meta handles cv-qualified and reference types", "[meta][concept]") {
    STATIC_REQUIRE(is_meta<const meta<int>>);
    STATIC_REQUIRE(is_meta<volatile meta<int>>);
    STATIC_REQUIRE(is_meta<const volatile meta<int>>);
    STATIC_REQUIRE(is_meta<meta<int>&>);
    STATIC_REQUIRE(is_meta<const meta<int>&>);
    STATIC_REQUIRE(is_meta<meta<int>&&>);
}

// ── meta is an aggregate ─────────────────────────────────────────────────

TEST_CASE("meta is an aggregate type", "[meta][aggregate]") {
    STATIC_REQUIRE(std::is_aggregate_v<meta<endpoint_info>>);
    STATIC_REQUIRE(std::is_aggregate_v<meta<int>>);
    STATIC_REQUIRE(std::is_aggregate_v<meta<help_info>>);
}

// ── meta on a struct — demonstrating intended usage ──────────────────────

TEST_CASE("meta as a struct member for type-level metadata", "[meta][usage]") {
    struct Dog {
        meta<endpoint_info> endpoint{{.path = "/dogs", .method = "POST"}};
        meta<help_info>     help{{.summary = "A good boy"}};
    };

    Dog d;
    REQUIRE(std::string_view{d.endpoint->path} == "/dogs");
    REQUIRE(std::string_view{d.endpoint->method} == "POST");
    REQUIRE(std::string_view{d.help->summary} == "A good boy");
}

TEST_CASE("multiple metas of the same type on a struct", "[meta][usage]") {
    struct TaggedThing {
        meta<tag_info> tag1{{.value = "pet"}};
        meta<tag_info> tag2{{.value = "animal"}};
        meta<tag_info> tag3{{.value = "good-boy"}};
    };

    TaggedThing t;
    REQUIRE(std::string_view{t.tag1->value} == "pet");
    REQUIRE(std::string_view{t.tag2->value} == "animal");
    REQUIRE(std::string_view{t.tag3->value} == "good-boy");
}
