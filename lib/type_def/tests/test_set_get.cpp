#include "test_model_types.hpp"

// ═════════════════════════════════════════════════════════════════════════
// set() assigns fields by type
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: set() assigns a string field", "[type_def][typed][set]") {
    SimpleArgs args;
    type_def<SimpleArgs>{}.set(args, "name", std::string("Alice"));
    REQUIRE(args.name.value == "Alice");
}

TEST_CASE("typed: set() assigns an int field", "[type_def][typed][set]") {
    SimpleArgs args;
    type_def<SimpleArgs>{}.set(args, "age", 30);
    REQUIRE(args.age.value == 30);
}

TEST_CASE("typed: set() assigns a bool field", "[type_def][typed][set]") {
    SimpleArgs args;
    type_def<SimpleArgs>{}.set(args, "active", true);
    REQUIRE(args.active.value == true);
}

TEST_CASE("hybrid: set() with matching type", "[type_def][hybrid][set]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age");
    PlainDog rex;

    t.set(rex, "name", std::string("Rex"));
    t.set(rex, "age", 3);

    REQUIRE(rex.name == "Rex");
    REQUIRE(rex.age == 3);
}

TEST_CASE("type_instance: set() with matching type", "[type_instance][set]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count");
    auto obj = t.create();

    obj.set("title", std::string("Dog Party"));
    obj.set("count", 50);

    REQUIRE(obj.get<std::string>("title") == "Dog Party");
    REQUIRE(obj.get<int>("count") == 50);
}

// ═════════════════════════════════════════════════════════════════════════
// set() with const char* to string field
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: set() with const char* to string field", "[type_def][typed][set]") {
    SimpleArgs args;
    type_def<SimpleArgs>{}.set(args, "name", "hello");
    REQUIRE(args.name.value == "hello");
}

TEST_CASE("hybrid: set() with const char* to string field", "[type_def][hybrid][set]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;

    t.set(rex, "name", "Rex");
    REQUIRE(rex.name == "Rex");
}

TEST_CASE("type_instance: set() with const char* to string field", "[type_instance][set]") {
    auto t = type_def("Event")
        .field<std::string>("title");
    auto obj = t.create();

    obj.set("title", "hello");
    REQUIRE(obj.get<std::string>("title") == "hello");
}

// ═════════════════════════════════════════════════════════════════════════
// set() overwrites existing values
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: set() overwrites existing values", "[type_def][typed][set]") {
    SimpleArgs args;
    type_def<SimpleArgs> t;

    t.set(args, "name", std::string("First"));
    t.set(args, "name", std::string("Second"));
    REQUIRE(args.name.value == "Second");
}

TEST_CASE("hybrid: set() overwrites existing values", "[type_def][hybrid][set]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;

    t.set(rex, "name", std::string("Rex"));
    t.set(rex, "name", std::string("Buddy"));
    REQUIRE(rex.name == "Buddy");
}

TEST_CASE("type_instance: set() overwrites existing values", "[type_instance][set]") {
    auto t = type_def("Event")
        .field<std::string>("title", std::string("First"));
    auto obj = t.create();

    obj.set("title", std::string("Second"));
    REQUIRE(obj.get<std::string>("title") == "Second");
}

// ═════════════════════════════════════════════════════════════════════════
// set() works on struct with metas
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: set() works on struct with metas", "[type_def][typed][set]") {
    Dog rex;
    type_def<Dog> t;

    t.set(rex, "name", std::string("Rex"));
    t.set(rex, "age", 3);
    t.set(rex, "breed", std::string("Husky"));

    REQUIRE(rex.name.value == "Rex");
    REQUIRE(rex.age.value == 3);
    REQUIRE(rex.breed.value == "Husky");
}

TEST_CASE("typed: set() works on struct with metas (MetaDog)", "[type_def][typed][set]") {
    MetaDog rex;
    type_def<MetaDog> t;

    t.set(rex, "name", std::string("Rex"));
    t.set(rex, "age", 3);

    REQUIRE(rex.name.value == "Rex");
    REQUIRE(rex.age.value == 3);
}

