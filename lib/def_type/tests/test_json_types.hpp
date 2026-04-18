#pragma once

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

#include <def_type/def_type.hpp>

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
struct EnumStruct;
struct MultiEnumStruct;
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
struct VecOfVecs;
struct MapOfVecs;
struct VecOfMaps;
struct OptionalVec;
struct VecOfOptionals;
struct DeepInner;
struct DeepMiddle;
struct DeepOuter;
struct DenseMapOfStructs;
struct PersonList;
struct VecOfEnums;
struct MapOfEnums;
struct HJsonDog;
struct HJsonMixed;
struct HJsonMetaOnly;

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

template <>
constexpr auto def_type::struct_info<EnumStruct>() {
    return def_type::field_info<EnumStruct>("method", "name");
}

template <>
constexpr auto def_type::struct_info<MultiEnumStruct>() {
    return def_type::field_info<MultiEnumStruct>("color", "method", "label");
}

template <>
constexpr auto def_type::struct_info<VecOfVecs>() {
    return def_type::field_info<VecOfVecs>("matrix");
}

template <>
constexpr auto def_type::struct_info<MapOfVecs>() {
    return def_type::field_info<MapOfVecs>("grouped");
}

template <>
constexpr auto def_type::struct_info<VecOfMaps>() {
    return def_type::field_info<VecOfMaps>("records");
}

template <>
constexpr auto def_type::struct_info<OptionalVec>() {
    return def_type::field_info<OptionalVec>("maybe_tags");
}

template <>
constexpr auto def_type::struct_info<VecOfOptionals>() {
    return def_type::field_info<VecOfOptionals>("scores");
}

template <>
constexpr auto def_type::struct_info<DeepInner>() {
    return def_type::field_info<DeepInner>("value");
}

template <>
constexpr auto def_type::struct_info<DeepMiddle>() {
    return def_type::field_info<DeepMiddle>("label", "inner");
}

template <>
constexpr auto def_type::struct_info<DeepOuter>() {
    return def_type::field_info<DeepOuter>("name", "middle", "items");
}

template <>
constexpr auto def_type::struct_info<DenseMapOfStructs>() {
    return def_type::field_info<DenseMapOfStructs>("locations");
}

template <>
constexpr auto def_type::struct_info<PersonList>() {
    return def_type::field_info<PersonList>("org", "people");
}

template <>
constexpr auto def_type::struct_info<VecOfEnums>() {
    return def_type::field_info<VecOfEnums>("methods");
}

template <>
constexpr auto def_type::struct_info<MapOfEnums>() {
    return def_type::field_info<MapOfEnums>("palette");
}

template <>
constexpr auto def_type::struct_info<HJsonDog>() {
    return def_type::field_info<HJsonDog>("endpoint", "help", "name", "age", "breed");
}

template <>
constexpr auto def_type::struct_info<HJsonMixed>() {
    return def_type::field_info<HJsonMixed>("help", "label", "counter", "score");
}

template <>
constexpr auto def_type::struct_info<HJsonMetaOnly>() {
    return def_type::field_info<HJsonMetaOnly>("endpoint", "help");
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

struct Address {
    field<std::string> street;
    field<std::string> zip;
};

struct Person {
    field<std::string> name;
    field<Address>     address;
};

struct TaggedItem {
    field<std::string>              title;
    field<std::vector<std::string>> tags;
};

struct MaybeNickname {
    field<std::string>                name;
    field<std::optional<std::string>> nickname;
};

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

struct MixedStruct {
    field<std::string> visible;
    int                internal_counter = 0;
    field<int>         score;
};

struct Team {
    field<std::string>             team_name;
    field<std::vector<SimpleArgs>> members;
};

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

struct Inner {
    field<int> x;
};

struct Outer {
    field<std::string>          name;
    field<std::optional<Inner>> extra;
};

struct VecOfVecs {
    field<std::vector<std::vector<int>>> matrix;
};

struct MapOfVecs {
    field<std::map<std::string, std::vector<std::string>>> grouped;
};

struct VecOfMaps {
    field<std::vector<std::map<std::string, int>>> records;
};

struct OptionalVec {
    field<std::optional<std::vector<std::string>>> maybe_tags;
};

struct VecOfOptionals {
    field<std::vector<std::optional<int>>> scores;
};

struct DeepInner {
    field<std::string> value;
};

struct DeepMiddle {
    field<std::string>  label;
    field<DeepInner>    inner;
};

struct DeepOuter {
    field<std::string>              name;
    field<DeepMiddle>               middle;
    field<std::vector<DeepInner>>   items;
};

struct DenseMapOfStructs {
    field<ankerl::unordered_dense::map<std::string, Address>> locations;
};

struct PersonList {
    field<std::string>         org;
    field<std::vector<Person>> people;
};

// ── Enum types ───────────────────────────────────────────────────────────

enum class HttpMethod { GET, POST, PUT, DELETE_METHOD };

enum class Color : int { red = 0, green = 1, blue = 2 };

struct EnumStruct {
    field<HttpMethod>  method;
    field<std::string> name;
};

struct MultiEnumStruct {
    field<Color>       color;
    field<HttpMethod>  method;
    field<std::string> label;
};

struct VecOfEnums {
    field<std::vector<HttpMethod>> methods;
};

struct MapOfEnums {
    field<std::map<std::string, Color>> palette;
};

// ── Hybrid-JSON structs ──────────────────────────────────────────────────

struct hjson_endpoint_info {
    const char* path   = "";
    const char* method = "GET";
};

struct hjson_help_info {
    const char* summary = "";
};

struct HJsonDog {
    meta<hjson_endpoint_info> endpoint{{.path = "/dogs", .method = "POST"}};
    meta<hjson_help_info>     help{{.summary = "A good boy"}};

    field<std::string>  name;
    field<int>          age;
    field<std::string>  breed;
};

struct HJsonMixed {
    meta<hjson_help_info>  help{{.summary = "mixed test"}};

    field<std::string> label;
    int                counter = 0;
    field<int>         score;
};

struct HJsonMetaOnly {
    meta<hjson_endpoint_info> endpoint{{.path = "/health", .method = "GET"}};
    meta<hjson_help_info>     help{{.summary = "health check"}};
};
