#include <catch2/catch_all.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <def_type.hpp>

using namespace def_type;

// Top-level public to_json / from_json for oneof types.
//
// The internal value_to_json / value_from_json have always known how to
// dispatch oneofs — the public to_json / from_json used to be constrained
// to reflected_struct only, which forced consumers to wrap variants in
// carrier structs just to test or codegen them. These tests pin the
// re-enabled top-level public API and the static_assert that catches the
// one form (oneof_by_parent_field) that genuinely cannot work top-level.

namespace top_level_oneof_t {

    // ── shape-only fixtures ──────────────────────────────────────────

    struct TextResource { std::string uri; std::string mimeType; std::string text; };
    struct BlobResource { std::string uri; std::string mimeType; std::string blob; };
    struct LinkResource { std::string uri; std::string mimeType; std::string href; };

    using Resource = oneof<TextResource, BlobResource, LinkResource>;

    // ── inside-object discriminator fixtures ─────────────────────────

    struct TextContent  { std::string type = "text";  std::string text;  };
    struct ImageContent { std::string type = "image"; std::string url; int width; int height; };
    struct AudioContent { std::string type = "audio"; std::string url; int duration_seconds; };

    using Content = oneof_by_field<"type",
        oneof_type<TextContent,  "text">,
        oneof_type<ImageContent, "image">,
        oneof_type<AudioContent, "audio">>;

    // ── outside-object discriminator fixture ─────────────────────────

    struct OuterMessage {
        std::string sender;
        std::string content_type;

        oneof_by_parent_field<&OuterMessage::content_type,
            oneof_type<TextContent,  "text">,
            oneof_type<ImageContent, "image">
        > content = TextContent{};
    };

    // ── ADL-override fixture (lives in its own namespace so ADL can find it) ─

}  // namespace top_level_oneof_t

namespace top_level_oneof_adl_t {
    struct Apple { int weight_grams; };
    struct Pear  { int ripeness;     };
    using Fruit = def_type::oneof<Apple, Pear>;

    inline void to_json(nlohmann::json& j, const Fruit& f) {
        j = nlohmann::json::object();
        f.match(
            [&](const Apple& a) { j["fruit_type"] = "apple"; j["weight_grams"] = a.weight_grams; },
            [&](const Pear&  p) { j["fruit_type"] = "pear";  j["ripeness"]    = p.ripeness;    }
        );
    }

    inline void from_json(const nlohmann::json& j, Fruit& f) {
        if (j.at("fruit_type") == "apple") f = Apple{ j.at("weight_grams").get<int>() };
        else                                f = Pear { j.at("ripeness").get<int>()    };
    }
}

using namespace top_level_oneof_t;

// ═════════════════════════════════════════════════════════════════════════
// A — Top-level oneof (shape-only)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("top-level oneof: to_json — text alt", "[oneof][top-level][shape-only]") {
    Resource r = TextResource{ "file:///hello.txt", "text/plain", "hello" };
    auto j = to_json(r);
    REQUIRE(j["uri"]      == "file:///hello.txt");
    REQUIRE(j["mimeType"] == "text/plain");
    REQUIRE(j["text"]     == "hello");
    REQUIRE_FALSE(j.contains("blob"));
    REQUIRE_FALSE(j.contains("href"));
}

TEST_CASE("top-level oneof: to_json — blob alt", "[oneof][top-level][shape-only]") {
    Resource r = BlobResource{ "file:///pic.png", "image/png", "base64data" };
    auto j = to_json(r);
    REQUIRE(j["blob"] == "base64data");
    REQUIRE_FALSE(j.contains("text"));
}

TEST_CASE("top-level oneof: to_json_string — compact", "[oneof][top-level][shape-only]") {
    Resource r = TextResource{ "u", "m", "t" };
    auto s = to_json_string(r);
    REQUIRE(s.find("\"uri\":\"u\"")  != std::string::npos);
    REQUIRE(s.find("\"text\":\"t\"") != std::string::npos);
    REQUIRE(s.find('\n') == std::string::npos);  // compact, no newlines
}

