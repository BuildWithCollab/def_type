#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include <ankerl/unordered_dense.h>

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <def_type.hpp>

using namespace def_type;
using json = nlohmann::json;

// ── Forward declarations for struct_info fallbacks ──────────────────────

struct SimpleArgs;
struct WithDefaults;
struct Address;
struct Person;
struct TaggedItem;
struct MaybeNickname;
struct CliArgs;
struct MixedStruct;
struct Team;
struct Config;
struct Labels;
struct Endpoint;
struct ServiceMap;
struct EmptyConfig;
struct TagSet;
struct IdSet;
struct DenseMapStruct;
struct DenseSetStruct;
struct Measurement;
struct FloatStruct;
struct BigNumbers;
struct Inner;
struct Outer;

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<SimpleArgs>() {
    return def_type::field_info<SimpleArgs>("name", "age", "active");
}

template <>
constexpr auto def_type::struct_info<WithDefaults>() {
    return def_type::field_info<WithDefaults>("city", "days");
}

template <>
constexpr auto def_type::struct_info<Address>() {
    return def_type::field_info<Address>("street", "zip");
}

template <>
constexpr auto def_type::struct_info<Person>() {
    return def_type::field_info<Person>("name", "address");
}

template <>
constexpr auto def_type::struct_info<TaggedItem>() {
    return def_type::field_info<TaggedItem>("title", "tags");
}

template <>
constexpr auto def_type::struct_info<MaybeNickname>() {
    return def_type::field_info<MaybeNickname>("name", "nickname");
}

template <>
constexpr auto def_type::struct_info<CliArgs>() {
    return def_type::field_info<CliArgs>("query", "verbose");
}

template <>
constexpr auto def_type::struct_info<MixedStruct>() {
    return def_type::field_info<MixedStruct>("visible", "internal_counter", "score");
}

template <>
constexpr auto def_type::struct_info<Team>() {
    return def_type::field_info<Team>("team_name", "members");
}

template <>
constexpr auto def_type::struct_info<Config>() {
    return def_type::field_info<Config>("name", "settings");
}

template <>
constexpr auto def_type::struct_info<Labels>() {
    return def_type::field_info<Labels>("tags");
}

template <>
constexpr auto def_type::struct_info<Endpoint>() {
    return def_type::field_info<Endpoint>("url", "port");
}

template <>
constexpr auto def_type::struct_info<ServiceMap>() {
    return def_type::field_info<ServiceMap>("services");
}

template <>
constexpr auto def_type::struct_info<EmptyConfig>() {
    return def_type::field_info<EmptyConfig>("settings");
}

template <>
constexpr auto def_type::struct_info<TagSet>() {
    return def_type::field_info<TagSet>("tags");
}

template <>
constexpr auto def_type::struct_info<IdSet>() {
    return def_type::field_info<IdSet>("ids");
}

template <>
constexpr auto def_type::struct_info<DenseMapStruct>() {
    return def_type::field_info<DenseMapStruct>("scores");
}

template <>
constexpr auto def_type::struct_info<DenseSetStruct>() {
    return def_type::field_info<DenseSetStruct>("names");
}

template <>
constexpr auto def_type::struct_info<Measurement>() {
    return def_type::field_info<Measurement>("unit", "value");
}

template <>
constexpr auto def_type::struct_info<FloatStruct>() {
    return def_type::field_info<FloatStruct>("weight");
}

template <>
constexpr auto def_type::struct_info<BigNumbers>() {
    return def_type::field_info<BigNumbers>("signed_big", "unsigned_big");
}

template <>
constexpr auto def_type::struct_info<Inner>() {
    return def_type::field_info<Inner>("x");
}

template <>
constexpr auto def_type::struct_info<Outer>() {
    return def_type::field_info<Outer>("name", "extra");
}
#endif

// ── Test structs ─────────────────────────────────────────────────────────

struct SimpleArgs {
    field<std::string> name;
    field<int>         age;
    field<bool>        active;
};

struct WithDefaults {
    field<std::string> city    {.value = "Portland"};
    field<int>         days    {.value = 7};
};


// ── Nested struct ────────────────────────────────────────────────────────

struct Address {
    field<std::string> street;
    field<std::string> zip;
};

struct Person {
    field<std::string> name;
    field<Address>     address;
};

