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

All three forms expose the same operations on the variant value:

```cpp
content.is<ImageContent>();        // bool
content.as<ImageContent>();        // ImageContent*  (nullptr if a different alt is active)
content.as<ImageContent>();        // const ImageContent* in const contexts

content.match(
    [](const TextContent&  t) { /* ... */ },
    [](const ImageContent& i) { /* ... */ }
);

// Mutating arms:
content.match(
    [](TextContent&  t) { t.text += "!"; },
    [](ImageContent& i) { i.width *= 2; }
);
```

Construction is from any listed alt by implicit conversion:

```cpp
Content c = ImageContent{ .url = "...", .width = 400, .height = 300 };
```

There is **no default constructor.** A `oneof<...>` value must be initialized with one of its alternatives — the library does not pick a "first" alt on the user's behalf. This means `oneof<...>` is not directly usable in contexts that require default construction (e.g. `std::vector::resize(n)`); wrap it in `std::optional<oneof<...>>` if "absent" is a real state in your domain.

`std::variant` is **not** part of the public surface. The internal storage is a `std::variant<Alts...>`, but the variant template never exposes it; `match` is the visit primitive.

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
