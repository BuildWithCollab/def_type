# Tests for `def_type::oneof<...>`

Comprehensive test catalog. Pair with `ONEOF.md` (public contract) and `ONEOF_BUILD.md` (implementation guide). Use Catch2, matching the style of the existing `tests/test_*.cpp` files.

## File layout

| File | Scope |
|---|---|
| `tests/test_oneof.cpp` | Class behavior — construction, `is<>`, `as<>`, `match`, copy/move. No JSON. |
| `tests/test_oneof_json.cpp` | JSON shapes for each form (shape-only, inside-object, outside-object). |
| `tests/test_oneof_composition.cpp` | `oneof` inside `field<>`, containers, `optional`, nested `oneof`. |

Use `test_model_types.hpp` style for fixture types if you add many. Keep the alt structs neutral (Dog/Cat, TextContent/ImageContent style) — do not import use-case vocabulary.

---

# A. Class behavior — `test_oneof.cpp`

No JSON in this file. Pure value-type behavior.

## A.1 — Construction

| # | Case |
|---|---|
| A1.1 | `oneof<Dog, Cat> p = Dog{...};` — active alt is `Dog`. |
| A1.2 | `oneof<Dog, Cat> p = Cat{...};` — active alt is `Cat`. |
| A1.3 | `oneof<Dog> p = Dog{...};` — single-alt `oneof` constructs and behaves. |
| A1.4 | `oneof<Dog, Cat> p;` — does **not** compile. Verify via SFINAE probe / `static_assert(!std::is_default_constructible_v<oneof<Dog,Cat>>)`. |
| A1.5 | `oneof<Dog, Cat> p = std::string{};` — does **not** compile (std::string is not an alt). Verify via SFINAE probe. |
| A1.6 | Brace init: `oneof<Dog, Cat> p{Dog{...}};` — works. |
| A1.7 | Move-construct from rvalue alt: `oneof<Dog, Cat> p = Dog{};` — alt's move ctor is invoked (verify via instrumented Dog if needed; otherwise just confirm the program compiles and runs). |

## A.2 — `is<T>()`

| # | Case |
|---|---|
| A2.1 | `p = Dog{}; p.is<Dog>()` → `true`. |
| A2.2 | `p = Dog{}; p.is<Cat>()` → `false`. |
| A2.3 | `p = Cat{}; p.is<Dog>()` → `false`. |
| A2.4 | `p = Cat{}; p.is<Cat>()` → `true`. |
| A2.5 | `p.is<Bird>()` where Bird is not an alt — compile error. SFINAE probe. |
| A2.6 | `is<>` on `const` instance returns the same value. |

## A.3 — `as<T>()`

| # | Case |
|---|---|
| A3.1 | `p = Dog{...}; auto* d = p.as<Dog>();` — non-null, fields match. |
| A3.2 | `p = Dog{}; p.as<Cat>()` → `nullptr`. |
| A3.3 | Const overload: `const auto& cp = p; cp.as<Dog>()` → `const Dog*` (verify via `static_assert` on type). |
| A3.4 | Mutate via `as<>`: `*p.as<Dog>() = Dog{...};` — value updates. |
| A3.5 | `as<UnrelatedType>()` — compile error. SFINAE probe. |
| A3.6 | After reassignment `p = Cat{}`, `p.as<Dog>()` → `nullptr`, `p.as<Cat>()` → non-null. |

## A.4 — `match(...)`

| # | Case |
|---|---|
| A4.1 | All alts covered, void-returning arms — body of correct arm runs. |
| A4.2 | All alts covered, value-returning arms — return value flows out. |
| A4.3 | Mutating arms — modifications persist on the variant. |
| A4.4 | Const variant + const arms — read-only access compiles and runs. |
| A4.5 | Generic catch-all `[](const auto&)` — matches the alt that has no specific overload. |
| A4.6 | Specific overload + catch-all — specific wins for its alt; catch-all takes the rest. |
| A4.7 | Missing an alt without a catch-all — compile error. SFINAE probe or compile-fail test. |
| A4.8 | Single-alt `oneof<Dog>` — `match` with one arm works. |
| A4.9 | Match arms returning a common base type (e.g. `std::string`) compose to a single return. |
| A4.10 | Match returning by reference — verify return-type deduction handles this. |

