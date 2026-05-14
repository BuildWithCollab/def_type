#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include <def_type.hpp>

using namespace def_type;

// ── Test types ──────────────────────────────────────────────────────────
//
// These tests attack comment preservation aggressively. The basic
// "header + inline comment on a primitive field" case is covered in
// test_from_toml.cpp; here we go after the harder cases — comments
// inside [section]s, comments inside [[arrays of tables]], comments
// adjacent to deeply nested fields, etc.

namespace test_toml_comments_types {

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

}  // namespace test_toml_comments_types

#ifndef DEF_TYPE_HAS_PFR
template <> constexpr auto def_type::struct_info<test_toml_comments_types::Network>() {
    return def_type::field_info<test_toml_comments_types::Network>("host", "port");
}
template <> constexpr auto def_type::struct_info<test_toml_comments_types::AppConfig>() {
    return def_type::field_info<test_toml_comments_types::AppConfig>("app_name", "network");
}
template <> constexpr auto def_type::struct_info<test_toml_comments_types::Server>() {
    return def_type::field_info<test_toml_comments_types::Server>("host", "port");
}
template <> constexpr auto def_type::struct_info<test_toml_comments_types::Cluster>() {
    return def_type::field_info<test_toml_comments_types::Cluster>("name", "servers");
}
#endif

// ═════════════════════════════════════════════════════════════════════════
// Comments inside a [section]
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("comments: inside a [section] survive mutation", "[toml][comments][round_trip]") {
    using namespace test_toml_comments_types;

    const std::string source = R"(app_name = "myapp"

# Network configuration block
[network]
# Hostname for the server
host = "localhost"
port = 8080  # default listening port
)";

    auto doc = from_toml<AppConfig>(source);
    doc->network->port = 9090;

    auto out = to_toml_string(doc);

    REQUIRE(out.find("Network configuration block")    != std::string::npos);
    REQUIRE(out.find("Hostname for the server")        != std::string::npos);
    REQUIRE(out.find("default listening port")         != std::string::npos);
    REQUIRE(out.find("port = 9090")                    != std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════
// Comments inside [[arrays of tables]]
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("comments: inside [[arrays_of_tables]] survive", "[toml][comments][round_trip]") {
    using namespace test_toml_comments_types;

    const std::string source = R"(name = "production"

# First server
[[servers]]
host = "a.example.com"
port = 1  # primary

# Second server
[[servers]]
host = "b.example.com"
port = 2  # secondary
)";

    auto doc = from_toml<Cluster>(source);
    doc->servers.value[0].port = 11;
    doc->servers.value[1].port = 22;

    auto out = to_toml_string(doc);

    REQUIRE(out.find("First server")    != std::string::npos);
    REQUIRE(out.find("Second server")   != std::string::npos);
    REQUIRE(out.find("primary")         != std::string::npos);
    REQUIRE(out.find("secondary")       != std::string::npos);
    REQUIRE(out.find("port = 11")       != std::string::npos);
    REQUIRE(out.find("port = 22")       != std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════
// Multiple consecutive comment lines above a key
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("comments: multiple consecutive lines preserved", "[toml][comments][round_trip]") {
    using namespace test_toml_comments_types;

    const std::string source = R"(# Line one of header
# Line two of header
# Line three of header

app_name = "myapp"

# Network section description, line 1
# Network section description, line 2
[network]
host = "localhost"
port = 8080
)";

    auto doc = from_toml<AppConfig>(source);
    doc->app_name = "renamed";

    auto out = to_toml_string(doc);

    REQUIRE(out.find("Line one of header")                != std::string::npos);
    REQUIRE(out.find("Line two of header")                != std::string::npos);
    REQUIRE(out.find("Line three of header")              != std::string::npos);
    REQUIRE(out.find("Network section description, line 1") != std::string::npos);
    REQUIRE(out.find("Network section description, line 2") != std::string::npos);
    REQUIRE(out.find("app_name = \"renamed\"")            != std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════
// Mutating a deeply-nested field doesn't disturb comments anywhere else
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("comments: mutation of nested field leaves all other comments untouched",
          "[toml][comments][round_trip]") {
    using namespace test_toml_comments_types;

    const std::string source = R"(# File header
app_name = "myapp"  # the application name

# Network configuration
[network]
host = "localhost"  # bound interface
port = 8080         # listening port
)";

    auto doc = from_toml<AppConfig>(source);
    doc->network->host = "0.0.0.0";   // mutate only the host inside the section

    auto out = to_toml_string(doc);

    REQUIRE(out.find("File header")                  != std::string::npos);
    REQUIRE(out.find("the application name")         != std::string::npos);
    REQUIRE(out.find("Network configuration")        != std::string::npos);
    REQUIRE(out.find("bound interface")              != std::string::npos);
    REQUIRE(out.find("listening port")               != std::string::npos);
    REQUIRE(out.find("host = \"0.0.0.0\"")           != std::string::npos);
    REQUIRE(out.find("port = 8080")                  != std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════
// Source contains a key the struct doesn't model — comment on that key
// must survive too
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("comments: comment on an unknown source key is preserved",
          "[toml][comments][round_trip]") {
    using namespace test_toml_comments_types;

    const std::string source = R"(app_name = "myapp"

# Feature flag — not modeled by AppConfig
experimental_mode = true  # turn this off for prod

[network]
host = "localhost"
port = 8080
)";

    auto doc = from_toml<AppConfig>(source);
    doc->network->port = 9090;

    auto out = to_toml_string(doc);

    REQUIRE(out.find("Feature flag")                       != std::string::npos);
    REQUIRE(out.find("turn this off for prod")             != std::string::npos);
    REQUIRE(out.find("experimental_mode = true")           != std::string::npos);
    REQUIRE(out.find("port = 9090")                        != std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════
// Round-trip without mutation should be (mostly) verbatim. Whitespace is
// not formally guaranteed — toml11 has its own re-emit rules — so we only
// assert content survives, not exact byte equality.
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("comments: round-trip without mutation preserves all content",
          "[toml][comments][round_trip]") {
    using namespace test_toml_comments_types;

    const std::string source = R"(# Top of file
# Below the top

app_name = "myapp"  # the name

# Network block
[network]
host = "localhost"
port = 8080  # default port
)";

    auto doc = from_toml<AppConfig>(source);
    // No mutation — straight re-emit

    auto out = to_toml_string(doc);

    REQUIRE(out.find("Top of file")        != std::string::npos);
    REQUIRE(out.find("Below the top")      != std::string::npos);
    REQUIRE(out.find("the name")           != std::string::npos);
    REQUIRE(out.find("Network block")      != std::string::npos);
    REQUIRE(out.find("default port")       != std::string::npos);
    REQUIRE(out.find("app_name = \"myapp\"") != std::string::npos);
    REQUIRE(out.find("host = \"localhost\"") != std::string::npos);
    REQUIRE(out.find("port = 8080")        != std::string::npos);
}
