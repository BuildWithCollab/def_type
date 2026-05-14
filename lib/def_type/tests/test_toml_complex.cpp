#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <ankerl/unordered_dense.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <def_type.hpp>

using namespace def_type;

// ── Test types ──────────────────────────────────────────────────────────

namespace test_toml_complex_types {

// Map of string to primitive
struct ScoresConfig {
    field<std::string>                   name;
    field<std::map<std::string, int>>    scores;
};

// Map of string to struct — emits as [scores.key] sections
struct Endpoint {
    field<std::string> host;
    field<int>         port;
};

struct EndpointMap {
    field<std::string>                          name;
    field<std::map<std::string, Endpoint>>      endpoints;
};

// unordered_map variant
struct UnorderedScoresConfig {
    field<std::string>                                    name;
    field<std::unordered_map<std::string, int>>           scores;
};

// dense map variant
struct DenseScoresConfig {
    field<std::string>                                              name;
    field<ankerl::unordered_dense::map<std::string, int>>           scores;
};

// Enum field
enum class Mood { happy, grumpy, sleepy };

struct PetWithMood {
    field<std::string> name;
    field<Mood>        mood;
};

// Deep nesting — 3 levels
struct Leaf {
    field<std::string> value;
};
struct Middle {
    field<std::string> label;
    field<Leaf>        leaf;
};
struct Root {
    field<std::string> name;
    field<Middle>      middle;
};

// Optional nested struct
struct Address {
    field<std::string> street;
    field<std::string> zip;
};
struct MaybeAddress {
    field<std::string>            name;
    field<std::optional<Address>> address;
};

// vector of vectors
struct Matrix {
    field<std::string>                          label;
    field<std::vector<std::vector<int>>>        rows;
};

// Wide numerics
struct WideNumbers {
    field<std::int64_t> big_int;
    field<double>       precise;
};

}  // namespace test_toml_complex_types

