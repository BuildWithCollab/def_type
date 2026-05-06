#include <catch2/catch_all.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <def_type.hpp>

using namespace def_type;

// ── Fixture types ─────────────────────────────────────────────────────────

namespace oneof_t {
    struct Dog  { std::string name; int age; };
    struct Cat  { std::string name; bool indoor; };
    struct Bird { std::string name; double wingspan; };

    struct TextResource { std::string uri; std::string mimeType; std::string text; };
    struct BlobResource { std::string uri; std::string mimeType; std::string blob; };
    struct LinkResource { std::string uri; std::string mimeType; std::string href; };

    struct TextContent  { std::string text; };
    struct ImageContent { std::string url; int width; int height; };
    struct AudioContent { std::string url; int duration_seconds; };

    using Resource = oneof<TextResource, BlobResource, LinkResource>;

    struct Envelope {
        std::string sender;
        Resource    payload = TextResource{};
    };

    using Content = oneof_by_field<"type",
        oneof_type<TextContent,  "text">,
        oneof_type<ImageContent, "image">,
        oneof_type<AudioContent, "audio">>;

    struct Message {
        std::string sender;
        Content     content = TextContent{};
    };

    struct OuterMessage {
        std::string sender;
        std::string content_type;

        oneof_by_parent_field<&OuterMessage::content_type,
            oneof_type<TextContent,  "text">,
            oneof_type<ImageContent, "image">,
            oneof_type<AudioContent, "audio">
        > content = TextContent{};
    };

    using Pet = oneof<Dog, Cat>;
    struct Shelter { std::vector<Pet> pets; };
    struct Box     { std::optional<Pet> maybe_pet; };
    struct Registry{ std::map<std::string, Pet> by_name; };
}

using namespace oneof_t;

// ═════════════════════════════════════════════════════════════════════════
// A — Class behavior (no JSON)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("oneof: construction from alt", "[oneof][class]") {
    oneof<Dog, Cat> p = Dog{ "Rex", 3 };
    REQUIRE((p.is<Dog>()));
    REQUIRE_FALSE((p.is<Cat>()));
    REQUIRE((p.as<Dog>() != nullptr));
    REQUIRE((p.as<Dog>()->name == "Rex"));
    REQUIRE((p.as<Dog>()->age == 3));
    REQUIRE((p.as<Cat>() == nullptr));
}

TEST_CASE("oneof: construction from second alt", "[oneof][class]") {
    oneof<Dog, Cat> p = Cat{ "Whiskers", true };
    REQUIRE((p.is<Cat>()));
    REQUIRE_FALSE((p.is<Dog>()));
    REQUIRE((p.as<Cat>()->indoor == true));
}

TEST_CASE("oneof: single-alt", "[oneof][class]") {
    oneof<Dog> p = Dog{ "Solo", 1 };
    REQUIRE((p.is<Dog>()));
    REQUIRE((p.as<Dog>()->name == "Solo"));
}

TEST_CASE("oneof: no default constructor", "[oneof][class][static_assert]") {
    static_assert(!std::is_default_constructible_v<oneof<Dog, Cat>>);
    static_assert(!std::is_default_constructible_v<Content>);
    static_assert(!std::is_default_constructible_v<decltype(OuterMessage{}.content)>);
}

TEST_CASE("oneof: not constructible from non-alt", "[oneof][class][static_assert]") {
    static_assert(!std::is_constructible_v<oneof<Dog, Cat>, std::string>);
    static_assert(!std::is_constructible_v<oneof<Dog, Cat>, int>);
    static_assert(!std::is_constructible_v<oneof<Dog, Cat>, Bird>);
}

TEST_CASE("oneof: as<T> mutate in place", "[oneof][class]") {
    oneof<Dog, Cat> p = Dog{ "Rex", 3 };
    p.as<Dog>()->age = 7;
    REQUIRE((p.as<Dog>()->age == 7));
}

TEST_CASE("oneof: as<T> const overload yields const T*", "[oneof][class]") {
    oneof<Dog, Cat> p = Dog{ "Rex", 3 };
    const auto& cp = p;
    static_assert(std::is_same_v<decltype(cp.as<Dog>()), const Dog*>);
    REQUIRE((cp.as<Dog>()->name == "Rex"));
}

TEST_CASE("oneof: reassign to different alt", "[oneof][class]") {
    oneof<Dog, Cat> p = Dog{ "Rex", 3 };
    REQUIRE((p.is<Dog>()));
    p = Cat{ "Whiskers", false };
    REQUIRE((p.is<Cat>()));
    REQUIRE_FALSE((p.is<Dog>()));
    REQUIRE((p.as<Cat>()->indoor == false));
}

