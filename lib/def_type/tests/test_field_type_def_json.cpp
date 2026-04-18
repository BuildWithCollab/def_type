#include "test_json_types.hpp"

// ═════════════════════════════════════════════════════════════════════════
// field<type_def<>> — reflection + JSON on structs with dynamic fields
//
// These tests verify that type_def<T>, to_json, from_json, and
// for_each_field work on structs containing field<type_def<>> members.
// ═════════════════════════════════════════════════════════════════════════

// ── Test types ──────────────────────────────────────────────────────────

inline auto reflected_details_type = type_def("Details")
    .field<std::string>("note")
    .field<int>("priority", 5);

struct ReflectedContainer {
    field<std::string>  name;
    field<type_def<>>   details {reflected_details_type};
};

#ifndef COLLAB_FIELD_HAS_PFR
template <>
constexpr auto def_type::struct_info<ReflectedContainer>() {
    return def_type::field_info<ReflectedContainer>("name", "details");
}
#endif

// ── Reflection queries ──────────────────────────────────────────────────

TEST_CASE("field<type_def<>>: type_def<T> sees field count", "[field][type_def][dynamic][reflect]") {
    type_def<ReflectedContainer> container_type;
    REQUIRE(container_type.field_count() == 2);
}

TEST_CASE("field<type_def<>>: type_def<T> sees field names", "[field][type_def][dynamic][reflect]") {
    type_def<ReflectedContainer> container_type;
    auto names = container_type.field_names();
    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "details");
}

TEST_CASE("field<type_def<>>: for_each_field iterates both fields", "[field][type_def][dynamic][reflect]") {
    ReflectedContainer container;
    container.name = "parent";
    container.details->set("note", std::string("hello"));

    std::vector<std::string> collected_names;
    type_def<ReflectedContainer>{}.for_each_field(container, [&](std::string_view field_name, auto& field_value) {
        collected_names.emplace_back(field_name);
    });

    REQUIRE(collected_names.size() == 2);
    REQUIRE(collected_names[0] == "name");
    REQUIRE(collected_names[1] == "details");
}

// ── JSON serialization ──────────────────────────────────────────────────

TEST_CASE("field<type_def<>>: to_json serializes entire struct", "[field][type_def][dynamic][reflect][to_json]") {
    ReflectedContainer container;
    container.name = "parent";
    container.details->set("note", std::string("hello"));
    container.details->set("priority", 10);

    auto j = to_json(container);

    REQUIRE(j["name"] == "parent");
    REQUIRE(j["details"].is_object());
    REQUIRE(j["details"]["note"] == "hello");
    REQUIRE(j["details"]["priority"] == 10);
}

// ── JSON deserialization ────────────────────────────────────────────────

TEST_CASE("field<type_def<>>: from_json deserializes entire struct", "[field][type_def][dynamic][reflect][from_json]") {
    auto j = json{
        {"name", "restored"},
        {"details", {{"note", "from json"}, {"priority", 42}}}
    };

    auto container = from_json<ReflectedContainer>(j);

    REQUIRE(container.name.value == "restored");
    REQUIRE(container.details->get<std::string>("note") == "from json");
    REQUIRE(container.details->get<int>("priority") == 42);
}

// ── Round-trip ──────────────────────────────────────────────────────────

TEST_CASE("field<type_def<>>: round-trip to_json then from_json", "[field][type_def][dynamic][reflect][roundtrip]") {
    ReflectedContainer original;
    original.name = "round-trip";
    original.details->set("note", std::string("survives"));
    original.details->set("priority", 77);

    auto j = to_json(original);
    auto restored = from_json<ReflectedContainer>(j);

    REQUIRE(restored.name.value == "round-trip");
    REQUIRE(restored.details->get<std::string>("note") == "survives");
    REQUIRE(restored.details->get<int>("priority") == 77);
}
