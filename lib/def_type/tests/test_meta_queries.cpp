#include "test_model_types.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// has_meta() — detects present metas
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: has_meta() detects present metas", "[type_def][typed][has_meta]") {
    type_def<Dog> t;
    REQUIRE(t.has_meta<endpoint_info>());
    REQUIRE(t.has_meta<help_info>());
}

TEST_CASE("hybrid: has_meta() detects present metas", "[type_def][hybrid][has_meta]") {
    type_def<MetaDog> t;
    REQUIRE(t.has_meta<help_info>());
}

TEST_CASE("dynamic: has_meta()", "[type_def][dynamic][has_meta]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/events"});
    REQUIRE(t.has_meta<endpoint_info>());
    REQUIRE(!t.has_meta<help_info>());
}

TEST_CASE("type_instance: has_meta()", "[type_instance][has_meta]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/e"})
        .field<int>("x");
    auto obj = t.create();

    REQUIRE(obj.type().has_meta<endpoint_info>() == true);
    REQUIRE(obj.type().has_meta<tag_info>() == false);
}

// ═══════════════════════════════════════════════════════════════════════════
// has_meta() — returns false for absent metas
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: has_meta() returns false for absent metas", "[type_def][typed][has_meta]") {
    type_def<Dog> t;
    REQUIRE(!t.has_meta<tag_info>());
    REQUIRE(!t.has_meta<cli_meta>());
}

TEST_CASE("hybrid: has_meta() returns false for absent metas", "[type_def][hybrid][has_meta]") {
    type_def<MetaDog> t;
    REQUIRE(!t.has_meta<endpoint_info>());
}

TEST_CASE("dynamic: has_meta() false when no metas", "[type_def][dynamic][has_meta]") {
    auto t = type_def("Simple")
        .field<int>("x");
    REQUIRE(!t.has_meta<endpoint_info>());
}

// ═══════════════════════════════════════════════════════════════════════════
// has_meta() — struct with no metas
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: has_meta() returns false when struct has no metas", "[type_def][typed][has_meta]") {
    type_def<SimpleArgs> t;
    REQUIRE(!t.has_meta<endpoint_info>());
    REQUIRE(!t.has_meta<help_info>());
}

TEST_CASE("hybrid: has_meta() returns false when struct has no metas", "[type_def][hybrid][has_meta]") {
    type_def<PlainDog> t;
    REQUIRE(!t.has_meta<help_info>());
}

// ═══════════════════════════════════════════════════════════════════════════
// has_meta() — multi-tagged struct
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: has_meta() works for multi-tagged struct", "[type_def][typed][has_meta]") {
    type_def<MultiTagged> t;
    REQUIRE(t.has_meta<tag_info>());
    REQUIRE(!t.has_meta<endpoint_info>());
}

TEST_CASE("hybrid: has_meta() works for multi-tagged struct", "[type_def][hybrid][has_meta]") {
    type_def<MultiTagDog> t;
    REQUIRE(t.has_meta<tag_info>());
    REQUIRE(!t.has_meta<endpoint_info>());
}

TEST_CASE("dynamic: has_meta() works for multi-tagged", "[type_def][dynamic][has_meta]") {
    auto t = type_def("MultiTag")
        .meta<tag_info>({.value = "a"})
        .meta<tag_info>({.value = "b"})
        .meta<tag_info>({.value = "c"});
    REQUIRE(t.has_meta<tag_info>());
    REQUIRE(!t.has_meta<endpoint_info>());
}

// ═══════════════════════════════════════════════════════════════════════════
// meta() — returns the metadata value
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: meta() returns the metadata value", "[type_def][typed][meta]") {
    type_def<Dog> t;
    auto ep = t.meta<endpoint_info>();
    REQUIRE(std::string_view{ep.path} == "/dogs");
    REQUIRE(std::string_view{ep.method} == "POST");
}

TEST_CASE("hybrid: meta() returns the metadata value", "[type_def][hybrid][meta]") {
    type_def<MetaDog> t;
    auto h = t.meta<help_info>();
    REQUIRE(std::string_view{h.summary} == "A dog");
}

TEST_CASE("dynamic: meta()", "[type_def][dynamic][meta]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/events", .method = "POST"});
    auto ep = t.meta<endpoint_info>();
    REQUIRE(std::string_view{ep.path} == "/events");
    REQUIRE(std::string_view{ep.method} == "POST");
}

TEST_CASE("type_instance: meta()", "[type_instance][meta]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/e"})
        .field<int>("x");
    auto obj = t.create();

    REQUIRE(std::string_view{obj.type().meta<endpoint_info>().path} == "/e");
}

// ═══════════════════════════════════════════════════════════════════════════
// meta() — returns different metadata types
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: meta() returns different metadata types", "[type_def][typed][meta]") {
    type_def<Dog> t;
    auto h = t.meta<help_info>();
    REQUIRE(std::string_view{h.summary} == "A good boy");
}

