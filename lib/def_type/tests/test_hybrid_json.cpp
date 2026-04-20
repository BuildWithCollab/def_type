#include "test_json_types.hpp"

// ═════════════════════════════════════════════════════════════════════════
// Plain-struct JSON — to_json / from_json for plain structs via type_def<T>
//
// Plain structs (no field<> members) auto-discovered via PFR:
//   type_def<PlainDog> dog_type;
//
// Overloads: to_json(instance, type_def) and from_json<T>(json, type_def)
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

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<HybridDog>() {
    return def_type::field_info<HybridDog>("name", "age", "breed");
}

template <>
constexpr auto def_type::struct_info<HybridPoint>() {
    return def_type::field_info<HybridPoint>("x", "y");
}
#endif

inline type_def<HybridDog> hybrid_dog_type;

inline type_def<HybridPoint> hybrid_point_type;

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

TEST_CASE("hybrid json: to_json with type_def — all fields auto-discovered", "[hybrid][to_json][json]") {
    // Auto-discovery finds ALL struct members
    type_def<HybridDog> dog_type;

    HybridDog rex;
    rex.name = "Rex";
    rex.age = 3;
    rex.breed = "Husky";

    auto j = to_json(rex, dog_type);

    REQUIRE(j.size() == 3);
    REQUIRE(j["name"] == "Rex");
    REQUIRE(j["age"] == 3);
    REQUIRE(j["breed"] == "Husky");
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
