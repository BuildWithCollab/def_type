#include "test_json_types.hpp"

// ═════════════════════════════════════════════════════════════════════════
// Hybrid JSON — to_json / from_json for plain structs via type_def<T>
//
// Plain structs (no field<> members) registered via the hybrid path:
//   auto dog_type = type_def<PlainDog>()
//       .field(&PlainDog::name, "name")
//       .field(&PlainDog::age, "age");
//
// Need overloads: to_json(instance, type_def) and from_json<T>(json, type_def)
// ═════════════════════════════════════════════════════════════════════════

// ── Test types ──────────────────────────────────────────────────────────

struct HybridDog {
    std::string name;
    int         age = 0;
    std::string breed;
};

struct HybridPoint {
    double x = 0.0;
    double y = 0.0;
};

// ── struct_info fallbacks (non-PFR builds) ──────────────────────────────

#ifndef COLLAB_FIELD_HAS_PFR
template <>
constexpr auto def_type::struct_info<HybridDog>() {
    return def_type::field_info<HybridDog>("name", "age", "breed");
}

template <>
constexpr auto def_type::struct_info<HybridPoint>() {
    return def_type::field_info<HybridPoint>("x", "y");
}
#endif

inline auto hybrid_dog_type = type_def<HybridDog>()
    .field(&HybridDog::name, "name")
    .field(&HybridDog::age, "age")
    .field(&HybridDog::breed, "breed");

inline auto hybrid_point_type = type_def<HybridPoint>()
    .field(&HybridPoint::x, "x")
    .field(&HybridPoint::y, "y");

// ── to_json with type_def ───────────────────────────────────────────────

TEST_CASE("hybrid json: to_json with type_def — basic", "[hybrid][to_json][json]") {
    HybridDog rex;
    rex.name = "Rex";
    rex.age = 3;
    rex.breed = "Husky";

    auto j = to_json(rex, hybrid_dog_type);

    REQUIRE(j["name"] == "Rex");
    REQUIRE(j["age"] == 3);
    REQUIRE(j["breed"] == "Husky");
}

TEST_CASE("hybrid json: to_json with type_def — defaults", "[hybrid][to_json][json]") {
    HybridDog dog;

    auto j = to_json(dog, hybrid_dog_type);

    REQUIRE(j["name"] == "");
    REQUIRE(j["age"] == 0);
    REQUIRE(j["breed"] == "");
}

TEST_CASE("hybrid json: to_json with type_def — doubles", "[hybrid][to_json][json]") {
    HybridPoint point;
    point.x = 1.5;
    point.y = 2.7;

    auto j = to_json(point, hybrid_point_type);

    REQUIRE(j["x"] == 1.5);
    REQUIRE(j["y"] == 2.7);
}

TEST_CASE("hybrid json: to_json with type_def — only registered fields", "[hybrid][to_json][json]") {
    // Register only some fields
    auto partial_type = type_def<HybridDog>()
        .field(&HybridDog::name, "name")
        .field(&HybridDog::age, "age");
    // breed is NOT registered

    HybridDog rex;
    rex.name = "Rex";
    rex.age = 3;
    rex.breed = "Husky";

    auto j = to_json(rex, partial_type);

    REQUIRE(j.size() == 2);
    REQUIRE(j["name"] == "Rex");
    REQUIRE(j["age"] == 3);
    REQUIRE(!j.contains("breed"));
}

// ── from_json with type_def ─────────────────────────────────────────────

TEST_CASE("hybrid json: from_json with type_def — basic", "[hybrid][from_json][json]") {
    auto j = json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}};

    auto dog = def_type::from_json<HybridDog>(j, hybrid_dog_type);

    REQUIRE(dog.name == "Rex");
    REQUIRE(dog.age == 3);
    REQUIRE(dog.breed == "Husky");
}

TEST_CASE("hybrid json: from_json with type_def — missing keys keep defaults", "[hybrid][from_json][json]") {
    auto j = json{{"name", "Rex"}};

    auto dog = def_type::from_json<HybridDog>(j, hybrid_dog_type);

    REQUIRE(dog.name == "Rex");
    REQUIRE(dog.age == 0);
    REQUIRE(dog.breed == "");
}

TEST_CASE("hybrid json: from_json with type_def — extra keys ignored", "[hybrid][from_json][json]") {
    auto j = json{{"name", "Rex"}, {"age", 3}, {"unknown", "ignored"}};

    auto dog = def_type::from_json<HybridDog>(j, hybrid_dog_type);

    REQUIRE(dog.name == "Rex");
    REQUIRE(dog.age == 3);
}

TEST_CASE("hybrid json: from_json with type_def — empty JSON gives defaults", "[hybrid][from_json][json]") {
    auto dog = def_type::from_json<HybridDog>(json::object(), hybrid_dog_type);

    REQUIRE(dog.name == "");
    REQUIRE(dog.age == 0);
}

// ── Round-trip ──────────────────────────────────────────────────────────

TEST_CASE("hybrid json: round-trip to_json then from_json", "[hybrid][json][roundtrip]") {
    HybridDog original;
    original.name = "Buddy";
    original.age = 5;
    original.breed = "Golden";

    auto j = to_json(original, hybrid_dog_type);
    auto restored = def_type::from_json<HybridDog>(j, hybrid_dog_type);

    REQUIRE(restored.name == "Buddy");
    REQUIRE(restored.age == 5);
    REQUIRE(restored.breed == "Golden");
}

// ── Error cases ─────────────────────────────────────────────────────────

TEST_CASE("hybrid json: from_json throws on non-object", "[hybrid][from_json][json][throw]") {
    REQUIRE_THROWS_AS(def_type::from_json<HybridDog>(json(42), hybrid_dog_type), std::logic_error);
    REQUIRE_THROWS_AS(def_type::from_json<HybridDog>(json::array(), hybrid_dog_type), std::logic_error);
}

TEST_CASE("hybrid json: from_json throws on type mismatch", "[hybrid][from_json][json][throw]") {
    auto j = json{{"name", 42}};  // name should be string
    REQUIRE_THROWS_AS(def_type::from_json<HybridDog>(j, hybrid_dog_type), std::logic_error);
}