TEST_CASE("top-level oneof: to_json_string — pretty", "[oneof][top-level][shape-only]") {
    Resource r = TextResource{ "u", "m", "t" };
    auto s = to_json_string(r, 2);
    REQUIRE(s.find('\n') != std::string::npos);  // pretty-printed contains newlines
}

TEST_CASE("top-level oneof: from_json — text alt", "[oneof][top-level][shape-only]") {
    nlohmann::json j = { {"uri","u"}, {"mimeType","text/plain"}, {"text","hi"} };
    auto r = from_json<Resource>(j);
    REQUIRE(r.is<TextResource>());
    REQUIRE(r.as<TextResource>()->text == "hi");
}

TEST_CASE("top-level oneof: from_json — link alt", "[oneof][top-level][shape-only]") {
    nlohmann::json j = { {"uri","u"}, {"mimeType","m"}, {"href","https://x"} };
    auto r = from_json<Resource>(j);
    REQUIRE(r.is<LinkResource>());
    REQUIRE(r.as<LinkResource>()->href == "https://x");
}

TEST_CASE("top-level oneof: from_json — string overload", "[oneof][top-level][shape-only]") {
    // Wrapped in std::string{} to disambiguate from the nlohmann::json overload
    // (a raw `const char[N]` literal is convertible to both — same quirk the
    // reflected_struct overloads have always had).
    auto r = from_json<Resource>(std::string{R"({"uri":"u","mimeType":"text/plain","text":"hi"})"});
    REQUIRE(r.is<TextResource>());
    REQUIRE(r.as<TextResource>()->text == "hi");
}

TEST_CASE("top-level oneof: round-trip — every alt", "[oneof][top-level][shape-only][round-trip]") {
    auto roundtrip = [](Resource r) -> Resource {
        return from_json<Resource>(to_json(r));
    };

    auto t = roundtrip(TextResource{ "a", "b", "c" });
    REQUIRE(t.is<TextResource>());
    REQUIRE(t.as<TextResource>()->text == "c");

    auto b = roundtrip(BlobResource{ "a", "b", "data" });
    REQUIRE(b.is<BlobResource>());
    REQUIRE(b.as<BlobResource>()->blob == "data");

    auto l = roundtrip(LinkResource{ "a", "b", "https://x" });
    REQUIRE(l.is<LinkResource>());
    REQUIRE(l.as<LinkResource>()->href == "https://x");
}

TEST_CASE("top-level oneof: from_json — unknown key throws", "[oneof][top-level][shape-only][errors]") {
    nlohmann::json j = { {"uri","u"}, {"weather","sunny"} };
    REQUIRE_THROWS_AS(from_json<Resource>(j), std::logic_error);
}

TEST_CASE("top-level oneof: from_json — multi-fit empty object throws", "[oneof][top-level][shape-only][errors]") {
    nlohmann::json j = nlohmann::json::object();
    REQUIRE_THROWS_AS(from_json<Resource>(j), std::logic_error);
}

TEST_CASE("top-level oneof: from_json — non-object throws", "[oneof][top-level][shape-only][errors]") {
    REQUIRE_THROWS_AS(from_json<Resource>(nlohmann::json::array()), std::logic_error);
    REQUIRE_THROWS_AS(from_json<Resource>(nlohmann::json("string")), std::logic_error);
}

TEST_CASE("top-level oneof: from_json string overload — parse error propagates", "[oneof][top-level][shape-only][errors]") {
    REQUIRE_THROWS(from_json<Resource>(std::string("{not json")));
}

// ═════════════════════════════════════════════════════════════════════════
// B — Top-level oneof_by_field (inside-object discriminator)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("top-level oneof_by_field: to_json — image alt", "[oneof][top-level][by-field]") {
    Content c = ImageContent{ .url = "https://x/cat.png", .width = 400, .height = 300 };
    auto j = to_json(c);
    REQUIRE(j["type"]   == "image");
    REQUIRE(j["url"]    == "https://x/cat.png");
    REQUIRE(j["width"]  == 400);
    REQUIRE(j["height"] == 300);
}

