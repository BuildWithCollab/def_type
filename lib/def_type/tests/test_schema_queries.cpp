#include "test_model_types.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// name()
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: name() returns the struct type name", "[type_def][typed][name]") {
    REQUIRE(type_def<SimpleArgs>{}.name() == "SimpleArgs");
}

TEST_CASE("typed: name() works for various types", "[type_def][typed][name]") {
    REQUIRE(type_def<Dog>{}.name() == "Dog");
    REQUIRE(type_def<MixedStruct>{}.name() == "MixedStruct");
    REQUIRE(type_def<SingleField>{}.name() == "SingleField");
}

TEST_CASE("hybrid: name()", "[type_def][hybrid][name]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    REQUIRE(t.name() == "PlainDog");
}

TEST_CASE("hybrid: name() works for various types", "[type_def][hybrid][name]") {
    auto dog = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    REQUIRE(dog.name() == "PlainDog");

    auto point = type_def<PlainPoint>()
        .field(&PlainPoint::x, "x")
        .field(&PlainPoint::y, "y");
    REQUIRE(point.name() == "PlainPoint");
}

TEST_CASE("dynamic: name()", "[type_def][dynamic][name]") {
    REQUIRE(type_def("Event").name() == "Event");
}

TEST_CASE("dynamic: name() with different names", "[type_def][dynamic][name]") {
    REQUIRE(type_def("Dog").name() == "Dog");
    REQUIRE(type_def("Widget").name() == "Widget");
    REQUIRE(type_def("").name() == "");
}

TEST_CASE("type_instance: name()", "[type_instance][name]") {
    auto t = type_def("Event")
        .field<int>("x");
    auto obj = t.create();
    REQUIRE(obj.type().name() == "Event");
}

// ═══════════════════════════════════════════════════════════════════════════
// field_count()
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: field_count() counts only field<> members", "[type_def][typed][field_count]") {
    REQUIRE(type_def<SimpleArgs>{}.field_count() == 3);
    REQUIRE(type_def<Dog>{}.field_count() == 3);
    REQUIRE(type_def<SingleField>{}.field_count() == 1);
}

TEST_CASE("typed: field_count() excludes meta<> members", "[type_def][typed][field_count]") {
    // Dog has 2 metas + 3 fields = 5 total members, but only 3 fields
    REQUIRE(type_def<Dog>{}.field_count() == 3);
}

TEST_CASE("typed: field_count() excludes plain members", "[type_def][typed][field_count]") {
    // MixedStruct has 1 meta + 2 fields + 1 plain = 4 total, 2 are field<>
    REQUIRE(type_def<MixedStruct>{}.field_count() == 2);
}

TEST_CASE("typed: field_count() is zero when struct has only metas", "[type_def][typed][field_count]") {
    REQUIRE(type_def<MetaOnly>{}.field_count() == 0);
}

TEST_CASE("typed: field_count() for multi-tagged struct", "[type_def][typed][field_count]") {
    // 3 metas + 1 field
    REQUIRE(type_def<MultiTagged>{}.field_count() == 1);
}

TEST_CASE("hybrid: field_count() includes registered fields", "[type_def][hybrid][field_count]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age")
        .field(&PlainDog::breed, "breed");
    REQUIRE(t.field_count() == 3);
}

TEST_CASE("hybrid: field_count() with zero registered fields", "[type_def][hybrid][field_count]") {
    auto t = type_def<PlainDog>();
    REQUIRE(t.field_count() == 0);
}

TEST_CASE("hybrid: field_count() excludes unregistered members", "[type_def][hybrid][field_count]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age");
    REQUIRE(t.field_count() == 2);
}

TEST_CASE("dynamic: field_count() with no fields", "[type_def][dynamic][field_count]") {
    REQUIRE(type_def("Empty").field_count() == 0);
}

TEST_CASE("dynamic: field_count()", "[type_def][dynamic][field_count]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count");
    REQUIRE(t.field_count() == 2);
}

TEST_CASE("dynamic: field_count() ignores type-level metas", "[type_def][dynamic][field_count]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/events"})
        .field<std::string>("title");
    REQUIRE(t.field_count() == 1);
}

TEST_CASE("type_instance: field_count()", "[type_instance][field_count]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count");
    auto obj = t.create();
    REQUIRE(obj.type().field_count() == 2);
}

// ═══════════════════════════════════════════════════════════════════════════
// field_names()
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: field_names() returns field<> member names only", "[type_def][typed][field_names]") {
    auto names = type_def<Dog>{}.field_names();
    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "age");
    REQUIRE(names[2] == "breed");
}

TEST_CASE("typed: field_names() excludes meta and plain members", "[type_def][typed][field_names]") {
    auto names = type_def<MixedStruct>{}.field_names();
    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "label");
    REQUIRE(names[1] == "score");
}

TEST_CASE("typed: field_names() for simple struct", "[type_def][typed][field_names]") {
    auto names = type_def<SimpleArgs>{}.field_names();
    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "age");
    REQUIRE(names[2] == "active");
}