// ── Vector fields ────────────────────────────────────────────────────────

struct TaggedItem {
    field<std::string>              title;
    field<std::vector<std::string>> tags;
};

// ── Optional fields ──────────────────────────────────────────────────────

struct MaybeNickname {
    field<std::string>                name;
    field<std::optional<std::string>> nickname;
};

// ── Fields with extensions ────────────────────────────────────────────────

struct posix_options {
    char short_flag = '\0';
    bool from_stdin = false;
};

struct posix_meta { posix_options posix{}; };

struct CliArgs {
    field<std::string, with<posix_meta>> query {
        .with = {{.posix = {.short_flag = 'q', .from_stdin = true}}}
    };
    field<bool, with<posix_meta>> verbose {
        .with = {{.posix = {.short_flag = 'v'}}}
    };
};

// ── Mixed Field + non-Field members ──────────────────────────────────────

struct MixedStruct {
    field<std::string> visible;
    int                internal_counter = 0;
    field<int>         score;
};

// ── Vector of nested reflected structs ───────────────────────────────────

struct Team {
    field<std::string>             team_name;
    field<std::vector<SimpleArgs>> members;
};

// ── Map fields ───────────────────────────────────────────────────────────

struct Config {
    field<std::string>                name;
    field<std::map<std::string, int>> settings;
};

struct Labels {
    field<std::unordered_map<std::string, std::string>> tags;
};

struct Endpoint {
    field<std::string> url;
    field<int>         port;
};

struct ServiceMap {
    field<std::map<std::string, Endpoint>> services;
};

struct EmptyConfig {
    field<std::map<std::string, int>> settings;
};

// ── Set fields ───────────────────────────────────────────────────────────

struct TagSet {
    field<std::set<std::string>> tags;
};

struct IdSet {
    field<std::unordered_set<int>> ids;
};

struct DenseMapStruct {
    field<ankerl::unordered_dense::map<std::string, int>> scores;
};

struct DenseSetStruct {
    field<ankerl::unordered_dense::set<std::string>> names;
};

// ── Numeric fields ──────────────────────────────────────────────────────

struct Measurement {
    field<std::string> unit;
    field<double>      value;
};

struct FloatStruct {
    field<float> weight;
};

struct BigNumbers {
    field<int64_t>  signed_big;
    field<uint64_t> unsigned_big;
};

// ── Optional nested structs ─────────────────────────────────────────────

struct Inner {
    field<int> x;
};

struct Outer {
    field<std::string>          name;
    field<std::optional<Inner>> extra;
};

// ── Tests ────────────────────────────────────────────────────────────────

TEST_CASE("to_json with primitive Field types", "[field][json]") {
    SimpleArgs args;
    args.name = "Alice";
    args.age = 30;
    args.active = true;

    auto j = to_json(args);

    REQUIRE(j["name"] == "Alice");
    REQUIRE(j["age"] == 30);
    REQUIRE(j["active"] == true);
}

TEST_CASE("to_json preserves default values", "[field][json]") {
    WithDefaults w{};
    auto j = to_json(w);

    REQUIRE(j["city"] == "Portland");
    REQUIRE(j["days"] == 7);
}

TEST_CASE("to_json with nested ReflectedStruct", "[field][json]") {
    Person p;
    p.name = "Bob";
    p.address.value.street = "123 Main St";
    p.address.value.zip = "97201";

    auto j = to_json(p);

    REQUIRE(j["name"] == "Bob");
    REQUIRE(j["address"]["street"] == "123 Main St");
    REQUIRE(j["address"]["zip"] == "97201");
}

TEST_CASE("to_json with vector field", "[field][json]") {
    TaggedItem item;
    item.title = "My Post";
    item.tags.value = {"c++", "modules", "reflection"};

    auto j = to_json(item);

    REQUIRE(j["title"] == "My Post");
    REQUIRE(j["tags"].is_array());
    REQUIRE(j["tags"].size() == 3);
    REQUIRE(j["tags"][0] == "c++");
    REQUIRE(j["tags"][1] == "modules");
    REQUIRE(j["tags"][2] == "reflection");
}

TEST_CASE("to_json with optional field — present", "[field][json]") {
    MaybeNickname m;
    m.name = "Alice";
    m.nickname.value = "Ali";

    auto j = to_json(m);

    REQUIRE(j["name"] == "Alice");
    REQUIRE(j["nickname"] == "Ali");
}