## A.5 — Copy / move

| # | Case |
|---|---|
| A5.1 | Copy ctor: `auto p2 = p;` — `p2` has same active alt and same value. |
| A5.2 | Move ctor: `auto p2 = std::move(p);` — `p2` has the value; `p` is moved-from but still valid. |
| A5.3 | Copy assignment: `p2 = p;` — same as copy ctor. |
| A5.4 | Move assignment: `p2 = std::move(p);` — same as move ctor. |
| A5.5 | Self-assignment: `p = p;` — no UB, value preserved. |
| A5.6 | Reassigning to a different active alt: `p = Cat{};` — old alt destroyed, new alt active. |

---

# B. JSON — `test_oneof_json.cpp`

Each form gets its own block. Reuse fixture types defined at the top of the file.

## B.1 — Shape-only

Fixtures:
```cpp
struct TextResource { std::string uri; std::string mimeType; std::string text; };
struct BlobResource { std::string uri; std::string mimeType; std::string blob; };
struct LinkResource { std::string uri; std::string mimeType; std::string href; };

using Resource = def_type::oneof<TextResource, BlobResource, LinkResource>;

struct Envelope {
    std::string sender;
    Resource    payload;
};
```

### B.1.a — Successful fitting

| # | Case |
|---|---|
| B1.a.1 | JSON `{uri, mimeType, text}` → fits Text. |
| B1.a.2 | JSON `{uri, mimeType, blob}` → fits Blob. |
| B1.a.3 | JSON `{uri, mimeType, href}` → fits Link. |
| B1.a.4 | Subset: JSON `{text}` only → fits Text (other fields default). |
| B1.a.5 | Single-alt `oneof<TextResource>` with `{}` → fits Text with all defaults. |
| B1.a.6 | Alt with nested struct field → JSON has the nested object → fits. |
| B1.a.7 | Alt with `optional<T>` field → JSON omits it → fits, optional empty. |
| B1.a.8 | Alt with `optional<T>` field → JSON is `null` → fits, optional empty. |
| B1.a.9 | Alt with `vector<int>` field → JSON has array → fits. |
| B1.a.10 | Alt with enum field → JSON has string → fits. |

### B.1.b — Round-trip

| # | Case |
|---|---|
| B1.b.1 | `to_json` → `from_json` → equal value, same active alt. |
| B1.b.2 | Round-trip with all-primitives alt. |
| B1.b.3 | Round-trip with nested-struct alt. |
| B1.b.4 | Round-trip with container alt. |
| B1.b.5 | `to_json_string` compact + indented forms. |

### B.1.c — Errors

| # | Case |
|---|---|
| B1.c.1 | JSON has key `"weird"` not in any alt → `std::logic_error`. |
| B1.c.2 | Error message names the bad key. |
| B1.c.3 | Error message names every alternative. |
| B1.c.4 | Two alts identical field set; any non-empty JSON → multi-fit. |
| B1.c.5 | `oneof<A, B>` where `A.fields ⊂ B.fields`, JSON only has A's keys → multi-fit. |
| B1.c.6 | Three alts, two fit, one does not → error names exactly the two. |
| B1.c.7 | Empty JSON `{}` against multi-alt `oneof` → multi-fit. |
| B1.c.8 | Top-level JSON is array → "expected JSON object". |
| B1.c.9 | Top-level JSON is string → throws. |
| B1.c.10 | Top-level JSON is `null` → throws. |
| B1.c.11 | Type mismatch inside a fitted alt (`int` field, JSON has `"abc"`) → propagated `value_from_json` error. |

## B.2 — Inside-object

Fixtures:
```cpp
struct TextContent  { std::string text; };
struct ImageContent { std::string url; int width; int height; };
struct AudioContent { std::string url; int duration_seconds; };

using Content = def_type::oneof<"type",
    TextContent,  "text",
    ImageContent, "image",
    AudioContent, "audio"
>;

struct Message {
    std::string sender;
    Content     content;
};
```

### B.2.a — Serialize

