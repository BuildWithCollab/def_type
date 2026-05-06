# `def_type::oneof<...>`

A discriminated union of value types with first-class JSON support, designed to fit into def_type's existing reflection cascade. One symbol — `def_type::oneof` — covers three JSON layouts. The form is chosen by the shape of the **first** template argument:

| First template arg | Form | Discriminator location |
|---|---|---|
| (none — types only) | **Shape-only** | No discriminator; alt selected by which fields are present |
| CNTTP string | **Inside-object** | A key inside the same JSON object as the alt's fields |
| Member pointer | **Outside-object** | A sibling key on the parent struct, beside the variant field |

Following alts on the same line, every form takes inline `(Type, "label")` pairs when labels are needed. Labels never live in a reusable per-alt wrapper — they appear only on the single `oneof<...>` line.

---

## Shape-only

Use when the alts have distinct field sets and the JSON has no discriminator key.

```cpp
struct TextResource { std::string uri; std::string mimeType; std::string text; };
struct BlobResource { std::string uri; std::string mimeType; std::string blob; };

using Resource = def_type::oneof<TextResource, BlobResource>;

struct Message {
    std::string sender;
    Resource    payload;
};
```

JSON the parent struct round-trips:

```json
{
  "sender": "alice",
  "payload": {
    "uri": "file:///x.py",
    "mimeType": "text/x-python",
    "text": "print('hi')"
  }
}
```

**Algorithm.** On deserialize, the library collects the JSON object's keys and asks each alt: *"are all of these your fields?"*

- Exactly one fits → deserialize into that alt.
- Zero fit → `std::logic_error` naming the JSON key that doesn't belong to any alt.
- Multiple fit → `std::logic_error` naming the alts that tied.

The match is **name-based, not type-based** — coercion across JSON value types is never attempted. If two alts both vacuously fit (e.g. for an empty JSON object `{}`), that's a multi-fit error; the library refuses to guess.

---

## Inside-object

Use when the alts share a common JSON shape and the type is selected by a key inside the same object.

```cpp
struct TextContent  { std::string text; };
struct ImageContent { std::string url; int width; int height; };

using Content = def_type::oneof<"type",
    TextContent,  "text",
    ImageContent, "image"
>;

struct Message {
    std::string sender;
    Content     content;
};
```

JSON:

```json
{
  "sender": "alice",
  "content": {
    "type": "image",
    "url": "https://x/cat.png",
    "width": 400,
    "height": 300
  }
}
```

**Algorithm.**

- On serialize, the library injects `"type": "<label>"` into the active alt's JSON, where `<label>` is the inline string paired with the alt's type in the `oneof<...>` declaration.
- On deserialize, the library reads `"type"`, picks the alt whose label matches, and deserializes the rest of the JSON object into that alt (the `"type"` key is ignored during alt deserialization since alt structs do not declare it).

The alt structs stay pure data — they do not declare a `type` field. The discriminator is purely a JSON-shape concern, handled by the variant.

---

## Outside-object

Use when the discriminator is a key on the **parent** struct, beside (not inside) the JSON object that holds the alt's fields.

```cpp
struct TextContent  { std::string text; };
struct ImageContent { std::string url; int width; int height; };

struct Message {
    std::string sender;
    std::string content_type;

    def_type::oneof<&Message::content_type,
        TextContent,  "text",
        ImageContent, "image"
    > content;
};
```

JSON:

```json
{
  "sender": "alice",
  "content_type": "image",
  "content": {
    "url": "https://x/cat.png",
    "width": 400,
    "height": 300
  }
}
```

**Algorithm.**

- On serialize, the library writes the active alt's label into `m.content_type` so the emitted JSON is always self-consistent — whatever the user previously wrote into `content_type` is overwritten by the truth from the variant.
- On deserialize, fields are visited in declaration order. `content_type` is read first as a normal string field. When the variant field `content` is visited, the library looks up its annotation (`&Message::content_type`), reads the already-deserialized value, picks the matching alt, and deserializes the nested object into it.

**Constraint.** The discriminator field on the parent must be declared **before** the variant field. The library enforces this with a static_assert at the variant declaration; otherwise the variant's deserializer would have no value to consult.

---

## Public surface on the variant value

All three forms expose the same five operations: a converting constructor, `is<T>()`, `as<T>()`, and `match(...)`. Default construction, copy, and move are explicitly characterized below. Nothing else is part of the public API.

### Construction

```cpp
Content c = ImageContent{ .url = "https://x/cat.png", .width = 400, .height = 300 };
Content d = TextContent{ .text = "hello" };

Content x;                                      // ❌ compile error — no default ctor
Content y = std::string{"raw"};                 // ❌ compile error — std::string is not a listed alt
```

The converting constructor is the **only** way to initialize a `oneof<...>` value. The default constructor is `= delete`d. The library will not pick a "first" alternative on the user's behalf.

The constructor is constrained to the alts list — passing any other type is a compile error, not a runtime miss. `std::vector<oneof<...>>::resize(n)` and similar default-construct-many APIs will not compile against `oneof<...>`. Wrap in `std::optional<oneof<...>>` if "absent" is meaningful in the domain.

Copy and move are defaulted; both behave exactly as the underlying `std::variant<Alts...>` would.

