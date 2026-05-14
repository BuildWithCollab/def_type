#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>
#include <optional>

#include <def_type.hpp>

using namespace def_type;

// ── Test types ──────────────────────────────────────────────────────────

namespace test_to_toml_types {

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

}  // namespace test_to_toml_types

#ifndef DEF_TYPE_HAS_PFR
template <> constexpr auto def_type::struct_info<test_to_toml_types::SimpleDog>() {
    return def_type::field_info<test_to_toml_types::SimpleDog>("name", "age", "good_boy");
}
template <> constexpr auto def_type::struct_info<test_to_toml_types::Network>() {
    return def_type::field_info<test_to_toml_types::Network>("host", "port");
}
template <> constexpr auto def_type::struct_info<test_to_toml_types::AppConfig>() {
    return def_type::field_info<test_to_toml_types::AppConfig>("app_name", "network");
}
template <> constexpr auto def_type::struct_info<test_to_toml_types::Server>() {
    return def_type::field_info<test_to_toml_types::Server>("host", "port");
}
template <> constexpr auto def_type::struct_info<test_to_toml_types::Cluster>() {
    return def_type::field_info<test_to_toml_types::Cluster>("name", "servers");
}
template <> constexpr auto def_type::struct_info<test_to_toml_types::WithOptional>() {
    return def_type::field_info<test_to_toml_types::WithOptional>("name", "nickname");
}
#endif

// ═════════════════════════════════════════════════════════════════════════
// to_toml — struct → ::toml::value
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("to_toml: primitive fields", "[to_toml]") {
    using namespace test_to_toml_types;

    SimpleDog rex;
    rex.name      = "Rex";
    rex.age       = 3;
    rex.good_boy  = true;

    auto v = to_toml(rex);

    REQUIRE(v.is_table());
    REQUIRE(v.at("name").as_string() == "Rex");
    REQUIRE(v.at("age").as_integer() == 3);
    REQUIRE(v.at("good_boy").as_boolean() == true);
}

TEST_CASE("to_toml_string: primitive fields render as bare keys", "[to_toml]") {
    using namespace test_to_toml_types;

    SimpleDog rex;
    rex.name      = "Rex";
    rex.age       = 3;
    rex.good_boy  = true;

    auto str = to_toml_string(rex);

    // Bare top-level keys
    REQUIRE(str.find("name = \"Rex\"") != std::string::npos);
    REQUIRE(str.find("age = 3")        != std::string::npos);
    REQUIRE(str.find("good_boy = true") != std::string::npos);
}

TEST_CASE("to_toml_string: nested struct renders as [section]", "[to_toml]") {
    using namespace test_to_toml_types;

    AppConfig cfg;
    cfg.app_name             = "myapp";
    cfg.network->host        = "localhost";
    cfg.network->port        = 8080;

    auto str = to_toml_string(cfg);

    REQUIRE(str.find("app_name = \"myapp\"") != std::string::npos);
    REQUIRE(str.find("[network]")            != std::string::npos);
    REQUIRE(str.find("host = \"localhost\"") != std::string::npos);
    REQUIRE(str.find("port = 8080")          != std::string::npos);
}

TEST_CASE("to_toml_string: vector<struct> renders as [[array_of_tables]]", "[to_toml]") {
    using namespace test_to_toml_types;

    Cluster cluster;
    cluster.name = "production";

    Server a;  a.host = "a.example.com";  a.port = 1;
    Server b;  b.host = "b.example.com";  b.port = 2;
    cluster.servers->push_back(a);
    cluster.servers->push_back(b);

    auto str = to_toml_string(cluster);

    REQUIRE(str.find("name = \"production\"") != std::string::npos);
    REQUIRE(str.find("[[servers]]")           != std::string::npos);
    REQUIRE(str.find("a.example.com")         != std::string::npos);
    REQUIRE(str.find("b.example.com")         != std::string::npos);
}

TEST_CASE("to_toml_string: empty std::optional is omitted", "[to_toml]") {
    using namespace test_to_toml_types;

    WithOptional w;
    w.name = "Anonymous";
    // nickname left empty

    auto str = to_toml_string(w);

    REQUIRE(str.find("name = \"Anonymous\"") != std::string::npos);
    REQUIRE(str.find("nickname")              == std::string::npos);
}

TEST_CASE("to_toml_string: present std::optional is emitted", "[to_toml]") {
    using namespace test_to_toml_types;

    WithOptional w;
    w.name      = "Mr. Whiskers";
    w.nickname  = std::optional<std::string>{ "Whiskey" };

    auto str = to_toml_string(w);

    REQUIRE(str.find("name = \"Mr. Whiskers\"") != std::string::npos);
    REQUIRE(str.find("nickname = \"Whiskey\"")  != std::string::npos);
}