TEST_CASE("oneof: copy and move", "[oneof][class]") {
    oneof<Dog, Cat> p = Dog{ "Rex", 3 };
    auto q = p;                        // copy
    REQUIRE((q.is<Dog>()));
    REQUIRE((q.as<Dog>()->name == "Rex"));

    auto r = std::move(q);             // move
    REQUIRE((r.is<Dog>()));

    oneof<Dog, Cat> s = Cat{ "C", true };
    s = p;                             // copy assign
    REQUIRE((s.is<Dog>()));

    s = std::move(p);                  // move assign
    REQUIRE((s.is<Dog>()));
}

TEST_CASE("oneof: match — void arms", "[oneof][class][match]") {
    oneof<Dog, Cat> p = Cat{ "Whiskers", true };
    bool dog_arm = false, cat_arm = false;
    p.match(
        [&](const Dog&) { dog_arm = true; },
        [&](const Cat&) { cat_arm = true; }
    );
    REQUIRE_FALSE(dog_arm);
    REQUIRE(cat_arm);
}

TEST_CASE("oneof: match — value-returning arms", "[oneof][class][match]") {
    oneof<Dog, Cat> p = Dog{ "Rex", 3 };
    auto label = p.match(
        [](const Dog&) { return std::string("dog"); },
        [](const Cat&) { return std::string("cat"); }
    );
    REQUIRE(label == "dog");
}

TEST_CASE("oneof: match — mutating arms persist", "[oneof][class][match]") {
    oneof<Dog, Cat> p = Dog{ "Rex", 3 };
    p.match(
        [](Dog& d) { d.age = 99; },
        [](Cat&)   {}
    );
    REQUIRE((p.as<Dog>()->age == 99));
}

TEST_CASE("oneof: match — generic catch-all", "[oneof][class][match]") {
    oneof<Dog, Cat, Bird> p = Bird{ "Tweety", 0.5 };
    auto label = p.match(
        [](const Dog&)   { return std::string("dog"); },
        [](const auto&)  { return std::string("other"); }
    );
    REQUIRE(label == "other");
}

// ═════════════════════════════════════════════════════════════════════════
// B.1 — Shape-only JSON (primary form)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("shape-only: serialize text", "[oneof][shape-only]") {
    Resource r = TextResource{ "file:///x.py", "text/x-python", "print('hi')" };
    auto j = detail::value_to_json(r);
    REQUIRE(j["uri"]      == "file:///x.py");
    REQUIRE(j["mimeType"] == "text/x-python");
    REQUIRE(j["text"]     == "print('hi')");
}

TEST_CASE("shape-only: deserialize text", "[oneof][shape-only]") {
    nlohmann::json j = {
        { "uri",      "file:///x.py" },
        { "mimeType", "text/x-python" },
        { "text",     "print('hi')" }
    };
    Resource r = TextResource{};
    detail::value_from_json(j, r);
    REQUIRE((r.is<TextResource>()));
    REQUIRE((r.as<TextResource>()->text == "print('hi')"));
}

TEST_CASE("shape-only: deserialize blob", "[oneof][shape-only]") {
    nlohmann::json j = {
        { "uri",      "file:///x.bin" },
        { "mimeType", "application/octet-stream" },
        { "blob",     "deadbeef" }
    };
    Resource r = TextResource{};
    detail::value_from_json(j, r);
    REQUIRE((r.is<BlobResource>()));
    REQUIRE((r.as<BlobResource>()->blob == "deadbeef"));
}

TEST_CASE("shape-only: round-trip via parent struct", "[oneof][shape-only]") {
    Envelope e{ "alice", TextResource{ "u", "m", "hello" } };
    auto j = to_json(e);
    REQUIRE(j["sender"] == "alice");
    REQUIRE(j["payload"]["text"] == "hello");

    auto restored = from_json<Envelope>(j);
    REQUIRE(restored.sender == "alice");
    REQUIRE((restored.payload.is<TextResource>()));
    REQUIRE((restored.payload.as<TextResource>()->text == "hello"));
}

TEST_CASE("shape-only: error on unknown key", "[oneof][shape-only][errors]") {
    nlohmann::json j = { { "uri", "u" }, { "weird", "x" } };
    Resource r = TextResource{};
    REQUIRE_THROWS_AS(detail::value_from_json(j, r), std::logic_error);
}

TEST_CASE("shape-only: error on multi-fit empty object", "[oneof][shape-only][errors]") {
    nlohmann::json j = nlohmann::json::object();
    Resource r = TextResource{};
    REQUIRE_THROWS_AS(detail::value_from_json(j, r), std::logic_error);
}