TEST_CASE("to_json with optional field — absent", "[field][json]") {
    MaybeNickname m;
    m.name = "Bob";
    // nickname left as std::nullopt

    auto j = to_json(m);

    REQUIRE(j["name"] == "Bob");
    REQUIRE(j["nickname"].is_null());
}

TEST_CASE("to_json with extensions — serializes value, not metadata", "[field][json][with]") {
    CliArgs args;
    args.query = "hello world";
    args.verbose = true;

    auto j = to_json(args);

    REQUIRE(j["query"] == "hello world");
    REQUIRE(j["verbose"] == true);
    REQUIRE(j.size() == 2);
    // Extension metadata (posix short_flag, from_stdin) does NOT appear
    // in the data JSON — only .value is serialized.
    REQUIRE(!j.contains("with"));
    REQUIRE(!j.contains("posix"));
    REQUIRE(!j.contains("short_flag"));
}

TEST_CASE("to_json includes all non-meta members", "[field][json]") {
    MixedStruct ms;
    ms.visible = "hello";
    ms.internal_counter = 999;
    ms.score = 42;

    auto j = to_json(ms);

    REQUIRE(j["visible"] == "hello");
    REQUIRE(j["score"] == 42);
    REQUIRE(j["internal_counter"] == 999);
    REQUIRE(j.size() == 3);
}

TEST_CASE("to_json with vector of nested reflected structs", "[field][json]") {
    Team t;
    t.team_name = "Pirates";

    SimpleArgs a1;
    a1.name = "Alice";
    a1.age = 30;
    a1.active = true;

    SimpleArgs a2;
    a2.name = "Bob";
    a2.age = 25;
    a2.active = false;

    t.members.value = {a1, a2};

    auto j = to_json(t);

    REQUIRE(j["team_name"] == "Pirates");
    REQUIRE(j["members"].is_array());
    REQUIRE(j["members"].size() == 2);
    REQUIRE(j["members"][0]["name"] == "Alice");
    REQUIRE(j["members"][0]["age"] == 30);
    REQUIRE(j["members"][1]["name"] == "Bob");
    REQUIRE(j["members"][1]["active"] == false);
}

TEST_CASE("to_json with empty vector", "[field][json]") {
    TaggedItem item;
    item.title = "Empty";
    // tags left as empty vector

    auto j = to_json(item);

    REQUIRE(j["tags"].is_array());
    REQUIRE(j["tags"].empty());
}

TEST_CASE("to_json with double field", "[field][json]") {
    Measurement m;
    m.unit = "celsius";
    m.value = 72.5;

    auto j = to_json(m);

    REQUIRE(j["unit"] == "celsius");
    REQUIRE(j["value"] == 72.5);
}

TEST_CASE("to_json with float field", "[field][json]") {
    FloatStruct fs;
    fs.weight.value = 3.14f;

    auto j = to_json(fs);
    REQUIRE(j["weight"].is_number_float());
    REQUIRE(j["weight"].get<float>() == Catch::Approx(3.14f));
}

