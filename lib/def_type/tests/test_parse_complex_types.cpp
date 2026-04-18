#include <catch2/catch_test_macros.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <def_type/def_type.hpp>

using namespace def_type;
using json = nlohmann::json;

// ═════════════════════════════════════════════════════════════════════════
// Test types — enums, nested structs, containers of structs
// ═════════════════════════════════════════════════════════════════════════

enum class Color { Red, Green, Blue };

struct Address {
    field<std::string> street;
    field<std::string> zip;
};

struct PersonWithAddress {
    field<std::string> name;
    field<Address>     address;
};

struct PersonWithAddresses {
    field<std::string>          name;
    field<std::vector<Address>> addresses;
};

struct PersonWithOptionalAddress {
    field<std::string>            name;
    field<std::optional<Address>> address;
};

struct PersonWithAddressMap {
    field<std::string>                        name;
    field<std::map<std::string, Address>>     locations;
};

struct PetWithColor {
    field<std::string> name;
    field<Color>       color;
};

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<Address>() {
    return def_type::field_info<Address>("street", "zip");
}

template <>
constexpr auto def_type::struct_info<PersonWithAddress>() {
    return def_type::field_info<PersonWithAddress>("name", "address");
}

template <>
constexpr auto def_type::struct_info<PersonWithAddresses>() {
    return def_type::field_info<PersonWithAddresses>("name", "addresses");
}

template <>
constexpr auto def_type::struct_info<PersonWithOptionalAddress>() {
    return def_type::field_info<PersonWithOptionalAddress>("name", "address");
}

template <>
constexpr auto def_type::struct_info<PersonWithAddressMap>() {
    return def_type::field_info<PersonWithAddressMap>("name", "locations");
}

template <>
constexpr auto def_type::struct_info<PetWithColor>() {
    return def_type::field_info<PetWithColor>("name", "color");
}
#endif

// ═════════════════════════════════════════════════════════════════════════
// Baseline: from_json (free function) handles complex types correctly
// These should all pass — they use value_from_json internally.
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("from_json: enum round-trips via magic_enum", "[parse][complex][baseline]") {
    PetWithColor pet;
    pet.name = "Whiskers";
    pet.color = Color::Blue;

    auto j = to_json(pet);
    // magic_enum serializes the enum name — just verify it round-trips
    REQUIRE(j["color"].is_string());

    auto pet2 = from_json<PetWithColor>(j);
    REQUIRE(pet2.color.value == Color::Blue);
}

TEST_CASE("from_json: vector of reflected structs", "[parse][complex][baseline]") {
    json j = {
        {"name", "Alice"},
        {"addresses", json::array({
            {{"street", "123 Main"}, {"zip", "97201"}},
            {{"street", "456 Oak"},  {"zip", "97202"}}
        })}
    };

    auto person = from_json<PersonWithAddresses>(j);
    REQUIRE(person.addresses.value.size() == 2);
    REQUIRE(person.addresses.value[0].street.value == "123 Main");
    REQUIRE(person.addresses.value[1].zip.value == "97202");
}

TEST_CASE("from_json: optional reflected struct (present)", "[parse][complex][baseline]") {
    json j = {
        {"name", "Bob"},
        {"address", {{"street", "789 Pine"}, {"zip", "97203"}}}
    };

    auto person = from_json<PersonWithOptionalAddress>(j);
    REQUIRE(person.address.value.has_value());
    REQUIRE(person.address.value->street.value == "789 Pine");
}

TEST_CASE("from_json: optional reflected struct (null)", "[parse][complex][baseline]") {
    json j = {{"name", "Bob"}, {"address", nullptr}};

    auto person = from_json<PersonWithOptionalAddress>(j);
    REQUIRE(!person.address.value.has_value());
}

TEST_CASE("from_json: map of reflected structs", "[parse][complex][baseline]") {
    json j = {
        {"name", "Carol"},
        {"locations", {
            {"home",   {{"street", "111 Elm"},  {"zip", "97204"}}},
            {"work",   {{"street", "222 Ash"},  {"zip", "97205"}}}
        }}
    };

    auto person = from_json<PersonWithAddressMap>(j);
    REQUIRE(person.locations.value.size() == 2);
    REQUIRE(person.locations.value["home"].street.value == "111 Elm");
    REQUIRE(person.locations.value["work"].zip.value == "97205");
}