### `is<T>() -> bool`

Returns `true` if `T` is the currently-active alternative; `false` otherwise.

```cpp
Content c = ImageContent{ /* ... */ };
c.is<ImageContent>();      // true
c.is<TextContent>();       // false
```

`T` must be one of the alts. `c.is<UnrelatedType>()` is a compile error, not a silent `false`.

### `as<T>()` / `as<T>() const`

Returns a pointer to the active alternative when `T` is the currently-active alt; returns `nullptr` otherwise. The const overload returns `const T*`.

```cpp
Content c = ImageContent{ .url = "...", .width = 400, .height = 300 };

if (auto* img = c.as<ImageContent>()) {
    img->width *= 2;             // mutate in place
}

if (auto* txt = c.as<TextContent>()) {
    // not reached — c is currently an ImageContent
}
```

`T` must be one of the alts. `c.as<UnrelatedType>()` is a compile error.

### `match(Fs&&...)` / `match(Fs&&...) const`

Visits the active alternative with whichever overload from `Fs...` accepts it. Returns whatever the chosen overload returns (the overloads must share a common return type, or all return `void`).

Both const-arm and mutating-arm forms work:

```cpp
content.match(
    [](const TextContent&  t) { /* read-only access */ },
    [](const ImageContent& i) { /* read-only access */ }
);

content.match(
    [](TextContent&  t) { t.text += "!"; },
    [](ImageContent& i) { i.width *= 2; }
);
```

A returning form:

```cpp
std::string label = content.match(
    [](const TextContent&)  { return std::string{"text"}; },
    [](const ImageContent&) { return std::string{"image"}; }
);
```

A generic catch-all is permitted and matches anything not otherwise covered:

```cpp
content.match(
    [](const ImageContent& i) { /* image-specific */ },
    [](const auto&)           { /* every other alt */ }
);
```

Note: `match` inherits `std::visit`'s exhaustiveness rule — every alternative must be matched by some overload in the set. A catch-all (`[](const auto&)`) satisfies this trivially. Without a catch-all, omitting an overload for any alt is a compile error. If a new alt is later added to a `oneof`, every `match` call site that lacks a catch-all will fail to compile until updated; sites with a catch-all silently absorb the new alt.

### What is **not** in the public API

- `std::variant` access. The internal storage is a `std::variant<Alts...>`, but no member exposes it. There is no `_underlying()`, no friend hatch, no `get<>` / `get_if<>` overloads. Use `match` to dispatch.
- Index-based access. There is no `index()`, no `valueless_by_exception()`. Alternatives are addressed by type via `is<T>` / `as<T>`, not by integer index.
- A "no active alternative" state. A `oneof<...>` value always holds one of its alts (because there is no default constructor and copy/move preserve the active alt). It cannot become valueless.
- Comparison operators. `oneof<...>` does not provide `==`, `<=>`, etc.; the user implements them if needed.
- Streaming, hashing, or `swap` beyond what defaulted copy/move imply. Not part of the public surface.

---

## Errors

| Situation | Result |
|---|---|
| Shape-only: JSON key not present on any alt | `std::logic_error` naming the bad key |
| Shape-only: JSON keys are a subset of multiple alts | `std::logic_error` naming the tied alts |
| Inside / outside: discriminator value doesn't match any label | `std::logic_error` naming the value and listing the known labels |
| Inside / outside: duplicate label across alts in the same `oneof` | `static_assert` at the variant declaration |
| Outside: variant field declared before its discriminator field | `static_assert` at the variant declaration |
| Wrong-typed JSON value for a field on the chosen alt | propagated from `value_from_json` (existing behavior) |

---

## Override path

When none of the three forms match the JSON shape your protocol uses — e.g. labels that aren't strings, or a discriminator that lives several levels above the variant — write a normal `nlohmann` ADL overload:

```cpp
void from_json(const nlohmann::json& j, Content& c);
void to_json  (nlohmann::json&       j, const Content& c);
```

If either overload is present, it **always wins** over the default form. The library detects each overload via a `requires` expression and skips its built-in path. Mixing forms is not supported — if you provide one, provide both, and own the round-trip.

---

## Composition with the rest of def_type

`oneof<...>` values compose with everything else in def_type by participating in the same `value_to_json` / `value_from_json` cascade. They can appear:

- as a plain struct member (typed path or PFR)
- inside `field<oneof<...>>` when validators or `with<>` metas are needed
- as an alternative inside another `oneof` (nested variants)
- as the value of a container — `std::vector<oneof<...>>`, `std::map<std::string, oneof<...>>`, `std::optional<oneof<...>>`, etc.

No extra opt-in is required. Everything falls out of the existing recursive descent.

---

## Out of scope

- Runtime registration of new alts. The alt list is fixed at the point `using V = def_type::oneof<...>` is written.
- A built-in default for label conventions. The library does not derive labels from C++ struct names; users state labels explicitly inside the `oneof<...>` declaration.
- Cross-struct discriminator coordination beyond the parent / sibling pattern of the outside-object form.
- Changes to any other part of def_type. `type_def`, `field<>`, `meta<>`, `validate`, and `parse` need no modifications to support `oneof<...>`.