TEST_CASE("hybrid: meta() returns different metadata types", "[type_def][hybrid][meta]") {
    type_def<MetaDog> t;
    auto h = t.meta<help_info>();
    REQUIRE(std::string_view{h.summary} == "A dog");
}

TEST_CASE("dynamic: meta() returns different metadata types", "[type_def][dynamic][meta]") {
    auto t = type_def("Multi")
        .meta<endpoint_info>({.path = "/multi", .method = "PUT"})
        .meta<help_info>({.summary = "multi endpoint"});
    auto ep = t.meta<endpoint_info>();
    REQUIRE(std::string_view{ep.path} == "/multi");
    REQUIRE(std::string_view{ep.method} == "PUT");
    auto h = t.meta<help_info>();
    REQUIRE(std::string_view{h.summary} == "multi endpoint");
}

// ═══════════════════════════════════════════════════════════════════════════
// meta() — returns first when multiple of same type
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: meta() returns first when multiple of same type", "[type_def][typed][meta]") {
    type_def<MultiTagged> t;
    auto tag = t.meta<tag_info>();
    REQUIRE(std::string_view{tag.value} == "pet");
}

TEST_CASE("hybrid: meta() returns first when multiple of same type", "[type_def][hybrid][meta]") {
    type_def<MultiTagDog> t;
    REQUIRE(std::string_view{t.meta<tag_info>().value} == "pet");
}

TEST_CASE("dynamic: meta() returns first when multiple", "[type_def][dynamic][meta]") {
    auto t = type_def("Tagged")
        .meta<tag_info>({.value = "first"})
        .meta<tag_info>({.value = "second"});
    REQUIRE(std::string_view{t.meta<tag_info>().value} == "first");
}

// ═══════════════════════════════════════════════════════════════════════════
// meta_count()
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: meta_count() returns correct count", "[type_def][typed][meta_count]") {
    REQUIRE(type_def<Dog>{}.meta_count<endpoint_info>() == 1);
    REQUIRE(type_def<Dog>{}.meta_count<help_info>() == 1);
    REQUIRE(type_def<Dog>{}.meta_count<tag_info>() == 0);
}

TEST_CASE("typed: meta_count() for multiple metas of same type", "[type_def][typed][meta_count]") {
    REQUIRE(type_def<MultiTagged>{}.meta_count<tag_info>() == 3);
}

TEST_CASE("typed: meta_count() is zero for no-meta struct", "[type_def][typed][meta_count]") {
    REQUIRE(type_def<SimpleArgs>{}.meta_count<endpoint_info>() == 0);
}

TEST_CASE("hybrid: meta_count()", "[type_def][hybrid][meta_count]") {
    type_def<MetaDog> t;
    REQUIRE(t.meta_count<help_info>() == 1);
    REQUIRE(t.meta_count<endpoint_info>() == 0);
}

TEST_CASE("hybrid: meta_count() is zero for no-meta struct", "[type_def][hybrid][meta_count]") {
    type_def<PlainDog> t;
    REQUIRE(t.meta_count<help_info>() == 0);
}

TEST_CASE("hybrid: meta_count() for multiple metas of same type", "[type_def][hybrid][meta_count]") {
    type_def<MultiTagDog> t;
    REQUIRE(t.meta_count<tag_info>() == 2);
    REQUIRE(t.meta_count<help_info>() == 1);
}

TEST_CASE("dynamic: meta_count()", "[type_def][dynamic][meta_count]") {
    auto t = type_def("Tagged")
        .meta<tag_info>({.value = "a"})
        .meta<tag_info>({.value = "b"})
        .meta<tag_info>({.value = "c"});
    REQUIRE(t.meta_count<tag_info>() == 3);
    REQUIRE(t.meta_count<endpoint_info>() == 0);
}

TEST_CASE("type_instance: meta_count()", "[type_instance][meta_count]") {
    auto t = type_def("Event")
        .meta<tag_info>({.value = "a"})
        .meta<tag_info>({.value = "b"})
        .field<int>("x");
    auto obj = t.create();

    REQUIRE(obj.type().meta_count<tag_info>() == 2);
}

// ═══════════════════════════════════════════════════════════════════════════
// metas()
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: metas() returns all metas of a given type", "[type_def][typed][metas]") {
    type_def<MultiTagged> t;
    auto tags = t.metas<tag_info>();
    REQUIRE(tags.size() == 3);
    REQUIRE(std::string_view{tags[0].value} == "pet");
    REQUIRE(std::string_view{tags[1].value} == "animal");
    REQUIRE(std::string_view{tags[2].value} == "good-boy");
}

