#include <catch2/catch_test_macros.hpp>

#include <def_type.hpp>

using namespace def_type;

struct SmokeTest {
    field<std::string> name;
    field<int>         age {.value = 0};
};

TEST_CASE("smoke: define a type and query its schema", "[smoke]") {
    type_def<SmokeTest> t;
    CHECK(t.name() == "SmokeTest");
    CHECK(t.field_count() == 2);
    CHECK(t.has_field("name"));
    CHECK(t.has_field("age"));
}

TEST_CASE("smoke: set and get fields by name", "[smoke]") {
    type_def<SmokeTest> t;
    SmokeTest obj;
    t.set(obj, "name", std::string("Rex"));
    t.set(obj, "age", 3);
    CHECK(t.get<std::string>(obj, "name") == "Rex");
    CHECK(t.get<int>(obj, "age") == 3);
}

TEST_CASE("smoke: JSON round-trip", "[smoke]") {
    SmokeTest obj;
    obj.name = "Buddy";
    obj.age = 7;

    auto j = to_json(obj);
    CHECK(j["name"] == "Buddy");
    CHECK(j["age"] == 7);

    auto restored = from_json<SmokeTest>(j);
    CHECK(restored.name.value == "Buddy");
    CHECK(restored.age.value == 7);
}