TEST_CASE("hybrid: set() on struct with meta members (MultiTagDog)", "[type_def][hybrid][set]") {
    auto t = type_def<MultiTagDog>()
        .field(&MultiTagDog::name, "name")
        .field(&MultiTagDog::age, "age");
    MultiTagDog dog;
    t.set(dog, "name", std::string("Rex"));
    t.set(dog, "age", 3);
    REQUIRE(dog.name == "Rex");
    REQUIRE(dog.age == 3);
    // Verify metas are untouched
    REQUIRE(std::string_view{dog.tag1->value} == "pet");
}

TEST_CASE("type_instance: set() works on type_def with metas", "[type_instance][set]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/events"})
        .field<std::string>("title")
        .field<int>("count");
    auto obj = t.create();

    obj.set("title", std::string("Launch"));
    obj.set("count", 100);

    REQUIRE(obj.get<std::string>("title") == "Launch");
    REQUIRE(obj.get<int>("count") == 100);
}

// ═════════════════════════════════════════════════════════════════════════
// get() callback finds field by name
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: get() finds field by name", "[type_def][typed][get]") {
    SimpleArgs args;
    args.age = 25;

    int found_value = 0;
    type_def<SimpleArgs>{}.get(args, "age", [&](std::string_view, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, int>) {
            found_value = value;
        }
    });

    REQUIRE(found_value == 25);
}

TEST_CASE("hybrid: get() finds field by name", "[type_def][hybrid][get]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::age, "age");
    PlainDog rex;
    rex.age = 25;

    int found_value = 0;
    t.get(rex, "age", [&](std::string_view, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, int>) {
            found_value = value;
        }
    });

    REQUIRE(found_value == 25);
}

// ═════════════════════════════════════════════════════════════════════════
// get() callback allows mutation
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: get() callback allows mutation", "[type_def][typed][get]") {
    SimpleArgs args;
    args.name = "Original";

    type_def<SimpleArgs>{}.get(args, "name", [](std::string_view, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, std::string>) {
            value = "Modified";
        }
    });

    REQUIRE(args.name.value == "Modified");
}

TEST_CASE("hybrid: get() callback allows mutation", "[type_def][hybrid][get]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;
    rex.name = "Original";

    t.get(rex, "name", [](std::string_view, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, std::string>) {
            value = "Modified";
        }
    });

    REQUIRE(rex.name == "Modified");
}

// ═════════════════════════════════════════════════════════════════════════
// get() callback on const instance
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: get() on const instance", "[type_def][typed][get]") {
    SimpleArgs args;
    args.age = 42;
    const SimpleArgs& cargs = args;

    int found_value = 0;
    type_def<SimpleArgs>{}.get(cargs, "age", [&](std::string_view, const auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, int>) {
            found_value = value;
        }
    });

    REQUIRE(found_value == 42);
}

TEST_CASE("hybrid: get() on const instance", "[type_def][hybrid][get]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::age, "age");
    PlainDog rex;
    rex.age = 42;
    const PlainDog& cref = rex;

    int found_value = 0;
    t.get(cref, "age", [&](std::string_view, const auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, int>) {
            found_value = value;
        }
    });

    REQUIRE(found_value == 42);
}

// ═════════════════════════════════════════════════════════════════════════
// get<V>() returns value
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: get<V>() returns value for matching type", "[type_def][typed][get_typed]") {
    SimpleArgs args;
    args.name = "Alice";
    args.age = 30;

    type_def<SimpleArgs> t;
    REQUIRE(t.get<std::string>(args, "name") == "Alice");
    REQUIRE(t.get<int>(args, "age") == 30);
}

TEST_CASE("hybrid: get<T>() returns value", "[type_def][hybrid][get_typed]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age");
    PlainDog rex;
    rex.name = "Rex";
    rex.age = 3;

    REQUIRE(t.get<std::string>(rex, "name") == "Rex");
    REQUIRE(t.get<int>(rex, "age") == 3);
}

TEST_CASE("type_instance: get() returns value", "[type_instance][get]") {
    auto t = type_def("Event")
        .field<std::string>("title", std::string("Dog Party"))
        .field<int>("count", 42);
    auto obj = t.create();

    REQUIRE(obj.get<std::string>("title") == "Dog Party");
    REQUIRE(obj.get<int>("count") == 42);
}