TEST_CASE("shape-only: subset matches single alt", "[oneof][shape-only]") {
    nlohmann::json j = { { "text", "hi" } };
    Resource r = BlobResource{};
    detail::value_from_json(j, r);
    REQUIRE((r.is<TextResource>()));
    REQUIRE((r.as<TextResource>()->text == "hi"));
}

// ═════════════════════════════════════════════════════════════════════════
// B.2 — Inside-object: oneof_by_field
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("oneof_by_field: serialize image", "[oneof][by-field]") {
    Content c = ImageContent{ "https://x/cat.png", 400, 300 };
    auto j = detail::value_to_json(c);
    REQUIRE(j["type"]   == "image");
    REQUIRE(j["url"]    == "https://x/cat.png");
    REQUIRE(j["width"]  == 400);
    REQUIRE(j["height"] == 300);
}

TEST_CASE("oneof_by_field: deserialize image", "[oneof][by-field]") {
    nlohmann::json j = {
        { "type", "image" }, { "url", "u" }, { "width", 1 }, { "height", 2 }
    };
    Content c = TextContent{};
    detail::value_from_json(j, c);
    REQUIRE((c.is<ImageContent>()));
    REQUIRE((c.as<ImageContent>()->url == "u"));
}

TEST_CASE("oneof_by_field: round-trip in parent", "[oneof][by-field]") {
    Message m{ "bob", ImageContent{ "u", 1, 2 } };
    auto j = to_json(m);
    REQUIRE(j["sender"] == "bob");
    REQUIRE(j["content"]["type"] == "image");

    auto restored = from_json<Message>(j);
    REQUIRE(restored.sender == "bob");
    REQUIRE((restored.content.is<ImageContent>()));
    REQUIRE((restored.content.as<ImageContent>()->url == "u"));
}

TEST_CASE("oneof_by_field: missing discriminator", "[oneof][by-field][errors]") {
    nlohmann::json j = { { "url", "x" }, { "width", 1 }, { "height", 1 } };
    Content c = TextContent{};
    REQUIRE_THROWS_AS(detail::value_from_json(j, c), std::logic_error);
}

TEST_CASE("oneof_by_field: unknown discriminator", "[oneof][by-field][errors]") {
    nlohmann::json j = { { "type", "video" } };
    Content c = TextContent{};
    REQUIRE_THROWS_AS(detail::value_from_json(j, c), std::logic_error);
}

TEST_CASE("oneof_by_field: round-trip text alt", "[oneof][by-field]") {
    Content c = TextContent{ "hello" };
    auto j = detail::value_to_json(c);
    REQUIRE(j["type"] == "text");
    REQUIRE(j["text"] == "hello");

    Content back = AudioContent{};
    detail::value_from_json(j, back);
    REQUIRE((back.is<TextContent>()));
    REQUIRE((back.as<TextContent>()->text == "hello"));
}

// ═════════════════════════════════════════════════════════════════════════
// B.3 — Outside-object: oneof_by_parent_field
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("oneof_by_parent_field: serialize image", "[oneof][by-parent]") {
    OuterMessage m{ "alice", "stale_value", ImageContent{ "u", 100, 50 } };
    auto j = to_json(m);
    REQUIRE(j["sender"] == "alice");
    REQUIRE(j["content_type"] == "image");
    REQUIRE(j["content"]["url"] == "u");
    REQUIRE(j["content"]["width"] == 100);
    REQUIRE_FALSE(j["content"].contains("type"));
}

TEST_CASE("oneof_by_parent_field: deserialize image", "[oneof][by-parent]") {
    nlohmann::json j = {
        { "sender", "alice" },
        { "content_type", "image" },
        { "content", { { "url", "u" }, { "width", 100 }, { "height", 50 } } }
    };
    auto m = from_json<OuterMessage>(j);
    REQUIRE(m.sender == "alice");
    REQUIRE(m.content_type == "image");
    REQUIRE((m.content.is<ImageContent>()));
    REQUIRE((m.content.as<ImageContent>()->url == "u"));
}

TEST_CASE("oneof_by_parent_field: round-trip", "[oneof][by-parent]") {
    OuterMessage m{ "bob", "", AudioContent{ "song.mp3", 240 } };
    auto j = to_json(m);
    auto restored = from_json<OuterMessage>(j);
    REQUIRE(restored.sender == "bob");
    REQUIRE(restored.content_type == "audio");
    REQUIRE((restored.content.is<AudioContent>()));
    REQUIRE((restored.content.as<AudioContent>()->duration_seconds == 240));
}