| # | Case |
|---|---|
| B2.a.1 | Active TextContent → JSON `{"type":"text","text":"..."}`. |
| B2.a.2 | Active ImageContent → JSON `{"type":"image","url":"...","width":...,"height":...}`. |
| B2.a.3 | Active AudioContent → JSON `{"type":"audio","url":"...","duration_seconds":...}`. |
| B2.a.4 | Embedded in `Message`: outer JSON has both `"sender"` and the nested `"content"` with `"type"` inside. |
| B2.a.5 | The discriminator key `"type"` appears as the **first** or otherwise stable position in the emitted JSON object (document expected ordering or note it is unspecified). |

### B.2.b — Deserialize

| # | Case |
|---|---|
| B2.b.1 | JSON `{"type":"image","url":"...","width":...,"height":...}` → `is<ImageContent>()`. |
| B2.b.2 | JSON `{"type":"text","text":"hello"}` → `is<TextContent>()`. |
| B2.b.3 | Inside `Message`, full nested JSON deserializes correctly. |
| B2.b.4 | Discriminator value with extra unknown JSON keys present → still picks the right alt; unknown keys ignored (existing reflected-struct behavior). |
| B2.b.5 | JSON has `"type"` and only some of the alt's data fields → alt is chosen, missing fields default. |

### B.2.c — Round-trip

| # | Case |
|---|---|
| B2.c.1 | Each alt round-trips through `to_json` → `from_json` → equal. |
| B2.c.2 | Round-trip preserves field values across all alt-specific fields. |

### B.2.d — Errors

| # | Case |
|---|---|
| B2.d.1 | JSON missing `"type"` → `std::logic_error` (key required). |
| B2.d.2 | JSON `"type": "unknown"` → `std::logic_error` listing known labels (`text`, `image`, `audio`). |
| B2.d.3 | JSON `"type"` is not a string (e.g. number) → propagated type error. |
| B2.d.4 | Top-level JSON for the variant is not an object → "expected JSON object". |
| B2.d.5 | Wrong-typed field on the chosen alt (e.g. `width` is a string) → propagated. |

### B.2.e — Compile-time

| # | Case |
|---|---|
| B2.e.1 | Duplicate label across alts (e.g. two alts labeled `"text"`) → static_assert. SFINAE probe via separate file or `static_assert` test. |
| B2.e.2 | Missing label for one alt (alternation has odd number of args) → static_assert. |
| B2.e.3 | Alt that is not a reflected struct (and no ADL override) → static_assert. |

## B.3 — Outside-object

Fixtures:
```cpp
struct TextContent  { std::string text; };
struct ImageContent { std::string url; int width; int height; };
struct AudioContent { std::string url; int duration_seconds; };

struct Message {
    std::string sender;
    std::string content_type;

    def_type::oneof<&Message::content_type,
        TextContent,  "text",
        ImageContent, "image",
        AudioContent, "audio"
    > content;
};
```

### B.3.a — Serialize

| # | Case |
|---|---|
| B3.a.1 | `Message{ .content = ImageContent{...} }` → JSON `content_type` is `"image"`, `content` is the nested image object. |
| B3.a.2 | Same for `TextContent` and `AudioContent`. |
| B3.a.3 | `m.content_type` written by user is **overwritten** by the active alt's label on serialize (or matches Strategy C from `ONEOF_BUILD.md` — pick one and test the actual chosen behavior). |
| B3.a.4 | The variant's nested JSON object does **not** contain a `"type"` key — the discriminator lives only on the parent. |

### B.3.b — Deserialize

| # | Case |
|---|---|
| B3.b.1 | JSON `{"sender":"...","content_type":"image","content":{...}}` → `m.content.is<ImageContent>()`. |
| B3.b.2 | Same for text and audio. |
| B3.b.3 | `m.content_type` after deserialize equals the JSON's `content_type` value. |
| B3.b.4 | Alt's nested object fields are populated correctly. |

### B.3.c — Round-trip

| # | Case |
|---|---|
| B3.c.1 | Each alt round-trips correctly. |
| B3.c.2 | After round-trip, `content_type` and the active alt's label match. |

### B.3.d — Errors

| # | Case |
|---|---|
| B3.d.1 | JSON missing `content_type` → `std::logic_error`. |
| B3.d.2 | JSON missing `content` → `std::logic_error`. |
| B3.d.3 | `content_type` value doesn't match any label → `std::logic_error` listing known labels. |
| B3.d.4 | `content_type` is not a string → propagated error. |
| B3.d.5 | `content` JSON has a wrong-typed field for the chosen alt → propagated. |

