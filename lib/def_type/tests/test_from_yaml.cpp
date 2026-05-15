#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <vector>

#include <def_type.hpp>

using namespace def_type;

// ── Test types ──────────────────────────────────────────────────────────

namespace test_from_yaml_types {

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

}  // namespace test_from_yaml_types

#ifndef DEF_TYPE_HAS_PFR
template <> constexpr auto def_type::struct_info<test_from_yaml_types::SimpleDog>() {
    return def_type::field_info<test_from_yaml_types::SimpleDog>("name", "age", "good_boy");
}
template <> constexpr auto def_type::struct_info<test_from_yaml_types::Network>() {
    return def_type::field_info<test_from_yaml_types::Network>("host", "port");
}
template <> constexpr auto def_type::struct_info<test_from_yaml_types::AppConfig>() {
    return def_type::field_info<test_from_yaml_types::AppConfig>("app_name", "network");
}
template <> constexpr auto def_type::struct_info<test_from_yaml_types::Server>() {
    return def_type::field_info<test_from_yaml_types::Server>("host", "port");
}
template <> constexpr auto def_type::struct_info<test_from_yaml_types::Cluster>() {
    return def_type::field_info<test_from_yaml_types::Cluster>("name", "servers");
}
template <> constexpr auto def_type::struct_info<test_from_yaml_types::WithOptional>() {
    return def_type::field_info<test_from_yaml_types::WithOptional>("name", "nickname");
}
#endif

// ═════════════════════════════════════════════════════════════════════════
// from_yaml — YAML string → typed struct
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("from_yaml: primitive fields", "[from_yaml]") {
    using namespace test_from_yaml_types;

    auto dog = from_yaml<SimpleDog>(R"(
name: Rex
age: 3
good_boy: true
)");

    REQUIRE(dog.name.value     == "Rex");
    REQUIRE(dog.age.value      == 3);
    REQUIRE(dog.good_boy.value == true);
}

TEST_CASE("from_yaml: nested mapping", "[from_yaml]") {
    using namespace test_from_yaml_types;

    auto cfg = from_yaml<AppConfig>(R"(
app_name: myapp
network:
  host: localhost
  port: 8080
)");

    REQUIRE(cfg.app_name.value      == "myapp");
    REQUIRE(cfg.network->host.value == "localhost");
    REQUIRE(cfg.network->port.value == 8080);
}

TEST_CASE("from_yaml: sequence of mappings", "[from_yaml]") {
    using namespace test_from_yaml_types;

    auto cluster = from_yaml<Cluster>(R"(
name: production
servers:
  - host: a.example.com
    port: 1
  - host: b.example.com
    port: 2
)");

    REQUIRE(cluster.name.value                  == "production");
    REQUIRE(cluster.servers->size()              == 2);
    REQUIRE(cluster.servers.value[0].host.value == "a.example.com");
    REQUIRE(cluster.servers.value[0].port.value == 1);
    REQUIRE(cluster.servers.value[1].host.value == "b.example.com");
    REQUIRE(cluster.servers.value[1].port.value == 2);
}

TEST_CASE("from_yaml: missing key keeps struct default", "[from_yaml]") {
    using namespace test_from_yaml_types;

    auto dog = from_yaml<SimpleDog>(R"(
name: Buddy
)");

    REQUIRE(dog.name.value     == "Buddy");
    REQUIRE(dog.age.value      == 0);
    REQUIRE(dog.good_boy.value == false);
}

TEST_CASE("from_yaml: optional present and absent", "[from_yaml]") {
    using namespace test_from_yaml_types;

    auto with_nick = from_yaml<WithOptional>(R"(
name: Mr. Whiskers
nickname: Whiskey
)");
    REQUIRE(with_nick.name.value             == "Mr. Whiskers");
    REQUIRE(with_nick.nickname->has_value());
    REQUIRE(with_nick.nickname.value.value() == "Whiskey");

    auto without_nick = from_yaml<WithOptional>(R"(
name: Anonymous
)");
    REQUIRE(without_nick.name.value == "Anonymous");
    REQUIRE_FALSE(without_nick.nickname->has_value());
}

// ═════════════════════════════════════════════════════════════════════════
// Round-trip — same data in, same data out (no comments by design)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("round-trip: data survives parse → emit", "[from_yaml][round_trip]") {
    using namespace test_from_yaml_types;

    SimpleDog original;
    original.name      = "Rex";
    original.age       = 5;
    original.good_boy  = true;

    auto str       = to_yaml_string(original);
    auto restored  = from_yaml<SimpleDog>(str);

    REQUIRE(restored.name.value     == "Rex");
    REQUIRE(restored.age.value      == 5);
    REQUIRE(restored.good_boy.value == true);
}