// ═════════════════════════════════════════════════════════════════════════
// get<V>() round-trips with set()
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: set() round-trips with get()", "[type_def][typed][set][get]") {
    Dog rex;
    type_def<Dog> t;

    t.set(rex, "name", std::string("Buddy"));
    t.set(rex, "age", 7);

    std::string got_name;
    int got_age = 0;
    t.get(rex, "name", [&](std::string_view, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, std::string>) {
            got_name = value;
        }
    });
    t.get(rex, "age", [&](std::string_view, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, int>) {
            got_age = value;
        }
    });

    REQUIRE(got_name == "Buddy");
    REQUIRE(got_age == 7);
}

TEST_CASE("typed: get<V>() round-trips with set()", "[type_def][typed][get_typed]") {
    Dog rex;
    type_def<Dog> t;

    t.set(rex, "name", std::string("Rex"));
    t.set(rex, "age", 5);
    t.set(rex, "breed", std::string("Husky"));

    REQUIRE(t.get<std::string>(rex, "name") == "Rex");
    REQUIRE(t.get<int>(rex, "age") == 5);
    REQUIRE(t.get<std::string>(rex, "breed") == "Husky");
}

TEST_CASE("hybrid: get<T>() round-trips with set()", "[type_def][hybrid][get_typed]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age")
        .field(&PlainDog::breed, "breed");
    PlainDog rex;

    t.set(rex, "name", std::string("Rex"));
    t.set(rex, "age", 3);
    t.set(rex, "breed", std::string("Husky"));

    REQUIRE(t.get<std::string>(rex, "name") == "Rex");
    REQUIRE(t.get<int>(rex, "age") == 3);
    REQUIRE(t.get<std::string>(rex, "breed") == "Husky");
}

TEST_CASE("type_instance: get() round-trips with set()", "[type_instance][get][set]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count")
        .field<bool>("active");
    auto obj = t.create();

    obj.set("title", std::string("Party"));
    obj.set("count", 99);
    obj.set("active", true);

    REQUIRE(obj.get<std::string>("title") == "Party");
    REQUIRE(obj.get<int>("count") == 99);
    REQUIRE(obj.get<bool>("active") == true);
}

// ═════════════════════════════════════════════════════════════════════════
// get<V>() on const instance
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("hybrid: get<V>() on const instance", "[type_def][hybrid][get_typed]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;
    rex.name = "Rex";
    const PlainDog& cref = rex;

    REQUIRE(t.get<std::string>(cref, "name") == "Rex");
}

TEST_CASE("type_instance: get<V>() on const instance", "[type_instance][get_typed]") {
    auto t = type_def("Event")
        .field<int>("count", 42);
    auto obj = t.create();
    const auto& cref = obj;

    REQUIRE(cref.get<int>("count") == 42);
}

TEST_CASE("typed: get<V>() on const instance", "[type_def][typed][get_typed]") {
    SimpleArgs args;
    args.active = true;
    const SimpleArgs& cargs = args;

    type_def<SimpleArgs> t;
    bool val = t.get<bool>(cargs, "active");
    REQUIRE(val == true);
}

// ═════════════════════════════════════════════════════════════════════════
// Integration
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: CliArgs integration — fields, metas, and extensions", "[type_def][typed][integration]") {
    type_def<CliArgs> t;

    // Schema
    REQUIRE(t.name() == "CliArgs");
    REQUIRE(t.field_count() == 3);
    REQUIRE(t.has_meta<help_info>());

    auto h = t.meta<help_info>();
    REQUIRE(std::string_view{h.summary} == "CLI tool");

    // Field names
    auto names = t.field_names();
    REQUIRE(names[0] == "query");
    REQUIRE(names[1] == "verbose");
    REQUIRE(names[2] == "limit");

    // Instance: set fields
    CliArgs args;
    t.set(args, "query", std::string("hello world"));
    t.set(args, "verbose", true);
    t.set(args, "limit", 50);

    // for_each to read query
    std::string query_val;
    t.for_each_field(args, [&](std::string_view name, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, std::string>) {
            if (name == "query") query_val = value;
        }
    });
    REQUIRE(query_val == "hello world");

    // get by name for verbose
    t.get(args, "verbose", [](std::string_view, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, bool>) {
            REQUIRE(value == true);
        }
    });
}

