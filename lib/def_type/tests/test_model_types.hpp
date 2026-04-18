#pragma once

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <def_type/def_type.hpp>

using namespace def_type;

// ═════════════════════════════════════════════════════════════════════════
// Metadata types — shared across all capability test files
// ═════════════════════════════════════════════════════════════════════════

struct endpoint_info {
    const char* path   = "";
    const char* method = "GET";
};

struct help_info {
    const char* summary = "";
};

struct tag_info {
    const char* value = "";
};

struct cli_meta {
    struct { char short_flag = '\0'; } cli;
};

struct render_meta {
    struct { const char* style = ""; int width = 0; } render;
};

// ═════════════════════════════════════════════════════════════════════════
// Typed structs — use field<> and meta<> members
// ═════════════════════════════════════════════════════════════════════════

struct SimpleArgs {
    field<std::string> name;
    field<int>         age;
    field<bool>        active;
};

struct Dog {
    meta<endpoint_info> endpoint{{.path = "/dogs", .method = "POST"}};
    meta<help_info>     help{{.summary = "A good boy"}};

    field<std::string>  name;
    field<int>          age;
    field<std::string>  breed;
};

struct MixedStruct {
    meta<help_info>    help{{.summary = "mixed test"}};

    field<std::string> label;
    int                counter = 0;
    field<int>         score;
};

struct MultiTagged {
    meta<tag_info> tag1{{.value = "pet"}};
    meta<tag_info> tag2{{.value = "animal"}};
    meta<tag_info> tag3{{.value = "good-boy"}};

    field<std::string> name;
};

struct MetaOnly {
    meta<endpoint_info> endpoint{{.path = "/health", .method = "GET"}};
    meta<help_info>     help{{.summary = "health check"}};
};

struct SingleField {
    field<int> value;
};

struct CliArgs {
    meta<help_info> help{{.summary = "CLI tool"}};

    field<std::string>                      query;
    field<bool, with<cli_meta>>             verbose{.with = {{.cli = {.short_flag = 'v'}}}};
    field<int, with<cli_meta, render_meta>> limit{
        .with = {{.cli = {.short_flag = 'l'}}, {.render = {.style = "bold", .width = 10}}}
    };
};

// ═════════════════════════════════════════════════════════════════════════
// Hybrid structs — plain C++ structs, no field<> or meta<>
// ═════════════════════════════════════════════════════════════════════════

struct PlainDog {
    std::string name;
    int         age    = 0;
    std::string breed;
};

struct PlainPoint {
    double x = 0.0;
    double y = 0.0;
};

// ═════════════════════════════════════════════════════════════════════════
// Hybrid structs — with meta<> members (for meta throw tests)
// ═════════════════════════════════════════════════════════════════════════

struct MetaDog {
    meta<help_info>    help{{.summary = "A dog"}};
    field<std::string> name;
    field<int>         age;
};

// ═════════════════════════════════════════════════════════════════════════
// Hybrid structs — with multiple meta<> members of same type
// ═════════════════════════════════════════════════════════════════════════

struct MultiTagDog {
    meta<tag_info> tag1{{.value = "pet"}};
    meta<tag_info> tag2{{.value = "animal"}};
    meta<help_info> help{{.summary = "A tagged dog"}};

    std::string name;
    int         age = 0;
};

// ═════════════════════════════════════════════════════════════════════════
// struct_info fallbacks (non-PFR builds)
// ═════════════════════════════════════════════════════════════════════════

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<SimpleArgs>() {
    return def_type::field_info<SimpleArgs>("name", "age", "active");
}

template <>
constexpr auto def_type::struct_info<Dog>() {
    return def_type::field_info<Dog>("endpoint", "help", "name", "age", "breed");
}

template <>
constexpr auto def_type::struct_info<MixedStruct>() {
    return def_type::field_info<MixedStruct>("help", "label", "counter", "score");
}

template <>
constexpr auto def_type::struct_info<MultiTagged>() {
    return def_type::field_info<MultiTagged>("tag1", "tag2", "tag3", "name");
}

template <>
constexpr auto def_type::struct_info<MetaOnly>() {
    return def_type::field_info<MetaOnly>("endpoint", "help");
}

template <>
constexpr auto def_type::struct_info<SingleField>() {
    return def_type::field_info<SingleField>("value");
}

template <>
constexpr auto def_type::struct_info<CliArgs>() {
    return def_type::field_info<CliArgs>("help", "query", "verbose", "limit");
}

template <>
constexpr auto def_type::struct_info<PlainDog>() {
    return def_type::field_info<PlainDog>("name", "age", "breed");
}

template <>
constexpr auto def_type::struct_info<PlainPoint>() {
    return def_type::field_info<PlainPoint>("x", "y");
}

template <>
constexpr auto def_type::struct_info<MetaDog>() {
    return def_type::field_info<MetaDog>("help", "name", "age");
}

template <>
constexpr auto def_type::struct_info<MultiTagDog>() {
    return def_type::field_info<MultiTagDog>("tag1", "tag2", "help", "name", "age");
}
#endif
