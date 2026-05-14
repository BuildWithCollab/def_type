#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>
#include <optional>

#include <def_type.hpp>

using namespace def_type;

// ── Test types ──────────────────────────────────────────────────────────

namespace test_from_toml_types {

struct SimpleDog {
    field<std::string> name;
    field<int>         age;
    field<bool>        good_boy;
};

struct Network {
    field<std::string> host;
    field<int>         port;
};

struct AppConfig {
    field<std::string> app_name;
    field<Network>     network;
};

struct Server {
    field<std::string> host;
    field<int>         port;
};

struct Cluster {
    field<std::string>          name;
    field<std::vector<Server>>  servers;
};

struct WithOptional {
    field<std::string>                 name;
    field<std::optional<std::string>>  nickname;
};

}  // namespace test_from_toml_types

#ifndef DEF_TYPE_HAS_PFR
template <> constexpr auto def_type::struct_info<test_from_toml_types::SimpleDog>() {
    return def_type::field_info<test_from_toml_types::SimpleDog>("name", "age", "good_boy");
}
template <> constexpr auto def_type::struct_info<test_from_toml_types::Network>() {
    return def_type::field_info<test_from_toml_types::Network>("host", "port");
}
template <> constexpr auto def_type::struct_info<test_from_toml_types::AppConfig>() {
    return def_type::field_info<test_from_toml_types::AppConfig>("app_name", "network");
}
template <> constexpr auto def_type::struct_info<test_from_toml_types::Server>() {
    return def_type::field_info<test_from_toml_types::Server>("host", "port");
}
template <> constexpr auto def_type::struct_info<test_from_toml_types::Cluster>() {
    return def_type::field_info<test_from_toml_types::Cluster>("name", "servers");
}
template <> constexpr auto def_type::struct_info<test_from_toml_types::WithOptional>() {
    return def_type::field_info<test_from_toml_types::WithOptional>("name", "nickname");
}
#endif

// ═════════════════════════════════════════════════════════════════════════
// from_toml — TOML string → toml_doc<T>
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("from_toml: primitive fields", "[from_toml]") {
    using namespace test_from_toml_types;

    auto doc = from_toml<SimpleDog>(R"(
        name = "Rex"
        age = 3
        good_boy = true
    )");

    REQUIRE(doc->name.value     == "Rex");
    REQUIRE(doc->age.value      == 3);
    REQUIRE(doc->good_boy.value == true);
}

TEST_CASE("from_toml: nested struct from [section]", "[from_toml]") {
    using namespace test_from_toml_types;

    auto doc = from_toml<AppConfig>(R"(
        app_name = "myapp"

        [network]
        host = "localhost"
        port = 8080
    )");

    REQUIRE(doc->app_name.value          == "myapp");
    REQUIRE(doc->network->host.value     == "localhost");
    REQUIRE(doc->network->port.value     == 8080);
}

TEST_CASE("from_toml: vector<struct> from [[array_of_tables]]", "[from_toml]") {
    using namespace test_from_toml_types;

    auto doc = from_toml<Cluster>(R"(
        name = "production"

        [[servers]]
        host = "a.example.com"
        port = 1

        [[servers]]
        host = "b.example.com"
        port = 2
    )");

    REQUIRE(doc->name.value              == "production");
    REQUIRE(doc->servers->size()          == 2);
    REQUIRE(doc->servers.value[0].host.value == "a.example.com");
    REQUIRE(doc->servers.value[0].port.value == 1);
    REQUIRE(doc->servers.value[1].host.value == "b.example.com");
    REQUIRE(doc->servers.value[1].port.value == 2);
}

TEST_CASE("from_toml: missing key keeps struct default", "[from_toml]") {
    using namespace test_from_toml_types;

    auto doc = from_toml<SimpleDog>(R"(
        name = "Buddy"
    )");

    REQUIRE(doc->name.value     == "Buddy");
    REQUIRE(doc->age.value      == 0);          // default, no key in TOML
    REQUIRE(doc->good_boy.value == false);      // default
}

TEST_CASE("from_toml: optional present and absent", "[from_toml]") {
    using namespace test_from_toml_types;

    auto with_nick = from_toml<WithOptional>(R"(
        name = "Mr. Whiskers"
        nickname = "Whiskey"
    )");
    REQUIRE(with_nick->name.value             == "Mr. Whiskers");
    REQUIRE(with_nick->nickname->has_value());
    REQUIRE(with_nick->nickname.value.value() == "Whiskey");

    auto without_nick = from_toml<WithOptional>(R"(
        name = "Anonymous"
    )");
    REQUIRE(without_nick->name.value           == "Anonymous");
    REQUIRE_FALSE(without_nick->nickname->has_value());
}

// ═════════════════════════════════════════════════════════════════════════
// toml_doc — value access shape
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("toml_doc: operator-> and operator*", "[from_toml][toml_doc]") {
    using namespace test_from_toml_types;

    auto doc = from_toml<SimpleDog>(R"(
        name = "Rex"
        age = 3
        good_boy = true
    )");

    // operator->
    REQUIRE(doc->name.value == "Rex");

    // operator*
    SimpleDog& ref = *doc;
    REQUIRE(ref.age.value == 3);

    // source AST is held alongside
    REQUIRE(doc.source.is_table());
    REQUIRE(doc.source.contains("name"));
}

TEST_CASE("toml_doc: mutation via operator-> persists", "[from_toml][toml_doc]") {
    using namespace test_from_toml_types;

    auto doc = from_toml<SimpleDog>(R"(
        name = "Rex"
        age = 3
        good_boy = true
    )");

    doc->age = 7;
    doc->name = "Renamed";

    REQUIRE(doc->age.value  == 7);
    REQUIRE(doc->name.value == "Renamed");
}

// ═════════════════════════════════════════════════════════════════════════
// Round-trip — the whole reason we chose toml11
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("round-trip: comments survive parse → mutate → emit", "[from_toml][round_trip]") {
    using namespace test_from_toml_types;

    const std::string source = R"(# Top-of-file comment
# Second line of header

name = "Rex" # the dog's name
age = 3
good_boy = true
)";

    auto doc = from_toml<SimpleDog>(source);
    doc->age = 7;                  // mutate one field

    auto out = to_toml_string(doc);

    // Data survived
    REQUIRE(out.find("age = 7")  != std::string::npos);
    REQUIRE(out.find("Rex")      != std::string::npos);

    // Comments survived
    REQUIRE(out.find("Top-of-file comment")     != std::string::npos);
    REQUIRE(out.find("Second line of header")    != std::string::npos);
    REQUIRE(out.find("the dog's name")           != std::string::npos);
}

TEST_CASE("round-trip: source keys unknown to the struct are preserved", "[from_toml][round_trip]") {
    using namespace test_from_toml_types;

    // SimpleDog only has name/age/good_boy — but TOML also has "favorite_toy"
    const std::string source = R"(
name = "Rex"
age = 3
good_boy = true
favorite_toy = "tennis ball"
)";

    auto doc = from_toml<SimpleDog>(source);
    doc->age = 7;

    auto out = to_toml_string(doc);

    REQUIRE(out.find("age = 7")                     != std::string::npos);
    REQUIRE(out.find("favorite_toy = \"tennis ball\"") != std::string::npos);
}