TEST_CASE("hybrid: full integration", "[type_def][hybrid][integration]") {
    auto dog_t = type_def<PlainDog>()
        .field(&PlainDog::name, "name",
            with<help_info>({.summary = "Dog's name"}))
        .field(&PlainDog::age, "age",
            with<help_info>({.summary = "Age in years"}),
            with<render_meta>({.render = {.style = "bold"}}))
        .field(&PlainDog::breed, "breed");

    // Schema checks
    REQUIRE(dog_t.name() == "PlainDog");
    REQUIRE(dog_t.field_count() == 3);
    auto names = dog_t.field_names();
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "age");
    REQUIRE(names[2] == "breed");

    // Field metas
    REQUIRE(dog_t.field("name").has_meta<help_info>());
    REQUIRE(std::string_view{dog_t.field("name").meta<help_info>().summary} == "Dog's name");
    REQUIRE(dog_t.field("age").has_meta<help_info>());
    REQUIRE(dog_t.field("age").has_meta<render_meta>());
    REQUIRE(!dog_t.field("breed").has_meta<help_info>());

    // Create + set + get
    PlainDog rex = dog_t.create();
    dog_t.set(rex, "name", std::string("Rex"));
    dog_t.set(rex, "age", 3);
    dog_t.set(rex, "breed", std::string("Husky"));

    REQUIRE(dog_t.get<std::string>(rex, "name") == "Rex");
    REQUIRE(dog_t.get<int>(rex, "age") == 3);
    REQUIRE(dog_t.get<std::string>(rex, "breed") == "Husky");

    // Throws on mismatch
    REQUIRE_THROWS_AS(dog_t.set(rex, "name", 42), std::logic_error);
    REQUIRE_THROWS_AS(dog_t.get<int>(rex, "name"), std::logic_error);
}

TEST_CASE("type_instance: full set/get integration", "[type_instance][set][get][integration]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count")
        .field<bool>("active");
    auto obj = t.create();

    // Set all fields
    obj.set("title", std::string("Dog Party"));
    obj.set("count", 50);
    obj.set("active", true);

    // Get all back
    REQUIRE(obj.get<std::string>("title") == "Dog Party");
    REQUIRE(obj.get<int>("count") == 50);
    REQUIRE(obj.get<bool>("active") == true);

    // Type mismatch throws
    REQUIRE_THROWS_AS(obj.set("title", 42), std::logic_error);
    REQUIRE_THROWS_AS(obj.get<int>("title"), std::logic_error);

    // Unknown field throws
    REQUIRE_THROWS_AS(obj.set("nope", 1), std::logic_error);
    REQUIRE_THROWS_AS(obj.get<int>("nope"), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// set() throws for unknown field
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: set() throws for unknown field name", "[type_def][typed][set][throw]") {
    SimpleArgs args;
    REQUIRE_THROWS_AS(type_def<SimpleArgs>{}.set(args, "nonexistent", 42), std::logic_error);
}

TEST_CASE("hybrid: set() throws for unknown field", "[type_def][hybrid][set][throw]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;
    REQUIRE_THROWS_AS(t.set(rex, "nope", 42), std::logic_error);
}