TEST_CASE("oneof_by_parent_field: unknown discriminator", "[oneof][by-parent][errors]") {
    nlohmann::json j = {
        { "sender", "x" }, { "content_type", "video" },
        { "content", { { "foo", "bar" } } }
    };
    REQUIRE_THROWS_AS(from_json<OuterMessage>(j), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// C — Manual JSON via nlohmann ADL hooks (the spec's override path)
// ═════════════════════════════════════════════════════════════════════════

namespace fruit_ns {
    struct Apple { int n; };
    struct Pear  { int n; };
    using Fruit = def_type::oneof<Apple, Pear>;

    inline void to_json(nlohmann::json& j, const Fruit& f) {
        j = nlohmann::json::object();
        if (auto* a = f.as<Apple>()) { j["kind"] = "A"; j["n"] = a->n; }
        else if (auto* p = f.as<Pear>()) { j["kind"] = "P"; j["n"] = p->n; }
    }

    inline void from_json(const nlohmann::json& j, Fruit& f) {
        if (j.at("kind") == "A") f = Apple{ j.at("n").get<int>() };
        else                     f = Pear { j.at("n").get<int>() };
    }
}

TEST_CASE("override: ADL probe sees user functions", "[oneof][override]") {
    static_assert(def_type::detail::has_user_oneof_to_json<fruit_ns::Fruit>);
    static_assert(def_type::detail::has_user_oneof_from_json<fruit_ns::Fruit>);
    REQUIRE(true);
}

TEST_CASE("override: ADL to_json wins", "[oneof][override]") {
    fruit_ns::Fruit f = fruit_ns::Apple{ 7 };
    auto j = detail::value_to_json(f);
    REQUIRE(j["kind"] == "A");
    REQUIRE(j["n"] == 7);
}

TEST_CASE("override: ADL from_json wins", "[oneof][override]") {
    nlohmann::json j = { { "kind", "P" }, { "n", 11 } };
    fruit_ns::Fruit f = fruit_ns::Apple{ 0 };
    detail::value_from_json(j, f);
    REQUIRE((f.is<fruit_ns::Pear>()));
    REQUIRE((f.as<fruit_ns::Pear>()->n == 11));
}

TEST_CASE("override: round-trip via ADL", "[oneof][override]") {
    fruit_ns::Fruit original = fruit_ns::Pear{ 42 };
    auto j = detail::value_to_json(original);
    fruit_ns::Fruit restored = fruit_ns::Apple{ 0 };
    detail::value_from_json(j, restored);
    REQUIRE((restored.is<fruit_ns::Pear>()));
    REQUIRE((restored.as<fruit_ns::Pear>()->n == 42));
}

// ═════════════════════════════════════════════════════════════════════════
// D — Composition (vector / map / optional)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("composition: vector of variants", "[oneof][composition]") {
    Shelter s{};
    s.pets.push_back(Dog{ "Rex", 3 });
    s.pets.push_back(Cat{ "Whiskers", true });

    auto j = to_json(s);
    REQUIRE(j["pets"].is_array());
    REQUIRE(j["pets"].size() == 2);
    REQUIRE(j["pets"][0]["age"] == 3);
    REQUIRE(j["pets"][1]["indoor"] == true);

    auto restored = from_json<Shelter>(j);
    REQUIRE(restored.pets.size() == 2);
    REQUIRE((restored.pets[0].is<Dog>()));
    REQUIRE((restored.pets[1].is<Cat>()));
}

TEST_CASE("composition: optional<oneof>", "[oneof][composition]") {
    Box b1{};
    auto j1 = to_json(b1);
    REQUIRE(j1["maybe_pet"].is_null());

    Box b2{ Dog{ "Rex", 3 } };
    auto j2 = to_json(b2);
    REQUIRE(j2["maybe_pet"]["age"] == 3);

    auto r1 = from_json<Box>(j1);
    REQUIRE_FALSE(r1.maybe_pet.has_value());
    auto r2 = from_json<Box>(j2);
    REQUIRE(r2.maybe_pet.has_value());
    REQUIRE((r2.maybe_pet->is<Dog>()));
}

TEST_CASE("composition: map<string, oneof>", "[oneof][composition]") {
    Registry r{};
    r.by_name.emplace("rex", Dog{ "Rex", 3 });
    r.by_name.emplace("whiskers", Cat{ "Whiskers", true });

    auto j = to_json(r);
    REQUIRE(j["by_name"]["rex"]["age"] == 3);
    REQUIRE(j["by_name"]["whiskers"]["indoor"] == true);

    auto restored = from_json<Registry>(j);
    REQUIRE(restored.by_name.size() == 2);
    REQUIRE((restored.by_name.at("rex").is<Dog>()));
    REQUIRE((restored.by_name.at("whiskers").is<Cat>()));
}
