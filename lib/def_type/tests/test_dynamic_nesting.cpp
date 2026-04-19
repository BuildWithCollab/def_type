#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include <string>

#include <def_type.hpp>

using namespace def_type;
using json = nlohmann::json;

// ═════════════════════════════════════════════════════════════════════════
// Dynamic nesting — .field("name", nested_type_def)
//
// A dynamic type_def can nest another dynamic type_def as a field.
// The nested type_def is passed directly — no template parameter.
//
//   Static equivalent:
//     struct Details { field<std::string> note; field<int> priority; };
//     struct Container { field<std::string> name; field<Details> details; };
//
//   Dynamic equivalent:
//     auto details_type = type_def("Details")
//         .field<std::string>("note").field<int>("priority");
//     auto container_type = type_def("Container")
//         .field<std::string>("name").field("details", details_type);
//
// ═════════════════════════════════════════════════════════════════════════

// ── Builder + schema queries ────────────────────────────────────────────

TEST_CASE("dynamic nesting: field() accepts a type_def", "[type_def][dynamic][nesting]") {
    auto inner_type = type_def("Inner")
        .field<std::string>("note")
        .field<int>("priority");

    auto outer_type = type_def("Outer")
        .field<std::string>("name")
        .field("details", inner_type);

    REQUIRE(outer_type.field_count() == 2);
    REQUIRE(outer_type.has_field("name"));
    REQUIRE(outer_type.has_field("details"));
}

TEST_CASE("dynamic nesting: field_names() includes nested field", "[type_def][dynamic][nesting]") {
    auto inner_type = type_def("Inner")
        .field<int>("x");

    auto outer_type = type_def("Outer")
        .field<std::string>("label")
        .field("child", inner_type);

    auto names = outer_type.field_names();
    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "label");
    REQUIRE(names[1] == "child");
}

// ── create() produces a nested type_instance ────────────────────────────

TEST_CASE("dynamic nesting: create() populates nested instance", "[type_def][dynamic][nesting]") {
    auto inner_type = type_def("Inner")
        .field<std::string>("note", std::string("default note"))
        .field<int>("priority", 5);

    auto outer_type = type_def("Outer")
        .field<std::string>("name")
        .field("details", inner_type);

    auto outer_object = outer_type.create();

    // The nested field should be a type_instance with defaults from inner_type
    auto nested = outer_object.get<type_instance>("details");
    REQUIRE(nested.get<std::string>("note") == "default note");
    REQUIRE(nested.get<int>("priority") == 5);
}

// ── set/get on nested fields ────────────────────────────────────────────

TEST_CASE("dynamic nesting: set and get nested type_instance", "[type_def][dynamic][nesting]") {
    auto inner_type = type_def("Inner")
        .field<std::string>("note")
        .field<int>("priority");

    auto outer_type = type_def("Outer")
        .field<std::string>("name")
        .field("details", inner_type);

    auto outer_object = outer_type.create();
    outer_object.set("name", std::string("parent"));

    // Modify nested instance via get
    auto nested = outer_object.get<type_instance>("details");
    nested.set("note", std::string("hello"));
    nested.set("priority", 10);
    outer_object.set("details", nested);

    auto retrieved = outer_object.get<type_instance>("details");
    REQUIRE(retrieved.get<std::string>("note") == "hello");
    REQUIRE(retrieved.get<int>("priority") == 10);
}

// ── 3-level deep nesting ────────────────────────────────────────────────

TEST_CASE("dynamic nesting: 3-level deep", "[type_def][dynamic][nesting]") {
    auto leaf_type = type_def("Leaf")
        .field<std::string>("value", std::string("deep"));

    auto middle_type = type_def("Middle")
        .field<std::string>("label", std::string("mid"))
        .field("leaf", leaf_type);

    auto root_type = type_def("Root")
        .field<std::string>("name", std::string("top"))
        .field("middle", middle_type);

    auto root = root_type.create();
    REQUIRE(root.get<std::string>("name") == "top");

    auto middle = root.get<type_instance>("middle");
    REQUIRE(middle.get<std::string>("label") == "mid");

    auto leaf = middle.get<type_instance>("leaf");
    REQUIRE(leaf.get<std::string>("value") == "deep");
}

// ── to_json serializes nested type_instance ─────────────────────────────

TEST_CASE("dynamic nesting: to_json", "[to_json][dynamic][nesting]") {
    auto inner_type = type_def("Inner")
        .field<int>("x", 42)
        .field<std::string>("label", std::string("hello"));

    auto outer_type = type_def("Outer")
        .field<std::string>("name", std::string("parent"))
        .field("nested", inner_type);

    auto outer_object = outer_type.create();
    auto j = outer_object.to_json();

    REQUIRE(j["name"] == "parent");
    REQUIRE(j["nested"].is_object());
    REQUIRE(j["nested"]["x"] == 42);
    REQUIRE(j["nested"]["label"] == "hello");
}

TEST_CASE("dynamic nesting: to_json 3-level deep", "[to_json][dynamic][nesting]") {
    auto leaf_type = type_def("Leaf")
        .field<std::string>("value", std::string("deep"));

    auto middle_type = type_def("Middle")
        .field<std::string>("label", std::string("mid"))
        .field("leaf", leaf_type);

    auto root_type = type_def("Root")
        .field<std::string>("name", std::string("top"))
        .field("middle", middle_type);

    auto root = root_type.create();
    auto j = root.to_json();

    REQUIRE(j["name"] == "top");
    REQUIRE(j["middle"]["label"] == "mid");
    REQUIRE(j["middle"]["leaf"]["value"] == "deep");
}