### B.3.e — Compile-time

| # | Case |
|---|---|
| B3.e.1 | Variant field declared **before** `content_type` → static_assert. |
| B3.e.2 | Member pointer aims at a non-string field → static_assert. |
| B3.e.3 | Member pointer's class is not the enclosing type → static_assert. |
| B3.e.4 | Duplicate label across alts → static_assert. |
| B3.e.5 | Alt that is not a reflected struct (and no ADL override) → static_assert. |

---

# C. Override path — `test_oneof_json.cpp` (continued)

User-supplied ADL `from_json` / `to_json` always wins. Use a separate fixture so the override doesn't bleed into other tests.

| # | Case |
|---|---|
| C.1 | Variant with user `from_json` → user function runs, default fitting algorithm does not. |
| C.2 | Variant with user `to_json` → user function runs, default emitter does not. |
| C.3 | Override applied even when default would have succeeded — verify the override's distinguishing behavior is observed (e.g. produces a JSON shape the default could not). |
| C.4 | Override throws `std::runtime_error("...")` for unknown discriminator → exception escapes correctly. |
| C.5 | Override-only `from_json` (no `to_json`) — document expected behavior; test that asymmetric overrides either work or fail consistently with the chosen design. |
| C.6 | A variant whose alts are **not** reflected structs (e.g. `oneof<int, std::string>`) compiles only because an override is present. |
| C.7 | Variant with override + nested inside another struct → outer struct's serialization invokes the override on the variant field. |

---

# D. Composition — `test_oneof_composition.cpp`

The variant participates in the existing JSON cascade. These tests confirm.

## D.1 — Inside `field<>`

| # | Case |
|---|---|
| D1.1 | `field<oneof<Dog, Cat>>` typed-path member — to_json, from_json, round-trip. |
| D1.2 | `field<oneof<...>>` with `validators(...)` — the validator runs against the variant value (or against a probe — design choice; document expected). |
| D1.3 | `field<oneof<...>, with<some_meta>>` — meta is queryable via `field("name").meta<some_meta>()`. |

## D.2 — Inside containers

| # | Case |
|---|---|
| D2.1 | `std::vector<oneof<Dog, Cat>>` — JSON array of mixed objects. Round-trip. |
| D2.2 | `std::map<std::string, oneof<Dog, Cat>>` — JSON object of mixed values. Round-trip. |
| D2.3 | `std::unordered_set<...>` of a hashable variant alt — only if alts are hashable; otherwise omit. |
| D2.4 | Vector of variants with mixed alts — `[{"name":"Rex","age":3}, {"name":"Whiskers","indoor":true}]`. |

## D.3 — Inside `optional`

| # | Case |
|---|---|
| D3.1 | `std::optional<oneof<Dog, Cat>>` — `std::nullopt` → JSON `null`. |
| D3.2 | `std::optional<oneof<Dog, Cat>>` with value → JSON of the active alt. |
| D3.3 | Round-trip of both. |

## D.4 — Nested variants

| # | Case |
|---|---|
| D4.1 | `oneof<Dog, oneof<Cat, Bird>>` — outer variant fits Dog; inner variant fits Cat. Round-trip both. |
| D4.2 | A struct-alt that itself contains a `field<oneof<...>>`. Round-trip. |
| D4.3 | 3-level nesting — variant → struct → variant → struct → variant. Round-trip. |

## D.5 — Plain struct member (PFR path)

| # | Case |
|---|---|
| D5.1 | A plain struct (no `field<>` wrappers) with a `oneof<...>` member — to_json, from_json. |

## D.6 — Dynamic path (`type_def("Name").field<oneof<...>>(...)`)

| # | Case |
|---|---|
| D6.1 | Define a dynamic type with a `oneof<...>` field. Create an instance, set/get the variant value, round-trip JSON. |
| D6.2 | `obj.load_json` overlays a variant field correctly. |

## D.7 — Mixed forms in one parent

| # | Case |
|---|---|
| D7.1 | A parent struct containing a shape-only `oneof`, an inside-object `oneof`, and an outside-object `oneof` simultaneously. All three serialize and deserialize correctly. |