TEST_CASE("type_instance: set() throws for unknown field", "[type_instance][set][throw]") {
    auto t = type_def("Event")
        .field<int>("count");
    auto obj = t.create();
    REQUIRE_THROWS_AS(obj.set("nope", 42), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// set() throws for empty string field name
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("type_instance: set() throws for empty string field name", "[type_instance][set][throw]") {
    auto t = type_def("Event")
        .field<int>("count");
    auto obj = t.create();
    REQUIRE_THROWS_AS(obj.set("", 42), std::logic_error);
}

TEST_CASE("typed: set() throws for empty string field name", "[type_def][typed][set][throw]") {
    SimpleArgs args;
    REQUIRE_THROWS_AS(type_def<SimpleArgs>{}.set(args, "", 42), std::logic_error);
}

TEST_CASE("hybrid: set() throws for empty string field name", "[type_def][hybrid][set][throw]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;
    REQUIRE_THROWS_AS(t.set(rex, "", 42), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// get<V>() throws for empty string field name
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("type_instance: get<V>() throws for empty string field name", "[type_instance][get][throw]") {
    auto t = type_def("Event")
        .field<int>("count");
    auto obj = t.create();
    REQUIRE_THROWS_AS(obj.get<int>(""), std::logic_error);
}

TEST_CASE("typed: get<V>() throws for empty string field name", "[type_def][typed][get_typed][throw]") {
    SimpleArgs args;
    REQUIRE_THROWS_AS(type_def<SimpleArgs>{}.get<int>(args, ""), std::logic_error);
}

TEST_CASE("hybrid: get<V>() throws for empty string field name", "[type_def][hybrid][get_typed][throw]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;
    REQUIRE_THROWS_AS(t.get<int>(rex, ""), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// get() callback throws for empty string field name
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: get() callback throws for empty string field name", "[type_def][typed][get][throw]") {
    SimpleArgs args;
    REQUIRE_THROWS_AS(
        type_def<SimpleArgs>{}.get(args, "", [](auto, auto&) {}),
        std::logic_error);
}

TEST_CASE("hybrid: get() callback throws for empty string field name", "[type_def][hybrid][get][throw]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;
    REQUIRE_THROWS_AS(
        t.get(rex, "", [](auto, auto&) {}),
        std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// set() throws for type mismatch
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: set() throws for type mismatch", "[type_def][typed][set][throw]") {
    SimpleArgs args;
    REQUIRE_THROWS_AS(type_def<SimpleArgs>{}.set(args, "name", 42), std::logic_error);
}

TEST_CASE("hybrid: set() throws for type mismatch", "[type_def][hybrid][set][throw]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;
    REQUIRE_THROWS_AS(t.set(rex, "name", 42), std::logic_error);
}

TEST_CASE("type_instance: set() throws for type mismatch", "[type_instance][set][throw]") {
    auto t = type_def("Event")
        .field<std::string>("title");
    auto obj = t.create();
    REQUIRE_THROWS_AS(obj.set("title", 42), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// set() does not modify field on type mismatch
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: set() does not modify field on type mismatch", "[type_def][typed][set][throw]") {
    SimpleArgs args;
    args.name = "Original";

    try { type_def<SimpleArgs>{}.set(args, "name", 42); } catch (const std::logic_error&) {}
    REQUIRE(args.name.value == "Original");
}

TEST_CASE("hybrid: set() does not modify field on type mismatch", "[type_def][hybrid][set][throw]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;
    rex.name = "Untouched";

    try { t.set(rex, "name", 42); } catch (const std::logic_error&) {}
    REQUIRE(rex.name == "Untouched");
}

TEST_CASE("type_instance: set() does not modify field on type mismatch", "[type_instance][set][throw]") {
    auto t = type_def("Event")
        .field<std::string>("title", std::string("Original"));
    auto obj = t.create();

    try { obj.set("title", 42); } catch (const std::logic_error&) {}
    REQUIRE(obj.get<std::string>("title") == "Original");
}

// ═════════════════════════════════════════════════════════════════════════
// set() throws for meta/plain member names
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: set() throws for meta member names", "[type_def][typed][set][throw]") {
    Dog rex;
    REQUIRE_THROWS_AS(type_def<Dog>{}.set(rex, "endpoint", 42), std::logic_error);
    REQUIRE_THROWS_AS(type_def<Dog>{}.set(rex, "help", 42), std::logic_error);
}

TEST_CASE("hybrid: set() throws for meta member names", "[type_def][hybrid][set][throw]") {
    type_def<MetaDog> t;
    MetaDog rex;
    REQUIRE_THROWS_AS(t.set(rex, "help", 42), std::logic_error);
}

TEST_CASE("typed: set() throws for plain member names", "[type_def][typed][set][throw]") {
    MixedStruct ms;
    REQUIRE_THROWS_AS(type_def<MixedStruct>{}.set(ms, "counter", 42), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// get<V>() throws for unknown field
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: get<V>() throws for unknown field", "[type_def][typed][get_typed][throw]") {
    SimpleArgs args;
    type_def<SimpleArgs> t;
    REQUIRE_THROWS_AS(t.get<int>(args, "nope"), std::logic_error);
}

TEST_CASE("hybrid: get<V>() throws for unknown field", "[type_def][hybrid][get_typed][throw]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex;
    REQUIRE_THROWS_AS(t.get<std::string>(rex, "nope"), std::logic_error);
}

TEST_CASE("type_instance: get<V>() throws for unknown field", "[type_instance][get][throw]") {
    auto t = type_def("Event")
        .field<int>("count");
    auto obj = t.create();
    REQUIRE_THROWS_AS(obj.get<int>("nope"), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// get<V>() throws for type mismatch
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: get<V>() throws for type mismatch", "[type_def][typed][get_typed][throw]") {
    SimpleArgs args;
    args.age = 42;
    type_def<SimpleArgs> t;
    REQUIRE_THROWS_AS(t.get<std::string>(args, "age"), std::logic_error);
}

TEST_CASE("hybrid: get<V>() throws for type mismatch", "[type_def][hybrid][get_typed][throw]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::age, "age");
    PlainDog rex;
    rex.age = 5;
    REQUIRE_THROWS_AS(t.get<std::string>(rex, "age"), std::logic_error);
}

TEST_CASE("type_instance: get<V>() throws for type mismatch", "[type_instance][get][throw]") {
    auto t = type_def("Event")
        .field<int>("count", 42);
    auto obj = t.create();
    REQUIRE_THROWS_AS(obj.get<std::string>("count"), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// get<V>() throws for meta/plain member names
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: get<V>() throws for meta member names", "[type_def][typed][get_typed][throw]") {
    Dog rex;
    REQUIRE_THROWS_AS(type_def<Dog>{}.get<int>(rex, "endpoint"), std::logic_error);
}

TEST_CASE("hybrid: get<V>() throws for meta member names", "[type_def][hybrid][get_typed][throw]") {
    type_def<MetaDog> t;
    MetaDog rex;
    REQUIRE_THROWS_AS(t.get<int>(rex, "help"), std::logic_error);
}

TEST_CASE("typed: get<V>() throws for plain member names", "[type_def][typed][get_typed][throw]") {
    MixedStruct ms;
    REQUIRE_THROWS_AS(type_def<MixedStruct>{}.get<int>(ms, "counter"), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// get() callback throws for unknown field
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: get() throws for unknown field name", "[type_def][typed][get][throw]") {
    SimpleArgs args;
    REQUIRE_THROWS_AS(
        type_def<SimpleArgs>{}.get(args, "nonexistent", [](std::string_view, auto&) {}),
        std::logic_error);
}

TEST_CASE("hybrid: get() callback throws for unknown field", "[type_def][hybrid][get][throw]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");
    PlainDog rex{"Rex", 3, "Husky"};
    REQUIRE_THROWS_AS(
        t.get(rex, "nope", [](std::string_view, auto&) {}),
        std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// get() callback throws for meta/plain member names
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: get() throws for meta member names", "[type_def][typed][get][throw]") {
    Dog rex;
    REQUIRE_THROWS_AS(
        type_def<Dog>{}.get(rex, "endpoint", [](std::string_view, auto&) {}),
        std::logic_error);
    REQUIRE_THROWS_AS(
        type_def<Dog>{}.get(rex, "help", [](std::string_view, auto&) {}),
        std::logic_error);
}

TEST_CASE("hybrid: get() callback throws for meta member names", "[type_def][hybrid][get][throw]") {
    type_def<MetaDog> t;
    MetaDog rex;
    REQUIRE_THROWS_AS(
        t.get(rex, "help", [](std::string_view, auto&) {}),
        std::logic_error);
}

TEST_CASE("typed: get() callback throws for plain member names", "[type_def][typed][get][throw]") {
    MixedStruct ms;
    REQUIRE_THROWS_AS(
        type_def<MixedStruct>{}.get(ms, "counter", [](std::string_view, auto&) {}),
        std::logic_error);
}
