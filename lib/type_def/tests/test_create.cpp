#include "test_model_types.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// create() returns default-constructed instance
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: create() returns default-constructed instance", "[type_def][typed][create]") {
    type_def<SimpleArgs> t;
    SimpleArgs args = t.create();
    REQUIRE(args.name.value.empty());
    REQUIRE(args.age.value == 0);
    REQUIRE(args.active.value == false);
}

TEST_CASE("hybrid: create()", "[type_def][hybrid][create]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age");
    PlainDog dog = t.create();
    REQUIRE(dog.name.empty());
    REQUIRE(dog.age == 0);
}

TEST_CASE("type_instance: constructed via create()", "[type_instance][create]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 100);
    auto obj = t.create();
    REQUIRE(obj.type().name() == "Event");
    REQUIRE(obj.get<std::string>("title") == "");
    REQUIRE(obj.get<int>("count") == 100);
}

// ═══════════════════════════════════════════════════════════════════════════
// create() preserves field defaults
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: create() preserves field defaults", "[type_def][typed][create]") {
    type_def<Dog> t;
    Dog d = t.create();
    REQUIRE(d.name.value.empty());
    REQUIRE(d.age.value == 0);
    REQUIRE(d.breed.value.empty());
}

TEST_CASE("type_instance: has default values from type_def", "[type_instance][create][defaults]") {
    auto t = type_def("Event")
        .field<int>("count", 100)
        .field<std::string>("title", std::string("Untitled"));
    auto obj = t.create();
    REQUIRE(obj.get<int>("count") == 100);
    REQUIRE(obj.get<std::string>("title") == "Untitled");
}

// ═══════════════════════════════════════════════════════════════════════════
// create() result is mutable and works with set
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: create() result is mutable and works with set", "[type_def][typed][create]") {
    type_def<Dog> t;
    Dog d = t.create();
    t.set(d, "name", std::string("Buddy"));
    REQUIRE(t.get<std::string>(d, "name") == "Buddy");
}

TEST_CASE("hybrid: create() result is mutable and works with set", "[type_def][hybrid][create]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age");
    PlainDog dog = t.create();
    t.set(dog, "name", std::string("Buddy"));
    REQUIRE(t.get<std::string>(dog, "name") == "Buddy");
}

TEST_CASE("type_instance: create() result is mutable and works with set", "[type_instance][create]") {
    auto t = type_def("Event")
        .field<std::string>("title");
    auto obj = t.create();
    obj.set("title", std::string("Party"));
    REQUIRE(obj.get<std::string>("title") == "Party");
}

// ═══════════════════════════════════════════════════════════════════════════
// Fields without defaults get default-constructed values
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("type_instance: fields without defaults get default-constructed values", "[type_instance][create][defaults]") {
    auto t = type_def("Event")
        .field<int>("count")
        .field<std::string>("title");
    auto obj = t.create();
    REQUIRE(obj.get<int>("count") == 0);
    REQUIRE(obj.get<std::string>("title") == "");
}

// ═══════════════════════════════════════════════════════════════════════════
// Construction from type_def
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("type_instance: construction from type_def", "[type_instance][create]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 100);
    type_instance obj(t);
    REQUIRE(obj.type().name() == "Event");
    REQUIRE(obj.get<int>("count") == 100);
}

// ═══════════════════════════════════════════════════════════════════════════
// create() on meta-only type_def (no fields)
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: create() on meta-only struct", "[type_def][typed][create][meta]") {
    type_def<MetaOnly> t;
    MetaOnly m = t.create();
    REQUIRE(t.has_meta<endpoint_info>());
    REQUIRE(std::string_view{t.meta<endpoint_info>().path} == "/health");
    REQUIRE(t.field_count() == 0);
}