---

# E. Parse path — `test_oneof_json.cpp` (continued)

`def_type::parse` is the JSON-with-reporting path. `oneof` participates by virtue of running through `value_from_json`.

| # | Case |
|---|---|
| E.1 | `parse` on a struct containing a shape-only `oneof` → `parse_result` reports validation errors and missing/extra keys correctly, with dotted paths into the variant. |
| E.2 | `parse_options{.strict = true}` with a variant that fails fitting → `parse_error` raised with correct details. |
| E.3 | `parse` on inside-object form: discriminator value mismatch → reflected as a parse error, not a raw `std::logic_error`. (Verify the existing parse path's error capture works for this case; if not, document the gap.) |
| E.4 | `parse` on outside-object form: discriminator value mismatch reported. |

---

# F. Edge cases — anywhere appropriate

| # | Case |
|---|---|
| F.1 | `oneof<TextResource, BlobResource>` where both alt structs have identical field sets → multi-fit on any non-empty JSON → matches B1.c.4 but worth a dedicated case for clarity. |
| F.2 | A variant with three or more alts where the multi-fit error message uses an Oxford-list-style formatter ("A, B, or C") — document the chosen formatting and assert against it. |
| F.3 | Single-alternative variant: `oneof<Dog>` — every operation works without any branching corner cases. Empty JSON `{}` deserializes to a default-constructed Dog, no multi-fit error. |
| F.4 | Variant with a long alt list (10+ alts) — sanity check that the compile-time machinery scales. |
| F.5 | Variant alt that is itself a templated type — e.g. `oneof<MyStruct<int>, MyStruct<std::string>>`. |
| F.6 | Variant whose alts are in different namespaces — verify `nameof` is **not** consulted (we don't auto-derive from struct names) and labels in the alternation are what's used. |

---

# G. Negative tests for documentation guarantees

These confirm specific guarantees made in `ONEOF.md` are real. Use SFINAE probes or `static_assert` blocks in dedicated `static_assert`-only test files where compile-error tests are needed.

| # | Case |
|---|---|
| G.1 | `oneof<Dog, Cat> p;` does not compile (no default ctor). |
| G.2 | `p.is<Bird>()` does not compile when `Bird` is not an alt. |
| G.3 | `p.as<Bird>()` does not compile when `Bird` is not an alt. |
| G.4 | Constructor from non-alt does not compile. |
| G.5 | `oneof<int>` with no override does not compile (alts must be reflected structs). |
| G.6 | Outside-object: variant declared before discriminator field does not compile. |
| G.7 | Outside-object: member pointer to non-string field does not compile. |
| G.8 | `match` missing an alt without a catch-all does not compile. |

---

# Test file headers — minimal example

Match the existing style:

```cpp
#include <catch2/catch_all.hpp>
#include <def_type.hpp>

using namespace def_type;

struct TextContent  { std::string text; };
struct ImageContent { std::string url; int width; int height; };

TEST_CASE("oneof: inside-object — image alt round-trips", "[oneof][inside-object]") {
    using Content = def_type::oneof<"type",
        TextContent,  "text",
        ImageContent, "image">;

    Content c = ImageContent{ .url = "https://x/cat.png", .width = 400, .height = 300 };
    auto j = def_type::to_json(c);

    REQUIRE(j["type"]   == "image");
    REQUIRE(j["url"]    == "https://x/cat.png");
    REQUIRE(j["width"]  == 400);
    REQUIRE(j["height"] == 300);

    auto restored = def_type::from_json<Content>(j);
    REQUIRE(restored.is<ImageContent>());
    REQUIRE(restored.as<ImageContent>()->url == "https://x/cat.png");
}
```

Use Catch2 tags consistently: `[oneof]` plus one of `[shape-only]`, `[inside-object]`, `[outside-object]`, `[override]`, `[composition]`, `[parse]`, `[errors]`, `[static_assert]`.

---

# What this catalog does not cover

- Performance / benchmark tests. Not required for correctness.
- Multi-threaded use of `oneof` values. Not in scope; `oneof` carries the same thread-safety story as `std::variant`.
- ABI stability tests. Not in scope.

If `ONEOF.md` is updated to specify behavior in any of the above areas, this catalog must be extended to match.
