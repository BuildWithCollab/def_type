#include <catch2/catch_test_macros.hpp>

#include <ankerl/unordered_dense.h>

#include <map>
#include <string>
#include <unordered_map>

#include <def_type.hpp>

using namespace def_type;

// ═════════════════════════════════════════════════════════════════════════
// is_non_string_keyed_map_v — trait correctness
//
// The trait powers a friendly static_assert in value_to_json /
// value_to_toml / value_from_json / value_from_toml that fires when a
// caller tries to (de)serialize a map whose key type isn't std::string.
// JSON object keys and TOML table keys are both strings — non-string
// keys have no representation in either format.
// ═════════════════════════════════════════════════════════════════════════

// True for non-std::string-keyed maps of every supported map flavor:
static_assert(detail::is_non_string_keyed_map_v<std::map<int, int>>);
static_assert(detail::is_non_string_keyed_map_v<std::map<int, std::string>>);
static_assert(detail::is_non_string_keyed_map_v<std::unordered_map<int, int>>);
static_assert(detail::is_non_string_keyed_map_v<ankerl::unordered_dense::map<int, int>>);

// False for std::string-keyed maps (these are the supported case):
static_assert(!detail::is_non_string_keyed_map_v<std::map<std::string, int>>);
static_assert(!detail::is_non_string_keyed_map_v<std::unordered_map<std::string, int>>);
static_assert(!detail::is_non_string_keyed_map_v<ankerl::unordered_dense::map<std::string, int>>);

// False for non-map types:
static_assert(!detail::is_non_string_keyed_map_v<int>);
static_assert(!detail::is_non_string_keyed_map_v<std::string>);
static_assert(!detail::is_non_string_keyed_map_v<std::vector<int>>);

// ═════════════════════════════════════════════════════════════════════════
// Sanity-check that std::string-keyed maps still serialize fine (i.e.
// the static_assert is correctly scoped to the catch-all branch and
// doesn't break supported cases).
// ═════════════════════════════════════════════════════════════════════════

namespace test_invalid_map_keys_types {

struct GoodConfig {
    field<std::string>                  name;
    field<std::map<std::string, int>>   scores;
};

}  // namespace test_invalid_map_keys_types

#ifndef DEF_TYPE_HAS_PFR
template <> constexpr auto def_type::struct_info<test_invalid_map_keys_types::GoodConfig>() {
    return def_type::field_info<test_invalid_map_keys_types::GoodConfig>("name", "scores");
}
#endif

TEST_CASE("invalid_map_keys: std::string-keyed map still serializes", "[toml][trait]") {
    using namespace test_invalid_map_keys_types;

    GoodConfig cfg;
    cfg.name                       = "stats";
    cfg.scores.value["agility"]    = 9;

    auto out      = to_toml_string(cfg);
    auto restored = from_toml<GoodConfig>(out);

    REQUIRE(restored->name.value                 == "stats");
    REQUIRE(restored->scores.value.at("agility") == 9);
}

// ═════════════════════════════════════════════════════════════════════════
// What a bad case would look like
//
// To verify the user-facing error, flip the `#if 0` to `#if 1` and build.
// You should see a single static_assert line, not a wall of template
// errors:
//
//   error: static assertion failed: def_type: map keys must be
//   std::string for serialization. TOML table keys (like JSON object
//   keys) are strings; non-string-keyed maps have no representation.
//
// Remember to flip it back to `#if 0` before committing.
// ═════════════════════════════════════════════════════════════════════════

#if 0
namespace verify_static_assert_fires {

struct BadConfig {
    field<std::map<int, std::string>> scores;
};

void this_should_not_compile() {
    BadConfig cfg;
    auto out = to_toml_string(cfg);   // ← static_assert in value_to_toml fires here
}

}  // namespace verify_static_assert_fires
#endif