TEST_CASE("type_instance: create() on meta-only type_def", "[type_instance][create][meta]") {
    auto t = type_def("Health")
        .meta<endpoint_info>({.path = "/health"});
    auto obj = t.create();
    REQUIRE(obj.type().has_meta<endpoint_info>());
    REQUIRE(obj.type().field_count() == 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// Multiple instances are independent
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: multiple instances are independent", "[type_def][typed][create][independence]") {
    type_def<SimpleArgs> t;
    SimpleArgs a = t.create();
    SimpleArgs b = t.create();

    t.set(a, "name", std::string("Alice"));
    t.set(a, "age", 30);

    t.set(b, "name", std::string("Bob"));
    t.set(b, "age", 25);

    REQUIRE(t.get<std::string>(a, "name") == "Alice");
    REQUIRE(t.get<int>(a, "age") == 30);
    REQUIRE(t.get<std::string>(b, "name") == "Bob");
    REQUIRE(t.get<int>(b, "age") == 25);
}

TEST_CASE("hybrid: multiple instances are independent", "[type_def][hybrid][create][independence]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name")
        .field(&PlainDog::age, "age");
    PlainDog a = t.create();
    PlainDog b = t.create();

    t.set(a, "name", std::string("Rex"));
    t.set(a, "age", 5);
    t.set(b, "name", std::string("Fido"));
    t.set(b, "age", 3);

    REQUIRE(t.get<std::string>(a, "name") == "Rex");
    REQUIRE(t.get<int>(a, "age") == 5);
    REQUIRE(t.get<std::string>(b, "name") == "Fido");
    REQUIRE(t.get<int>(b, "age") == 3);
}

TEST_CASE("type_instance: multiple objects from same type_def are independent", "[type_instance][create][independence]") {
    auto t = type_def("Event")
        .field<std::string>("title", std::string("Default"))
        .field<int>("count", 0);

    auto a = t.create();
    auto b = t.create();

    a.set("title", std::string("Party A"));
    a.set("count", 10);

    b.set("title", std::string("Party B"));
    b.set("count", 20);

    REQUIRE(a.get<std::string>("title") == "Party A");
    REQUIRE(a.get<int>("count") == 10);
    REQUIRE(b.get<std::string>("title") == "Party B");
    REQUIRE(b.get<int>("count") == 20);
}

// ═══════════════════════════════════════════════════════════════════════════
// Full integration
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("type_instance: full integration", "[type_instance][integration]") {
    auto event_t = type_def("Event")
        .meta<endpoint_info>({.path = "/events"})
        .meta<help_info>({.summary = "An event"})
        .field<std::string>("title")
        .field<int>("attendees", 100)
        .field<bool>("verbose", false);

    // Create + defaults
    auto party = event_t.create();
    REQUIRE(party.get<int>("attendees") == 100);
    REQUIRE(party.get<bool>("verbose") == false);
    REQUIRE(party.get<std::string>("title") == "");

    // Set values
    party.set("title", std::string("Dog Party"));
    party.set("attendees", 50);
    party.set("verbose", true);

    // Get values back
    REQUIRE(party.get<std::string>("title") == "Dog Party");
    REQUIRE(party.get<int>("attendees") == 50);
    REQUIRE(party.get<bool>("verbose") == true);

    // Type mismatch throws
    REQUIRE_THROWS_AS(party.set("title", 42), std::logic_error);
    REQUIRE_THROWS_AS(party.get<int>("title"), std::logic_error);

    // Unknown field throws
    REQUIRE_THROWS_AS(party.set("nope", 1), std::logic_error);
    REQUIRE_THROWS_AS(party.get<int>("nope"), std::logic_error);

    // has() checks
    REQUIRE(party.has("title"));
    REQUIRE(party.has("attendees"));
    REQUIRE(!party.has("nope"));

    // type() access to metas and field defaults
    REQUIRE(party.type().name() == "Event");
    REQUIRE(party.type().has_meta<endpoint_info>());
    REQUIRE(party.type().has_meta<help_info>());
    REQUIRE(party.type().field("attendees").has_default());
    REQUIRE(party.type().field("attendees").default_value<int>() == 100);
}