TEST_CASE("top-level oneof_by_field: to_json_string — compact", "[oneof][top-level][by-field]") {
    Content c = TextContent{ .text = "hi" };
    auto s = to_json_string(c);
    REQUIRE(s.find("\"type\":\"text\"") != std::string::npos);
    REQUIRE(s.find("\"text\":\"hi\"")   != std::string::npos);
}

TEST_CASE("top-level oneof_by_field: to_json_string — pretty", "[oneof][top-level][by-field]") {
    Content c = AudioContent{ .url = "u", .duration_seconds = 90 };
    auto s = to_json_string(c, 4);
    REQUIRE(s.find('\n') != std::string::npos);
}

TEST_CASE("top-level oneof_by_field: from_json — text alt", "[oneof][top-level][by-field]") {
    nlohmann::json j = { {"type","text"}, {"text","hi"} };
    auto c = from_json<Content>(j);
    REQUIRE(c.is<TextContent>());
    REQUIRE(c.as<TextContent>()->text == "hi");
}

TEST_CASE("top-level oneof_by_field: from_json — string overload", "[oneof][top-level][by-field]") {
    auto c = from_json<Content>(std::string{R"({"type":"audio","url":"u","duration_seconds":90})"});
    REQUIRE(c.is<AudioContent>());
    REQUIRE(c.as<AudioContent>()->duration_seconds == 90);
}

TEST_CASE("top-level oneof_by_field: round-trip — every alt", "[oneof][top-level][by-field][round-trip]") {
    auto roundtrip = [](Content c) -> Content {
        return from_json<Content>(to_json(c));
    };

    auto t = roundtrip(TextContent{ .text = "hi" });
    REQUIRE(t.is<TextContent>());
    REQUIRE(t.as<TextContent>()->text == "hi");

    auto i = roundtrip(ImageContent{ .url = "u", .width = 1, .height = 2 });
    REQUIRE(i.is<ImageContent>());
    REQUIRE(i.as<ImageContent>()->width == 1);

    auto a = roundtrip(AudioContent{ .url = "u", .duration_seconds = 7 });
    REQUIRE(a.is<AudioContent>());
    REQUIRE(a.as<AudioContent>()->duration_seconds == 7);
}

TEST_CASE("top-level oneof_by_field: from_json — missing discriminator throws", "[oneof][top-level][by-field][errors]") {
    nlohmann::json j = { {"text","hi"} };
    REQUIRE_THROWS_AS(from_json<Content>(j), std::logic_error);
}

TEST_CASE("top-level oneof_by_field: from_json — unknown discriminator throws", "[oneof][top-level][by-field][errors]") {
    nlohmann::json j = { {"type","video"}, {"url","u"} };
    REQUIRE_THROWS_AS(from_json<Content>(j), std::logic_error);
}