#ifndef DEF_TYPE_HAS_PFR
template <> constexpr auto def_type::struct_info<test_toml_complex_types::ScoresConfig>() {
    return def_type::field_info<test_toml_complex_types::ScoresConfig>("name", "scores");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::Endpoint>() {
    return def_type::field_info<test_toml_complex_types::Endpoint>("host", "port");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::EndpointMap>() {
    return def_type::field_info<test_toml_complex_types::EndpointMap>("name", "endpoints");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::UnorderedScoresConfig>() {
    return def_type::field_info<test_toml_complex_types::UnorderedScoresConfig>("name", "scores");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::DenseScoresConfig>() {
    return def_type::field_info<test_toml_complex_types::DenseScoresConfig>("name", "scores");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::PetWithMood>() {
    return def_type::field_info<test_toml_complex_types::PetWithMood>("name", "mood");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::Leaf>() {
    return def_type::field_info<test_toml_complex_types::Leaf>("value");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::Middle>() {
    return def_type::field_info<test_toml_complex_types::Middle>("label", "leaf");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::Root>() {
    return def_type::field_info<test_toml_complex_types::Root>("name", "middle");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::Address>() {
    return def_type::field_info<test_toml_complex_types::Address>("street", "zip");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::MaybeAddress>() {
    return def_type::field_info<test_toml_complex_types::MaybeAddress>("name", "address");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::Matrix>() {
    return def_type::field_info<test_toml_complex_types::Matrix>("label", "rows");
}
template <> constexpr auto def_type::struct_info<test_toml_complex_types::WideNumbers>() {
    return def_type::field_info<test_toml_complex_types::WideNumbers>("big_int", "precise");
}
#endif

// ═════════════════════════════════════════════════════════════════════════
// std::map<string, T> — primitive value
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("complex: map<string,int> round-trips", "[toml][complex][map]") {
    using namespace test_toml_complex_types;

    ScoresConfig cfg;
    cfg.name                          = "stats";
    cfg.scores.value["agility"]       = 9;
    cfg.scores.value["cuteness"]      = 10;

    auto out = to_toml_string(cfg);
    auto restored = from_toml<ScoresConfig>(out);

    REQUIRE(restored->name.value                == "stats");
    REQUIRE(restored->scores.value.at("agility")  == 9);
    REQUIRE(restored->scores.value.at("cuteness") == 10);
}

// ═════════════════════════════════════════════════════════════════════════
// std::map<string, Struct> — emits as [section] per key
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("complex: map<string,struct> round-trips", "[toml][complex][map]") {
    using namespace test_toml_complex_types;

    EndpointMap cfg;
    cfg.name = "production";

    Endpoint a; a.host = "a.example.com"; a.port = 80;
    Endpoint b; b.host = "b.example.com"; b.port = 443;
    cfg.endpoints.value.emplace("primary", a);
    cfg.endpoints.value.emplace("backup",  b);

    auto out = to_toml_string(cfg);
    auto restored = from_toml<EndpointMap>(out);

    REQUIRE(restored->name.value                                   == "production");
    REQUIRE(restored->endpoints.value.at("primary").host.value     == "a.example.com");
    REQUIRE(restored->endpoints.value.at("primary").port.value     == 80);
    REQUIRE(restored->endpoints.value.at("backup").host.value      == "b.example.com");
    REQUIRE(restored->endpoints.value.at("backup").port.value      == 443);
}

// ═════════════════════════════════════════════════════════════════════════
// unordered_map and dense_map work the same way
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("complex: unordered_map<string,int> round-trips", "[toml][complex][map]") {
    using namespace test_toml_complex_types;

    UnorderedScoresConfig cfg;
    cfg.name                     = "stats";
    cfg.scores.value["agility"]  = 9;

    auto out = to_toml_string(cfg);
    auto restored = from_toml<UnorderedScoresConfig>(out);

    REQUIRE(restored->name.value                 == "stats");
    REQUIRE(restored->scores.value.at("agility") == 9);
}

TEST_CASE("complex: ankerl dense_map<string,int> round-trips", "[toml][complex][map]") {
    using namespace test_toml_complex_types;

    DenseScoresConfig cfg;
    cfg.name                     = "stats";
    cfg.scores.value["agility"]  = 9;

    auto out = to_toml_string(cfg);
    auto restored = from_toml<DenseScoresConfig>(out);

    REQUIRE(restored->name.value                 == "stats");
    REQUIRE(restored->scores.value.at("agility") == 9);
}

// ═════════════════════════════════════════════════════════════════════════
// Enum class — serialized as string via magic_enum
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("complex: enum class round-trips as string", "[toml][complex][enum]") {
    using namespace test_toml_complex_types;

    PetWithMood pet;
    pet.name  = "Sleepy";
    pet.mood  = Mood::sleepy;

    auto out = to_toml_string(pet);
    REQUIRE(out.find("mood = \"sleepy\"") != std::string::npos);

    auto restored = from_toml<PetWithMood>(out);
    REQUIRE(restored->name.value == "Sleepy");
    REQUIRE(restored->mood.value == Mood::sleepy);
}

TEST_CASE("complex: enum round-trip preserves comments", "[toml][complex][enum][round_trip]") {
    using namespace test_toml_complex_types;

    const std::string source = R"(# Pet definition
name = "Grumpy Cat"
mood = "grumpy"  # the cat's mood
)";

    auto doc = from_toml<PetWithMood>(source);
    REQUIRE(doc->mood.value == Mood::grumpy);

    doc->mood = Mood::happy;
    auto out = to_toml_string(doc);

    REQUIRE(out.find("mood = \"happy\"")     != std::string::npos);
    REQUIRE(out.find("Pet definition")        != std::string::npos);
    REQUIRE(out.find("the cat's mood")        != std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════
// 3-level deep struct nesting
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("complex: 3-level deep nesting round-trips", "[toml][complex][nesting]") {
    using namespace test_toml_complex_types;

    Root r;
    r.name                       = "top";
    r.middle->label              = "mid";
    r.middle->leaf->value        = "deep";

    auto out = to_toml_string(r);
    auto restored = from_toml<Root>(out);

    REQUIRE(restored->name.value                  == "top");
    REQUIRE(restored->middle->label.value         == "mid");
    REQUIRE(restored->middle->leaf->value.value   == "deep");
}

// ═════════════════════════════════════════════════════════════════════════
// std::optional<NestedStruct>
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("complex: optional<struct> present round-trips", "[toml][complex][optional]") {
    using namespace test_toml_complex_types;

    MaybeAddress p;
    p.name = "Alice";
    Address addr;
    addr.street = "123 Main St";
    addr.zip    = "97201";
    p.address.value = addr;

    auto out = to_toml_string(p);
    auto restored = from_toml<MaybeAddress>(out);

    REQUIRE(restored->name.value                            == "Alice");
    REQUIRE(restored->address.value.has_value());
    REQUIRE(restored->address.value->street.value           == "123 Main St");
    REQUIRE(restored->address.value->zip.value              == "97201");
}

TEST_CASE("complex: optional<struct> absent is omitted", "[toml][complex][optional]") {
    using namespace test_toml_complex_types;

    MaybeAddress p;
    p.name = "Anonymous";
    // address is empty

    auto out = to_toml_string(p);
    REQUIRE(out.find("name = \"Anonymous\"") != std::string::npos);
    REQUIRE(out.find("address")               == std::string::npos);

    auto restored = from_toml<MaybeAddress>(out);
    REQUIRE(restored->name.value == "Anonymous");
    REQUIRE_FALSE(restored->address.value.has_value());
}

// ═════════════════════════════════════════════════════════════════════════
// vector<vector<T>>
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("complex: vector<vector<int>> round-trips", "[toml][complex][vector]") {
    using namespace test_toml_complex_types;

    Matrix m;
    m.label = "id";
    m.rows.value = {{1, 0}, {0, 1}};

    auto out = to_toml_string(m);
    auto restored = from_toml<Matrix>(out);

    REQUIRE(restored->label.value      == "id");
    REQUIRE(restored->rows.value.size() == 2);
    REQUIRE(restored->rows.value[0]    == std::vector<int>{1, 0});
    REQUIRE(restored->rows.value[1]    == std::vector<int>{0, 1});
}

// ═════════════════════════════════════════════════════════════════════════
// Wide numeric types
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("complex: int64 and double round-trip", "[toml][complex][numeric]") {
    using namespace test_toml_complex_types;

    WideNumbers n;
    n.big_int  = std::int64_t{9'000'000'000};   // > 2^31
    n.precise  = 3.14159265358979;

    auto out = to_toml_string(n);
    auto restored = from_toml<WideNumbers>(out);

    REQUIRE(restored->big_int.value == std::int64_t{9'000'000'000});
    REQUIRE(restored->precise.value == Catch::Approx(3.14159265358979));
}