TEST_CASE("hybrid: metas() returns all metas of a given type", "[type_def][hybrid][metas]") {
    type_def<MetaDog> t;
    auto helps = t.metas<help_info>();
    REQUIRE(helps.size() == 1);
}

TEST_CASE("typed: metas() returns single-element vector for one meta", "[type_def][typed][metas]") {
    type_def<Dog> t;
    auto eps = t.metas<endpoint_info>();
    REQUIRE(eps.size() == 1);
    REQUIRE(std::string_view{eps[0].path} == "/dogs");
}

TEST_CASE("hybrid: metas() returns single-element vector", "[type_def][hybrid][metas]") {
    type_def<MetaDog> t;
    auto helps = t.metas<help_info>();
    REQUIRE(helps.size() == 1);
    REQUIRE(std::string_view{helps[0].summary} == "A dog");
}

TEST_CASE("dynamic: metas()", "[type_def][dynamic][metas]") {
    auto t = type_def("Tagged")
        .meta<tag_info>({.value = "a"})
        .meta<tag_info>({.value = "b"});
    auto tags = t.metas<tag_info>();
    REQUIRE(tags.size() == 2);
    REQUIRE(std::string_view{tags[0].value} == "a");
    REQUIRE(std::string_view{tags[1].value} == "b");
}

TEST_CASE("type_instance: metas()", "[type_instance][metas]") {
    auto t = type_def("Event")
        .meta<tag_info>({.value = "a"})
        .meta<tag_info>({.value = "b"})
        .field<int>("x");
    auto obj = t.create();

    auto tags = obj.type().metas<tag_info>();
    REQUIRE(tags.size() == 2);
    REQUIRE(std::string_view{tags[0].value} == "a");
    REQUIRE(std::string_view{tags[1].value} == "b");
}

TEST_CASE("dynamic: metas() returns single-element vector", "[type_def][dynamic][metas]") {
    auto t = type_def("Single")
        .meta<endpoint_info>({.path = "/single", .method = "GET"});
    auto eps = t.metas<endpoint_info>();
    REQUIRE(eps.size() == 1);
    REQUIRE(std::string_view{eps[0].path} == "/single");
}

TEST_CASE("typed: metas() returns empty vector for absent type", "[type_def][typed][metas]") {
    type_def<Dog> t;
    auto tags = t.metas<tag_info>();
    REQUIRE(tags.size() == 0);
}

TEST_CASE("hybrid: metas() returns empty vector for absent type", "[type_def][hybrid][metas]") {
    type_def<MetaDog> t;
    auto eps = t.metas<endpoint_info>();
    REQUIRE(eps.size() == 0);
}

TEST_CASE("dynamic: metas() returns empty vector for absent type", "[type_def][dynamic][metas]") {
    auto t = type_def("Event")
        .field<int>("x");
    auto tags = t.metas<tag_info>();
    REQUIRE(tags.size() == 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// has_meta() — false for absent metas on meta-bearing type
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic: has_meta() returns false for absent metas on meta-bearing type", "[type_def][dynamic][has_meta]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/e"});
    REQUIRE(!t.has_meta<help_info>());
}

// ═══════════════════════════════════════════════════════════════════════════
// Meta-only struct
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: meta-only struct has zero fields but metas work", "[type_def][typed][meta]") {
    type_def<MetaOnly> t;
    REQUIRE(t.field_count() == 0);
    REQUIRE(t.has_meta<endpoint_info>());
    auto ep = t.meta<endpoint_info>();
    REQUIRE(std::string_view{ep.path} == "/health");
}

TEST_CASE("hybrid: meta-only struct has zero fields but metas work", "[type_def][hybrid][meta]") {
    type_def<MetaOnly> t;
    REQUIRE(t.field_count() == 0);
    REQUIRE(t.has_meta<endpoint_info>());
    auto ep = t.meta<endpoint_info>();
    REQUIRE(std::string_view{ep.path} == "/health");
}

TEST_CASE("dynamic: meta-only type_def has zero fields but metas work", "[type_def][dynamic][meta]") {
    auto t = type_def("Health")
        .meta<endpoint_info>({.path = "/health"});
    REQUIRE(t.field_count() == 0);
    REQUIRE(t.has_meta<endpoint_info>());
    auto ep = t.meta<endpoint_info>();
    REQUIRE(std::string_view{ep.path} == "/health");
}

// ═══════════════════════════════════════════════════════════════════════════
// meta() throws for absent meta
// ═══════════════════════════════════════════════════════════════════════════

// typed/hybrid meta<M>() for absent types is compile-time — returns default-constructed M,
// does not throw. Only the dynamic path throws at runtime.

TEST_CASE("dynamic: meta() throws for absent meta", "[type_def][dynamic][meta][throw]") {
    auto t = type_def("Event")
        .field<int>("x");

    REQUIRE_THROWS_AS(t.meta<endpoint_info>(), std::logic_error);
}