TEST_CASE("top-level oneof_by_field: from_json — non-object throws", "[oneof][top-level][by-field][errors]") {
    REQUIRE_THROWS_AS(from_json<Content>(nlohmann::json::array()), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// C — Top-level oneof_by_parent_field
//
// to_json works at top level (emits the alt's nested data — no
// discriminator, since that lives on the parent). from_json is statically
// disallowed: the discriminator is fundamentally not in scope at top level.
// ═════════════════════════════════════════════════════════════════════════

using OuterContent = decltype(OuterMessage{}.content);

TEST_CASE("top-level oneof_by_parent_field: to_json — emits alt data only", "[oneof][top-level][by-parent]") {
    OuterContent c = ImageContent{ .url = "https://x/cat.png", .width = 400, .height = 300 };
    auto j = to_json(c);
    REQUIRE(j["url"]    == "https://x/cat.png");
    REQUIRE(j["width"]  == 400);
    REQUIRE(j["height"] == 300);
    // The struct's own `type` field is part of the alt, so it serializes too —
    // that's the alt's own JSON. The parent's separate discriminator is what
    // top-level can't synthesize.
    REQUIRE(j["type"] == "image");
}

TEST_CASE("top-level oneof_by_parent_field: to_json_string", "[oneof][top-level][by-parent]") {
    OuterContent c = TextContent{ .text = "hi" };
    auto s = to_json_string(c);
    REQUIRE(s.find("\"text\":\"hi\"") != std::string::npos);
}

// from_json<OuterContent>(...) is intentionally rejected at compile time
// via static_assert. The expected user move is to call from_json on the
// enclosing parent struct, which can read its sibling discriminator and
// dispatch correctly.
TEST_CASE("top-level oneof_by_parent_field: parent struct round-trips", "[oneof][top-level][by-parent][round-trip]") {
    OuterMessage m{ "alice", "image",
        ImageContent{ .url = "https://x/cat.png", .width = 400, .height = 300 } };
    auto j = to_json(m);
    auto restored = from_json<OuterMessage>(j);
    REQUIRE(restored.sender == "alice");
    REQUIRE(restored.content.is<ImageContent>());
    REQUIRE(restored.content.as<ImageContent>()->url == "https://x/cat.png");
}

// ═════════════════════════════════════════════════════════════════════════
// D — ADL override path still wins for top-level
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("top-level oneof: ADL to_json wins", "[oneof][top-level][override]") {
    top_level_oneof_adl_t::Fruit f = top_level_oneof_adl_t::Apple{ 250 };
    auto j = to_json(f);
    REQUIRE(j["fruit_type"]   == "apple");
    REQUIRE(j["weight_grams"] == 250);
}

TEST_CASE("top-level oneof: ADL from_json wins", "[oneof][top-level][override]") {
    nlohmann::json j = { {"fruit_type","pear"}, {"ripeness", 7} };
    auto f = from_json<top_level_oneof_adl_t::Fruit>(j);
    REQUIRE(f.is<top_level_oneof_adl_t::Pear>());
    REQUIRE(f.as<top_level_oneof_adl_t::Pear>()->ripeness == 7);
}

TEST_CASE("top-level oneof: ADL round-trip via top-level API", "[oneof][top-level][override][round-trip]") {
    top_level_oneof_adl_t::Fruit original = top_level_oneof_adl_t::Pear{ 9 };
    auto j = to_json(original);
    auto restored = from_json<top_level_oneof_adl_t::Fruit>(j);
    REQUIRE(restored.is<top_level_oneof_adl_t::Pear>());
    REQUIRE(restored.as<top_level_oneof_adl_t::Pear>()->ripeness == 9);
}

// ═════════════════════════════════════════════════════════════════════════
// E — Codegen-style usage: the use case from FEEDBACK.md
//
// "Wanting to write 'this exact JSON parses to the right oneof variant;
// this oneof serializes to this exact JSON' is a normal test shape."
// Without top-level oneof support, you'd have to wrap in a carrier struct.
// With it, the test reads exactly like the assertion you want to make.
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("codegen: variant-only round-trip without carrier struct", "[oneof][top-level][codegen]") {
    // Exact JSON the protocol says to emit:
    auto expected = nlohmann::json{
        {"type",   "image"},
        {"url",    "https://example.com/cat.png"},
        {"width",  400},
        {"height", 300}
    };

    Content c = ImageContent{ .url = "https://example.com/cat.png", .width = 400, .height = 300 };

    REQUIRE(to_json(c) == expected);
    REQUIRE(from_json<Content>(expected).is<ImageContent>());
}

TEST_CASE("codegen: shape-only variant — exact JSON shape", "[oneof][top-level][codegen]") {
    auto expected = nlohmann::json{
        {"uri",      "file:///hello.txt"},
        {"mimeType", "text/plain"},
        {"text",     "hello"}
    };

    Resource r = TextResource{ "file:///hello.txt", "text/plain", "hello" };

    REQUIRE(to_json(r) == expected);
    auto restored = from_json<Resource>(expected);
    REQUIRE(restored.is<TextResource>());
    REQUIRE(restored.as<TextResource>()->text == "hello");
}