TEST_CASE("dynamic nesting: to_json empty nested", "[to_json][dynamic][nesting]") {
    auto inner_type = type_def("Empty");

    auto outer_type = type_def("Wrapper")
        .field<std::string>("label", std::string("has empty"))
        .field("empty", inner_type);

    auto outer_object = outer_type.create();
    auto j = outer_object.to_json();

    REQUIRE(j["label"] == "has empty");
    REQUIRE(j["empty"].is_object());
    REQUIRE(j["empty"].empty());
}

// ── from_json (create from JSON) ────────────────────────────────────────

TEST_CASE("dynamic nesting: create from JSON", "[from_json][dynamic][nesting]") {
    auto inner_type = type_def("Inner")
        .field<std::string>("note")
        .field<int>("priority");

    auto outer_type = type_def("Outer")
        .field<std::string>("name")
        .field("details", inner_type);

    auto j = json{
        {"name", "parent"},
        {"details", {{"note", "hello"}, {"priority", 10}}}
    };

    auto outer_object = outer_type.create(j);

    REQUIRE(outer_object.get<std::string>("name") == "parent");
    auto nested = outer_object.get<type_instance>("details");
    REQUIRE(nested.get<std::string>("note") == "hello");
    REQUIRE(nested.get<int>("priority") == 10);
}

// ── Round-trip ──────────────────────────────────────────────────────────

TEST_CASE("dynamic nesting: round-trip to_json then create(json)", "[json][dynamic][nesting][roundtrip]") {
    auto inner_type = type_def("Inner")
        .field<int>("x")
        .field<std::string>("label");

    auto outer_type = type_def("Outer")
        .field<std::string>("name")
        .field("nested", inner_type);

    auto original = outer_type.create();
    original.set("name", std::string("parent"));
    auto nested = original.get<type_instance>("nested");
    nested.set("x", 42);
    nested.set("label", std::string("hello"));
    original.set("nested", nested);

    auto j = original.to_json();
    auto restored = outer_type.create(j);

    REQUIRE(restored.get<std::string>("name") == "parent");
    auto restored_nested = restored.get<type_instance>("nested");
    REQUIRE(restored_nested.get<int>("x") == 42);
    REQUIRE(restored_nested.get<std::string>("label") == "hello");
}

TEST_CASE("dynamic nesting: 3-level round-trip", "[json][dynamic][nesting][roundtrip]") {
    auto leaf_type = type_def("Leaf")
        .field<std::string>("value");

    auto middle_type = type_def("Middle")
        .field<std::string>("label")
        .field("leaf", leaf_type);

    auto root_type = type_def("Root")
        .field<std::string>("name")
        .field("middle", middle_type);

    auto leaf = leaf_type.create();
    leaf.set("value", std::string("deep"));

    auto middle = middle_type.create();
    middle.set("label", std::string("mid"));
    middle.set("leaf", leaf);

    auto root = root_type.create();
    root.set("name", std::string("top"));
    root.set("middle", middle);

    auto j = root.to_json();
    auto restored = root_type.create(j);

    REQUIRE(restored.get<std::string>("name") == "top");
    auto restored_middle = restored.get<type_instance>("middle");
    REQUIRE(restored_middle.get<std::string>("label") == "mid");
    auto restored_leaf = restored_middle.get<type_instance>("leaf");
    REQUIRE(restored_leaf.get<std::string>("value") == "deep");
}

// ── load_json overlay ───────────────────────────────────────────────────

TEST_CASE("dynamic nesting: load_json overlays nested", "[json][dynamic][nesting][load_json]") {
    auto inner_type = type_def("Inner")
        .field<int>("x", 1)
        .field<std::string>("label", std::string("original"));

    auto outer_type = type_def("Outer")
        .field<std::string>("name", std::string("before"))
        .field("nested", inner_type);

    auto outer_object = outer_type.create();

    outer_object.load_json(json{
        {"nested", {{"x", 99}, {"label", "updated"}}}
    });

    REQUIRE(outer_object.get<std::string>("name") == "before");
    auto updated_nested = outer_object.get<type_instance>("nested");
    REQUIRE(updated_nested.get<int>("x") == 99);
    REQUIRE(updated_nested.get<std::string>("label") == "updated");
}

// ── Mixed: nested alongside other field types ───────────────────────────

TEST_CASE("dynamic nesting: alongside vectors and primitives", "[to_json][dynamic][nesting]") {
    auto detail_type = type_def("Detail")
        .field<std::string>("note", std::string("important"));

    auto container_type = type_def("Container")
        .field<std::string>("title", std::string("My Thing"))
        .field<int>("count", 5)
        .field<std::vector<std::string>>("tags")
        .field("detail", detail_type);

    auto container_object = container_type.create();
    container_object.set("tags", std::vector<std::string>{"a", "b", "c"});

    auto j = container_object.to_json();

    REQUIRE(j["title"] == "My Thing");
    REQUIRE(j["count"] == 5);
    REQUIRE(j["tags"].size() == 3);
    REQUIRE(j["detail"]["note"] == "important");
}