TEST_CASE("hybrid: field_names() includes registered fields", "[type_def][hybrid][field_names]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age");
    auto names = t.field_names();
    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "age");
}

TEST_CASE("hybrid: field_names() empty when no fields registered", "[type_def][hybrid][field_names]") {
    auto t = type_def<PlainDog>();
    REQUIRE(t.field_names().empty());
}

TEST_CASE("hybrid: field_names() preserves registration order", "[type_def][hybrid][field_names]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::breed, "breed")
        .field(&PlainDog::age, "age")
        .field(&PlainDog::name, "name");
    auto names = t.field_names();
    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "breed");
    REQUIRE(names[1] == "age");
    REQUIRE(names[2] == "name");
}

TEST_CASE("dynamic: field_names()", "[type_def][dynamic][field_names]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("attendees")
        .field<bool>("verbose");
    auto names = t.field_names();
    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "title");
    REQUIRE(names[1] == "attendees");
    REQUIRE(names[2] == "verbose");
}

TEST_CASE("dynamic: field_names() empty", "[type_def][dynamic][field_names]") {
    REQUIRE(type_def("Empty").field_names().empty());
}

TEST_CASE("type_instance: field_names()", "[type_instance][field_names]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count");
    auto obj = t.create();
    auto names = obj.type().field_names();
    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "title");
    REQUIRE(names[1] == "count");
}

TEST_CASE("dynamic: field_names() preserves registration order", "[type_def][dynamic][field_names]") {
    auto t = type_def("Event")
        .field<bool>("verbose")
        .field<std::string>("title")
        .field<int>("count");
    auto names = t.field_names();
    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "verbose");
    REQUIRE(names[1] == "title");
    REQUIRE(names[2] == "count");
}

// ═══════════════════════════════════════════════════════════════════════════
// typed type_def still works without hybrid registration
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: type_def<T> without hybrid still works", "[type_def][hybrid][compat]") {
    type_def<MetaDog> t;
    REQUIRE(t.field_count() == 2);
    REQUIRE(t.name() == "MetaDog");

    MetaDog rex;
    rex.name = "Rex";
    rex.age = 3;

    std::string found_name;
    t.for_each_field(rex, [&](std::string_view name, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, std::string>)
            if (name == "name") found_name = value;
    });
    REQUIRE(found_name == "Rex");
}

// ═══════════════════════════════════════════════════════════════════════════
// stateless
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: two type_def instances behave identically", "[type_def][typed][stateless]") {
    type_def<Dog> t1;
    type_def<Dog> t2;

    REQUIRE(t1.name() == t2.name());
    REQUIRE(t1.field_count() == t2.field_count());
    REQUIRE(t1.has_meta<endpoint_info>() == t2.has_meta<endpoint_info>());

    auto ep1 = t1.meta<endpoint_info>();
    auto ep2 = t2.meta<endpoint_info>();
    REQUIRE(std::string_view{ep1.path} == std::string_view{ep2.path});
    REQUIRE(std::string_view{ep1.method} == std::string_view{ep2.method});
}

TEST_CASE("hybrid: two type_def instances behave identically", "[type_def][hybrid][stateless]") {
    auto t1 = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age")
        .field(&PlainDog::breed, "breed");
    auto t2 = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age")
        .field(&PlainDog::breed, "breed");

    REQUIRE(t1.name() == t2.name());
    REQUIRE(t1.field_count() == t2.field_count());
    REQUIRE(t1.field_names() == t2.field_names());
}

TEST_CASE("dynamic: two type_def instances behave identically", "[type_def][dynamic][stateless]") {
    auto t1 = type_def("Event")
        .meta<endpoint_info>({.path = "/events"})
        .field<std::string>("title")
        .field<int>("count");
    auto t2 = type_def("Event")
        .meta<endpoint_info>({.path = "/events"})
        .field<std::string>("title")
        .field<int>("count");

    REQUIRE(t1.name() == t2.name());
    REQUIRE(t1.field_count() == t2.field_count());
    REQUIRE(t1.field_names() == t2.field_names());
    REQUIRE(t1.has_meta<endpoint_info>() == t2.has_meta<endpoint_info>());

    auto ep1 = t1.meta<endpoint_info>();
    auto ep2 = t2.meta<endpoint_info>();
    REQUIRE(std::string_view{ep1.path} == std::string_view{ep2.path});
}

// ═══════════════════════════════════════════════════════════════════════════
// type() access (type_instance)
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("type_instance: type() returns the backing type_def", "[type_instance][type]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/events"})
        .field<std::string>("title")
        .field<int>("count");
    auto obj = t.create();

    REQUIRE(obj.type().name() == "Event");
    REQUIRE(obj.type().field_count() == 2);
    REQUIRE(obj.type().has_meta<endpoint_info>());
    REQUIRE(std::string_view{obj.type().meta<endpoint_info>().path} == "/events");
}
