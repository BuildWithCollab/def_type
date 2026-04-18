#include "test_model_types.hpp"

// ═════════════════════════════════════════════════════════════════════════
// Finds fields by name
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: has_field() finds field<> members by name", "[type_def][typed][has_field]") {
    type_def<Dog> t;
    REQUIRE(t.has_field("name"));
    REQUIRE(t.has_field("age"));
    REQUIRE(t.has_field("breed"));
    REQUIRE(!t.has_field("nope"));
}

TEST_CASE("hybrid: has_field()", "[type_def][hybrid][has_field]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age");
    REQUIRE(t.has_field("name"));
    REQUIRE(t.has_field("age"));
    REQUIRE(!t.has_field("breed"));
    REQUIRE(!t.has_field("nope"));
}

TEST_CASE("dynamic: has_field()", "[type_def][dynamic][has_field]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count");
    REQUIRE(t.has_field("title"));
    REQUIRE(t.has_field("count"));
    REQUIRE(!t.has_field("nope"));
    REQUIRE(!t.has_field(""));
}

TEST_CASE("type_instance: has_field() via type()", "[type_instance][has_field]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count");
    auto obj = t.create();

    REQUIRE(obj.type().has_field("title") == true);
    REQUIRE(obj.type().has_field("nope") == false);
}

// ═════════════════════════════════════════════════════════════════════════
// Rejects unknown names
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: has_field() returns false for unknown names", "[type_def][typed][has_field]") {
    type_def<Dog> t;
    REQUIRE(!t.has_field("nope"));
    REQUIRE(!t.has_field(""));
    REQUIRE(!t.has_field("Name"));
}

TEST_CASE("hybrid: has_field() returns false for unknown names", "[type_def][hybrid][has_field]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    REQUIRE(!t.has_field("nope"));
    REQUIRE(!t.has_field(""));
    REQUIRE(!t.has_field("Name"));
}

TEST_CASE("dynamic: has_field() returns false for unknown names", "[type_def][dynamic][has_field]") {
    auto t = type_def("Event")
        .field<int>("x");
    REQUIRE(!t.has_field("nope"));
    REQUIRE(!t.has_field(""));
    REQUIRE(!t.has_field("Title"));
}

// ═════════════════════════════════════════════════════════════════════════
// Rejects meta member names
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: has_field() returns false for meta member names", "[type_def][typed][has_field]") {
    type_def<Dog> t;
    REQUIRE(!t.has_field("endpoint"));
    REQUIRE(!t.has_field("help"));
}

TEST_CASE("hybrid: has_field() rejects meta member names", "[type_def][hybrid][has_field]") {
    type_def<MetaDog> t;
    REQUIRE(!t.has_field("help"));
    REQUIRE(t.has_field("name"));
    REQUIRE(t.has_field("age"));
}

// ═════════════════════════════════════════════════════════════════════════
// Rejects plain member names
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: has_field() returns false for plain member names", "[type_def][typed][has_field]") {
    type_def<MixedStruct> t;
    REQUIRE(t.has_field("label"));
    REQUIRE(t.has_field("score"));
    REQUIRE(!t.has_field("counter"));
}

TEST_CASE("hybrid: has_field() rejects unregistered members", "[type_def][hybrid][has_field]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age");
    REQUIRE(t.has_field("name"));
    REQUIRE(t.has_field("age"));
    REQUIRE(!t.has_field("breed"));
}

// ═════════════════════════════════════════════════════════════════════════
// Single-field struct
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: has_field() on single-field struct", "[type_def][typed][has_field]") {
    type_def<SingleField> t;
    REQUIRE(t.has_field("value"));
    REQUIRE(!t.has_field("other"));
}

TEST_CASE("hybrid: has_field() on single-field struct", "[type_def][hybrid][has_field]") {
    auto t = type_def<PlainPoint>()
        .field(&PlainPoint::x, "x");
    REQUIRE(t.has_field("x"));
    REQUIRE(!t.has_field("y"));
}

TEST_CASE("dynamic: has_field() on single-field struct", "[type_def][dynamic][has_field]") {
    auto t = type_def("Thing")
        .field<int>("value");
    REQUIRE(t.has_field("value"));
    REQUIRE(!t.has_field("other"));
}

// ═════════════════════════════════════════════════════════════════════════
// Meta-only struct
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: has_field() on meta-only struct", "[type_def][typed][has_field]") {
    type_def<MetaOnly> t;
    REQUIRE(!t.has_field("endpoint"));
    REQUIRE(!t.has_field("help"));
}

TEST_CASE("hybrid: has_field() on meta-only struct with zero fields", "[type_def][hybrid][has_field]") {
    auto t = type_def<MetaOnly>();
    REQUIRE(!t.has_field("endpoint"));
    REQUIRE(!t.has_field("help"));
    REQUIRE(!t.has_field("anything"));
}

TEST_CASE("dynamic: has_field() on empty type_def", "[type_def][dynamic][has_field]") {
    auto t = type_def("Empty");
    REQUIRE(!t.has_field("anything"));
}

// ═════════════════════════════════════════════════════════════════════════
// type_instance has()
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("type_instance: has() for existing field", "[type_instance][has]") {
    auto t = type_def("Event")
        .field<int>("count");
    auto obj = t.create();

    REQUIRE(obj.has("count"));
}

TEST_CASE("type_instance: has() for missing field", "[type_instance][has]") {
    auto t = type_def("Event")
        .field<int>("count");
    auto obj = t.create();

    REQUIRE(!obj.has("nope"));
}

TEST_CASE("type_instance: has() returns false for empty string", "[type_instance][has]") {
    auto t = type_def("Event")
        .field<int>("count");
    auto obj = t.create();

    REQUIRE(!obj.has(""));
}

TEST_CASE("type_instance: has() is case-sensitive", "[type_instance][has]") {
    auto t = type_def("Event")
        .field<int>("count");
    auto obj = t.create();

    REQUIRE(obj.has("count"));
    REQUIRE(!obj.has("Count"));
}

TEST_CASE("type_instance: has() finds all fields in multi-field type", "[type_instance][has]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count")
        .field<bool>("active");
    auto obj = t.create();

    REQUIRE(obj.has("title") == true);
    REQUIRE(obj.has("count") == true);
    REQUIRE(obj.has("active") == true);
}

TEST_CASE("type_instance: has() rejects meta-name strings", "[type_instance][has]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/e"})
        .field<int>("count");
    auto obj = t.create();

    REQUIRE(obj.has("count") == true);
    REQUIRE(obj.has("endpoint") == false);
}