// ═════════════════════════════════════════════════════════════════════════
// THE ACTUAL TEST: .parse() on typed path — does it handle complex types?
//
// The spike doc says .parse() uses j[key].template get<InnerType>()
// instead of value_from_json, so enums, vectors/maps/optionals of
// reflected structs should FAIL (silently default).
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed parse: enum field populated correctly", "[parse][complex][typed]") {
    // Serialize first to get the exact string magic_enum produces
    PetWithColor ref;
    ref.name = "Whiskers";
    ref.color = Color::Blue;
    auto ref_json = to_json(ref);

    // Now parse that JSON back — should round-trip correctly
    auto result = type_def<PetWithColor>{}.parse(ref_json);
    REQUIRE(result->color.value == Color::Blue);
    REQUIRE(result->name.value == "Whiskers");
}

TEST_CASE("typed parse: vector of reflected structs populated correctly", "[parse][complex][typed]") {
    json j = {
        {"name", "Alice"},
        {"addresses", json::array({
            {{"street", "123 Main"}, {"zip", "97201"}},
            {{"street", "456 Oak"},  {"zip", "97202"}}
        })}
    };

    auto result = type_def<PersonWithAddresses>{}.parse(j);

    REQUIRE(result->addresses.value.size() == 2);
    REQUIRE(result->addresses.value[0].street.value == "123 Main");
    REQUIRE(result->addresses.value[1].zip.value == "97202");
}

TEST_CASE("typed parse: optional reflected struct populated correctly", "[parse][complex][typed]") {
    json j = {
        {"name", "Bob"},
        {"address", {{"street", "789 Pine"}, {"zip", "97203"}}}
    };

    auto result = type_def<PersonWithOptionalAddress>{}.parse(j);

    REQUIRE(result->address.value.has_value());
    REQUIRE(result->address.value->street.value == "789 Pine");
}

TEST_CASE("typed parse: optional reflected struct null", "[parse][complex][typed]") {
    json j = {{"name", "Bob"}, {"address", nullptr}};

    auto result = type_def<PersonWithOptionalAddress>{}.parse(j);

    REQUIRE(!result->address.value.has_value());
}

TEST_CASE("typed parse: map of reflected structs populated correctly", "[parse][complex][typed]") {
    json j = {
        {"name", "Carol"},
        {"locations", {
            {"home", {{"street", "111 Elm"},  {"zip", "97204"}}},
            {"work", {{"street", "222 Ash"},  {"zip", "97205"}}}
        }}
    };

    auto result = type_def<PersonWithAddressMap>{}.parse(j);

    REQUIRE(result->locations.value.size() == 2);
    REQUIRE(result->locations.value["home"].street.value == "111 Elm");
    REQUIRE(result->locations.value["work"].zip.value == "97205");
}

// ═════════════════════════════════════════════════════════════════════════
// Bring it home: nested vectors of structs with enums inside
// ═════════════════════════════════════════════════════════════════════════

struct ColoredAddress {
    field<std::string> street;
    field<Color>       door_color;
};

struct PersonWithColoredAddresses {
    field<std::string>                 name;
    field<std::vector<ColoredAddress>> addresses;
};

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<ColoredAddress>() {
    return def_type::field_info<ColoredAddress>("street", "door_color");
}

template <>
constexpr auto def_type::struct_info<PersonWithColoredAddresses>() {
    return def_type::field_info<PersonWithColoredAddresses>("name", "addresses");
}
#endif

TEST_CASE("typed parse: vector of structs containing enums", "[parse][complex][typed][nested]") {
    // Build JSON using serialization to get correct enum string casing
    ColoredAddress a1; a1.street = "100 Red Rd";  a1.door_color = Color::Red;
    ColoredAddress a2; a2.street = "200 Blue Bl"; a2.door_color = Color::Blue;
    PersonWithColoredAddresses ref;
    ref.name = "Dave";
    ref.addresses.value = {a1, a2};
    auto j = to_json(ref);

    auto result = type_def<PersonWithColoredAddresses>{}.parse(j);

    REQUIRE(result->addresses.value.size() == 2);
    REQUIRE(result->addresses.value[0].street.value == "100 Red Rd");
    REQUIRE(result->addresses.value[0].door_color.value == Color::Red);
    REQUIRE(result->addresses.value[1].door_color.value == Color::Blue);
}
