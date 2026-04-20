#include "test_model_types.hpp"

// ═════════════════════════════════════════════════════════════════════════
// field() returns view with correct name
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: field() returns view with correct name", "[type_def][typed][field_query]") {
    type_def<SimpleArgs> t;
    auto fv = t.field("name");
    REQUIRE(fv.name() == "name");
}

TEST_CASE("hybrid: field().name()", "[type_def][hybrid][field_query]") {
    type_def<PlainDog> t;
    REQUIRE(t.field("name").name() == "name");
}

TEST_CASE("dynamic: field().name()", "[type_def][dynamic][field_query]") {
    auto t = type_def("Event")
        .field<std::string>("title");
    REQUIRE(t.field("title").name() == "title");
}

// ═════════════════════════════════════════════════════════════════════════
// field() works for each field
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: field() works for each field", "[type_def][typed][field_query]") {
    type_def<Dog> t;
    REQUIRE(t.field("name").name() == "name");
    REQUIRE(t.field("age").name() == "age");
    REQUIRE(t.field("breed").name() == "breed");
}

TEST_CASE("hybrid: field() works for each field", "[type_def][hybrid][field_query]") {
    type_def<PlainDog> t;
    REQUIRE(t.field("name").name() == "name");
    REQUIRE(t.field("age").name() == "age");
    REQUIRE(t.field("breed").name() == "breed");
}

TEST_CASE("dynamic: field() works for each field", "[type_def][dynamic][field_query]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count")
        .field<bool>("active");
    REQUIRE(t.field("title").name() == "title");
    REQUIRE(t.field("count").name() == "count");
    REQUIRE(t.field("active").name() == "active");
}

// ═════════════════════════════════════════════════════════════════════════
// field_def default_value<V>()
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic: field().default_value<T>()", "[type_def][dynamic][field_query]") {
    auto t = type_def("Event")
        .field<int>("count", 100)
        .field<std::string>("title", std::string("Untitled"));
    REQUIRE(t.field("count").default_value<int>() == 100);
    REQUIRE(t.field("title").default_value<std::string>() == "Untitled");
}

// ═════════════════════════════════════════════════════════════════════════
// Edge cases
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: single field struct field access", "[type_def][typed][field_query]") {
    type_def<SingleField> t;
    REQUIRE(t.field("value").name() == "value");
}

TEST_CASE("hybrid: single field struct field access", "[type_def][hybrid][field_query]") {
    type_def<PlainPoint> t;
    REQUIRE(t.field("x").name() == "x");
}

TEST_CASE("dynamic: single field struct field access", "[type_def][dynamic][field_query]") {
    auto t = type_def("Thing")
        .field<int>("value");
    REQUIRE(t.field("value").name() == "value");
}

TEST_CASE("typed: field() returns valid view for each Dog field", "[type_def][typed][field_query]") {
    type_def<Dog> t;
    REQUIRE(t.field("breed").name() == "breed");
}

TEST_CASE("type_instance: field() returns field_def via type()", "[type_instance][field_query]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 100);
    auto obj = t.create();

    REQUIRE(obj.type().field("title").name() == "title");
    REQUIRE(obj.type().field("count").default_value<int>() == 100);
}

TEST_CASE("typed: field().default_value() returns T{} value", "[type_def][typed][field_query]") {
    type_def<SimpleArgs> t;
    REQUIRE(t.field("name").default_value<std::string>() == "");
}

TEST_CASE("typed: plain struct field().default_value() returns T{} value", "[type_def][typed][field_query]") {
    type_def<PlainDog> t;
    REQUIRE(t.field("name").default_value<std::string>() == "");
    REQUIRE(t.field("age").default_value<int>() == 0);
}

// ═════════════════════════════════════════════════════════════════════════
// field() throws for unknown name
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: field() throws for unknown name", "[type_def][typed][field_query][throw]") {
    REQUIRE_THROWS_AS(type_def<Dog>{}.field("nonexistent"), std::logic_error);
}

TEST_CASE("hybrid: field() throws for unknown name", "[type_def][hybrid][field_query][throw]") {
    type_def<PlainDog> t;
    REQUIRE_THROWS_AS(t.field("nonexistent"), std::logic_error);
}

TEST_CASE("dynamic: field() throws for unknown name", "[type_def][dynamic][field_query][throw]") {
    auto t = type_def("Event")
        .field<std::string>("title");
    REQUIRE_THROWS_AS(t.field("nonexistent"), std::logic_error);
}

TEST_CASE("type_instance: field() throws for unknown name", "[type_instance][field_query][throw]") {
    auto t = type_def("Event")
        .field<int>("count");
    auto obj = t.create();
    REQUIRE_THROWS_AS(obj.type().field("nonexistent"), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// field() throws on empty / meta-only
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: field() throws on meta-only struct", "[type_def][typed][field_query][throw]") {
    REQUIRE_THROWS_AS(type_def<MetaOnly>{}.field("anything"), std::logic_error);
}

TEST_CASE("hybrid: field() finds auto-discovered plain members", "[type_def][hybrid][field_query]") {
    type_def<PlainDog> t;
    REQUIRE(t.field("name").name() == "name");
    REQUIRE(t.field("age").name() == "age");
    REQUIRE(t.field("breed").name() == "breed");
    REQUIRE_THROWS_AS(t.field("anything"), std::logic_error);
}

TEST_CASE("dynamic: field() throws on empty type_def", "[type_def][dynamic][field_query][throw]") {
    auto t = type_def("Empty");
    REQUIRE_THROWS_AS(t.field("anything"), std::logic_error);
}

TEST_CASE("type_instance: field() throws on empty type_def", "[type_instance][field_query][throw]") {
    auto t = type_def("Empty");
    auto obj = t.create();
    REQUIRE_THROWS_AS(obj.type().field("anything"), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// field() throws for meta member names / finds plain member names
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: field() throws for meta member names", "[type_def][typed][field_query][throw]") {
    REQUIRE_THROWS_AS(type_def<Dog>{}.field("endpoint"), std::logic_error);
}

TEST_CASE("hybrid: field() throws for meta member names", "[type_def][hybrid][field_query][throw]") {
    REQUIRE_THROWS_AS(type_def<MetaDog>{}.field("help"), std::logic_error);
}

TEST_CASE("typed: field() finds plain member names", "[type_def][typed][field_query]") {
    auto fv = type_def<MixedStruct>{}.field("counter");
    REQUIRE(fv.name() == "counter");
    REQUIRE(fv.default_value<int>() == 0);
}

// ═════════════════════════════════════════════════════════════════════════
// field_def default_value throws
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic: field_def default_value throws for wrong type", "[type_def][dynamic][field_query][throw]") {
    auto t = type_def("Event")
        .field<int>("count", 100);
    REQUIRE_THROWS_AS(t.field("count").default_value<std::string>(), std::logic_error);
}

TEST_CASE("dynamic: field_def default_value throws when no default set", "[type_def][dynamic][field_query][throw]") {
    auto t = type_def("Event")
        .field<int>("count");
    REQUIRE_THROWS_AS(t.field("count").default_value<int>(), std::logic_error);
}

TEST_CASE("typed: field_def default_value works for auto-discovered fields", "[type_def][typed][field_query]") {
    REQUIRE(type_def<SimpleArgs>{}.field("name").default_value<std::string>() == "");
}

TEST_CASE("typed: plain struct field_def default_value works", "[type_def][typed][field_query]") {
    type_def<PlainDog> t;
    REQUIRE(t.field("name").default_value<std::string>() == "");
}