TEST_CASE("to_json with int64_t and uint64_t fields", "[field][json]") {
    BigNumbers bn;
    bn.signed_big.value = 9'000'000'000LL;
    bn.unsigned_big.value = 18'000'000'000ULL;

    auto j = to_json(bn);
    REQUIRE(j["signed_big"] == 9'000'000'000LL);
    REQUIRE(j["unsigned_big"] == 18'000'000'000ULL);
}

TEST_CASE("to_json with optional ReflectedStruct — present", "[field][json]") {
    Outer o;
    o.name = "test";
    o.extra.value = Inner{};
    o.extra.value->x.value = 42;

    auto j = to_json(o);
    REQUIRE(j["name"] == "test");
    REQUIRE(j["extra"].is_object());
    REQUIRE(j["extra"]["x"] == 42);
}

TEST_CASE("to_json with optional ReflectedStruct — absent", "[field][json]") {
    Outer o;
    o.name = "test";

    auto j = to_json(o);
    REQUIRE(j["name"] == "test");
    REQUIRE(j["extra"].is_null());
}

// ── Map fields ───────────────────────────────────────────────────────────

TEST_CASE("to_json with std::map<string, V>", "[field][json][map]") {
    Config c;
    c.name = "my-app";
    c.settings.value = {{"timeout", 30}, {"retries", 3}};

    auto j = to_json(c);

    REQUIRE(j["name"] == "my-app");
    REQUIRE(j["settings"].is_object());
    REQUIRE(j["settings"]["timeout"] == 30);
    REQUIRE(j["settings"]["retries"] == 3);
}

TEST_CASE("to_json with std::unordered_map<string, V>", "[field][json][map]") {
    Labels l;
    l.tags.value = {{"env", "prod"}, {"region", "us-west"}};

    auto j = to_json(l);

    REQUIRE(j["tags"].is_object());
    REQUIRE(j["tags"]["env"] == "prod");
    REQUIRE(j["tags"]["region"] == "us-west");
}

TEST_CASE("to_json with map of reflected structs as values", "[field][json][map]") {
    ServiceMap sm;
    Endpoint api;
    api.url = "https://api.example.com";
    api.port = 443;
    Endpoint db;
    db.url = "localhost";
    db.port = 5432;
    sm.services.value = {{"api", api}, {"db", db}};

    auto j = to_json(sm);

    REQUIRE(j["services"]["api"]["url"] == "https://api.example.com");
    REQUIRE(j["services"]["api"]["port"] == 443);
    REQUIRE(j["services"]["db"]["url"] == "localhost");
    REQUIRE(j["services"]["db"]["port"] == 5432);
}

TEST_CASE("to_json with empty map", "[field][json][map]") {
    EmptyConfig ec;
    auto j = to_json(ec);

    REQUIRE(j["settings"].is_object());
    REQUIRE(j["settings"].empty());
}

// ── Set fields ───────────────────────────────────────────────────────────

TEST_CASE("to_json with std::set", "[field][json][set]") {
    TagSet ts;
    ts.tags.value = {"alpha", "beta", "gamma"};

    auto j = to_json(ts);

    REQUIRE(j["tags"].is_array());
    REQUIRE(j["tags"].size() == 3);
    // std::set is sorted, so order is deterministic
    REQUIRE(j["tags"][0] == "alpha");
    REQUIRE(j["tags"][1] == "beta");
    REQUIRE(j["tags"][2] == "gamma");
}

TEST_CASE("to_json with std::unordered_set", "[field][json][set]") {
    IdSet is;
    is.ids.value = {10, 20, 30};

    auto j = to_json(is);

    REQUIRE(j["ids"].is_array());
    REQUIRE(j["ids"].size() == 3);
}

TEST_CASE("to_json with empty set", "[field][json][set]") {
    TagSet ts;

    auto j = to_json(ts);

    REQUIRE(j["tags"].is_array());
    REQUIRE(j["tags"].empty());
}

// ── ankerl::unordered_dense ───────────────────────────────────────────────

TEST_CASE("to_json with ankerl::unordered_dense::map", "[field][json][dense]") {
    DenseMapStruct dm;
    dm.scores.value = {{"alice", 100}, {"bob", 85}};

    auto j = to_json(dm);

    REQUIRE(j["scores"].is_object());
    REQUIRE(j["scores"]["alice"] == 100);
    REQUIRE(j["scores"]["bob"] == 85);
}

TEST_CASE("to_json with ankerl::unordered_dense::set", "[field][json][dense]") {
    DenseSetStruct ds;
    ds.names.value = {"x", "y", "z"};

    auto j = to_json(ds);

    REQUIRE(j["names"].is_array());
    REQUIRE(j["names"].size() == 3);
}

// ── to_json_string ───────────────────────────────────────────────────────

TEST_CASE("to_json_string compact output", "[field][json][string]") {
    WithDefaults w{};
    std::string s = to_json_string(w);

    auto j = json::parse(s);
    REQUIRE(j.at("city") == json("Portland"));
    REQUIRE(j.at("days") == json(7));
    REQUIRE(s.find('\n') == std::string::npos);
}

TEST_CASE("to_json_string pretty output", "[field][json][string]") {
    WithDefaults w{};
    std::string s = to_json_string(w, 2);

    auto j = json::parse(s);
    REQUIRE(j.at("city") == json("Portland"));
    REQUIRE(j.at("days") == json(7));
    REQUIRE(s.find('\n') != std::string::npos);
}
