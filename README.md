# def_type

**Write a struct. Get reflection, serialization, validation, and schema queries for free.**

Just write a normal C++ struct — `type_def<T>` discovers every member automatically. Need validation or metadata on specific fields? Wrap them with `field<T>`. Everything else just works. No macros. No code generation. No registration. Just C++23.

## Table of Contents

- [The Problem](#the-problem)
- [Quick Start](#quick-start)
- [Fields](#fields)
- [Type-Level Metadata with `meta<T>`](#type-level-metadata-with-metat)
- [Querying the Schema](#querying-the-schema)
- [Working with Data](#working-with-data)
- [Iteration](#iteration)
- [JSON Serialization](#json-serialization)
- [Variants with `oneof<>`](#variants-with-oneof)
- [Type-Erased JSON with `unknown`](#type-erased-json-with-unknown)
- [Dynamic Nesting](#dynamic-nesting)
- [Validation](#validation)
- [Parse — JSON with Reporting](#parse--json-with-reporting)
- [PFR vs Manual Registration](#pfr-vs-manual-registration)
- [type_instance — Dynamic Runtime Objects](#type_instance--dynamic-runtime-objects)
- [Safety](#safety)
- [Dependencies](#dependencies)
- [Building](#building)

---

## The Problem

C++ gives you structs. Structs are great for holding data, but they don't *know* anything about themselves. Want to iterate over fields? Write it by hand. Want to serialize to JSON? Write a custom function. Want to attach metadata like "this field has a CLI short flag"? There's no standard way.

Every framework solves this differently — Unreal has `UPROPERTY()` macros, Qt has `moc`, Protocol Buffers have `.proto` files and code generation. They all work, but they all force you to leave normal C++ behind.

`def_type` takes a different approach: **your types stay as regular C++ structs**, and the framework works alongside them.

## Quick Start

```cpp
#include <def_type.hpp>
using namespace def_type;
```

### Just Write a Struct

```cpp
struct Dog {
    std::string name;
    int         age = 0;
    std::string breed;
};

// That's it. type_def sees everything.
type_def<Dog> t;

t.name();          // "Dog"
t.field_count();   // 3
t.field_names();   // ["name", "age", "breed"]

Dog rex;
rex.name = "Rex";
rex.age = 3;
rex.breed = "Husky";

// Serialize to JSON
auto j = to_json(rex);
// {"name": "Rex", "age": 3, "breed": "Husky"}

// Deserialize from JSON
auto buddy = from_json_string<Dog>(R"({"name": "Buddy", "age": 5, "breed": "Lab"})");
buddy.name;  // "Buddy"

// Set/get by name at runtime
t.set(rex, "name", std::string("Max"));
t.get<std::string>(rex, "name");   // "Max"

// Iterate all fields
t.for_each_field(rex, [](std::string_view name, auto& value) {
    // name: "name", "age", "breed"
    // value: typed reference — std::string&, int&, std::string&
});
```

No wrappers. No registration. No macros. Just a plain C++ struct and `type_def<T>{}`.

### Adding Validation and Metadata

When you need validators or field-level metadata, wrap individual fields with `field<T>`:

```cpp
struct cli_info {
    const char* help = "";
};

struct Dog {
    meta<endpoint_info> endpoint{{.path = "/dogs", .method = "POST"}};

    std::string name;                                 // plain — still works
    field<std::string, with<cli_info>>  breed {
        .with = {{.help = "Name of the dog breed"}}
    };
    field<int> age {
        .value = 0,
        .validators = validators(in_range{.min = 0, .max = 25})
    };
};

type_def<Dog> t;
t.field_count();                  // 3 — all members discovered
t.has_meta<endpoint_info>();      // true
t.meta<endpoint_info>().path;     // "/dogs"

// Field-level metadata (only on field<> members with with<>)
t.field("breed").has_meta<cli_info>();          // true
t.field("breed").meta<cli_info>().help;         // "Name of the dog breed"

// Validation (only on field<> members with validators)
Dog rex;
rex.name = "Rex";
rex.breed = "Husky";
rex.age = 3;

t.valid(rex);                     // true — age is in range [0, 25]

rex.age = -1;
t.valid(rex);                     // false

auto result = t.validate(rex);
result.errors()[0].path;          // "age"
result.errors()[0].validator;     // "in_range"
```

`field<T>` is a transparent wrapper — it *is* the value. Implicit conversion, `operator->`, aggregate initialization:

```cpp
rex.breed = "Husky";          // just works — implicit conversion
rex.age = 3;
std::string b = rex.breed;    // implicit conversion back
rex.breed->size();             // operator-> passes through to std::string
```

### When to Use `field<>`

`field<>` is **optional**. Use it when you need:

- **Validators** — `.validators = validators(not_empty{}, max_length{50})`
- **Field-level metadata** — `field<T, with<cli_meta>>` attaches domain-specific metadata
- **Both** — validators + metas on the same field

Plain members and `field<>` members coexist freely in the same struct. If you just want reflection, serialization, and schema queries — plain struct members are all you need.

### The Dynamic Path — No Struct Required

Define a type entirely at runtime:

```cpp
auto event_t = type_def("Event")
    .meta<endpoint_info>({.path = "/events"})
    .meta<help_info>({.summary = "An event"})
    .field<std::string>("title")
    .field<int>("attendees", 100)     // 100 is the default
    .field<bool>("verbose", false,
        with<cli_meta>({.cli = {.short_flag = 'v'}}));
```

Create instances and work with them:

```cpp
auto party = event_t.create();

party.get<int>("attendees");       // 100
party.get<bool>("verbose");        // false
party.get<std::string>("title");   // "" (default-constructed)

party.set("title", std::string("Dog Party"));
party.set("attendees", 50);

party.has("title");                // true
party.has("nope");                 // false

party.type().name();               // "Event"
party.type().field_count();        // 3
party.type().has_meta<endpoint_info>(); // true
```

### One Concept to Rule Them All

Both paths satisfy the `type_definition` concept. Write generic code once:

```cpp
std::string schema_summary(const type_definition auto& t) {
    std::string result = std::string(t.name()) + ": ";
    auto names = t.field_names();
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) result += ", ";
        result += names[i];
    }
    return result;
}

// Works with any path:
schema_summary(type_def<Dog>{});           // "Dog: name, breed, age"
schema_summary(type_def<PlainDog>{});      // "PlainDog: name, age, breed"
schema_summary(event_t);                    // "Event: title, attendees, verbose"
```

---

## Fields

### `field<T>` — The Transparent Value Wrapper

`field<T>` holds a value and gets out of your way:

```cpp
field<int> age;
age = 42;              // assign like a normal int
int years = age;       // read like a normal int
age.value;             // direct access when you need it
```

```cpp
field<std::string> name;
name = "Rex";
name->size();         // operator-> passes through
name->empty();
```

Structs using `field<T>` remain aggregates — designated initializers work:

```cpp
struct Weather {
    field<std::string> city;
    field<int>         days {.value = 7};       // default value
};
Weather weather{};  // city is "", days is 7
```

`field<T>` is optional — use it in the typed path when you need validators or field-level metadata. Plain struct members work too. The dynamic path stores values internally.

### Supported Field Types

`field<T>` and dynamic `.field<T>()` support any type, but JSON serialization and reflection have specific support for:

- **Primitives:** `int`, `float`, `double`, `bool`, `std::string`, `int64_t`, `uint64_t`
- **Enums:** any `enum class` — serialized by name via magic_enum (e.g. `Color::blue` → `"blue"`)
- **Containers:** `std::vector<T>`, `std::map<std::string, T>`, `std::set<T>`, `std::unordered_set<T>`, `ankerl::unordered_dense::map<std::string, T>`, `ankerl::unordered_dense::set<T>`
- **Optionals:** `std::optional<T>`
- **Nested structs:** `field<Address>` where Address is also a reflected struct
- **Nested containers:** `std::vector<std::vector<T>>`, `std::map<std::string, std::vector<T>>`, etc.
- **Variants:** `oneof<A, B, C>` — a value that holds one of several types, like `std::variant` but with JSON support. Plus `oneof_by_field<>` / `oneof_by_parent_field<>` helpers when the JSON has a discriminator field. See [Variants with `oneof<>`](#variants-with-oneof).
- **Type-erased JSON:** `unknown` — a field that holds raw JSON whose shape you don't know yet, and lets you probe it with `.is<T>()` / `.as<T>()` / `.try_as<T>()` (top-level or by path) when you do. See [Type-Erased JSON with `unknown`](#type-erased-json-with-unknown).

All of the above compose — `field<std::vector<std::optional<Address>>>` just works.

### Field-Level Metadata with `with<>`

Attach domain-specific metadata directly to fields. The metadata types are *yours* — plain structs:

```cpp
struct cli_meta {
    struct { char short_flag = '\0'; } cli;
};

struct render_meta {
    struct { const char* style = ""; int width = 0; } render;
};
```

**Typed path** — metadata baked into the field type:
```cpp
struct CliArgs {
    field<std::string> query;
    field<bool, with<cli_meta>> verbose {
        .with = {{.cli = {.short_flag = 'v'}}}
    };
    field<int, with<cli_meta, render_meta>> limit {
        .with = {{.cli = {.short_flag = 'l'}},
                 {.render = {.style = "bold", .width = 10}}}
    };
};

type_def<CliArgs>{}.field("verbose").has_meta<cli_meta>();              // true
type_def<CliArgs>{}.field("verbose").meta<cli_meta>().cli.short_flag;   // 'v'
type_def<CliArgs>{}.field("limit").meta_count<cli_meta>();              // 1
```

**Dynamic path** — same `with<M>({...})` syntax:
```cpp
auto t = type_def("CLI")
    .field<bool>("verbose", false,
        with<cli_meta>({.cli = {.short_flag = 'v'}}),
        with<render_meta>({.render = {.style = "dim"}}));

t.field("verbose").meta<cli_meta>().cli.short_flag;       // 'v'
t.field("verbose").meta<render_meta>().render.style;       // "dim"
```

---

## Type-Level Metadata with `meta<T>`

Attach metadata to the type itself, not just fields.

**Typed path** — `meta<M>` members on the struct:
```cpp
struct Dog {
    meta<endpoint_info> endpoint{{.path = "/dogs", .method = "POST"}};
    meta<help_info>     help{{.summary = "A good boy"}};
    field<std::string>  name;
    field<int>          age;
};

type_def<Dog> t;
t.has_meta<endpoint_info>();         // true
t.meta<endpoint_info>().path;        // "/dogs"
t.meta_count<endpoint_info>();       // 1
```

**Dynamic path** — chained `.meta<M>()` calls:
```cpp
auto t = type_def("Event")
    .meta<endpoint_info>({.path = "/events", .method = "POST"})
    .meta<help_info>({.summary = "An event"})
    .field<std::string>("title");

t.has_meta<endpoint_info>();         // true
t.meta<endpoint_info>().path;        // "/events"
t.meta<tag_info>();                  // throws std::logic_error — not present
```

Multiple metas of the same type are supported:

```cpp
struct TaggedThing {
    meta<tag_info> tag1{{.value = "pet"}};
    meta<tag_info> tag2{{.value = "animal"}};
    field<std::string> name;
};

type_def<TaggedThing> t;
t.meta_count<tag_info>();            // 2
t.meta<tag_info>().value;            // "pet" — returns the first
auto all = t.metas<tag_info>();      // vector of all
```

---

## Querying the Schema

Every `type_def` — typed or dynamic — gives you:

| Method | Returns | Description |
|--------|---------|-------------|
| `name()` | `string_view` | Type name |
| `field_count()` | `size_t` | Number of fields (excludes metas) |
| `field_names()` | `vector<string>` | Ordered field names |
| `has_field(name)` | `bool` | Field exists? |
| `field(name)` | `field_def` | Field descriptor (throws if not found) |
| `has_meta<M>()` | `bool` | Has metadata of type M? |
| `meta<M>()` | `M` | First metadata of type M (throws if absent) |
| `meta_count<M>()` | `size_t` | Count of metadata of type M |
| `metas<M>()` | `vector<M>` | All metadata of type M |

### Field Descriptors (`field_def`)

`field(name)` returns a descriptor you can query further:

```cpp
auto t = type_def("Event")
    .field<std::string>("title")
    .field<int>("attendees", 100,
        with<render_meta>({.render = {.style = "bold", .width = 10}}));

auto f = t.field("attendees");
f.name();                    // "attendees"
f.default_value<int>();      // 100
f.has_meta<render_meta>();   // true
f.meta<render_meta>().render.style;  // "bold"
```

---

## Working with Data

### set() and get()

**Typed path** — `t.set(instance, field_name, value)` and `t.get<T>(instance, field_name)`:

```cpp
type_def<Dog> t;
Dog rex;

t.set(rex, "name", std::string("Rex"));
t.set(rex, "age", 3);
t.get<std::string>(rex, "name");    // "Rex"
t.get<int>(rex, "age");             // 3
```

Works the same way with plain structs — PFR discovers the members automatically:

```cpp
type_def<PlainDog> t;
PlainDog rex;

t.set(rex, "name", std::string("Rex"));
t.get<std::string>(rex, "name");    // "Rex"
rex.name;                            // "Rex" — it's just a plain member
```

**Dynamic path** — set/get are methods on the instance:

```cpp
auto obj = event_t.create();
obj.set("title", std::string("Dog Party"));
obj.get<std::string>("title");   // "Dog Party"
```

### Callback-Style get()

For the typed path, a callback form gives you the typed reference directly:

```cpp
type_def<Dog> t;
Dog rex;
rex.age = 25;

t.get(rex, "age", [&](std::string_view name, auto& value) {
    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, int>) {
        value = 99;  // mutate in-place
    }
});
rex.age.value;  // 99
```

### Error Handling

Type mismatches throw `std::logic_error` and **do not modify** the field:

```cpp
t.set(rex, "name", 42);              // throws — type mismatch
t.set(rex, "nonexistent", 42);       // throws — unknown field
t.set(rex, "", 42);                  // throws — empty field name
t.get<int>(rex, "name");             // throws — wrong type
t.set(rex, "endpoint", 42);          // throws — "endpoint" is a meta, not a field
```

---

## Iteration

### Iterating Fields with Values

**Typed** — you get real typed references:

```cpp
type_def<Dog>{}.for_each_field(rex, [](std::string_view name, auto& value) {
    // Visits: name (std::string&), age (int&), breed (std::string&)
    // Does NOT visit meta<> members
});
```

**Dynamic** — via `field_value` wrapper:

```cpp
obj.for_each_field([](std::string_view name, field_value value) {
    if (name == "title")
        value.as<std::string>() = "New Title";  // mutable typed access
});

// const iteration
const auto& const_obj = obj;
const_obj.for_each_field([](std::string_view name, const_field_value value) {
    // read-only access
});
```

### Iterating Field Descriptors (Schema Only)

Iterate field descriptors without an instance — useful for building UIs, generating help text:

```cpp
// Typed — compile-time descriptor
type_def<CliArgs>{}.for_each_field([](auto descriptor) {
    descriptor.name();
    if constexpr (descriptor.template has_meta<cli_meta>()) {
        auto flag = descriptor.template meta<cli_meta>().cli.short_flag;
    }
});

// Dynamic — field_def
event_t.for_each_field([](field_def fd) {
    fd.name();
    fd.default_value<int>();    // returns the default
    fd.has_meta<cli_meta>();
});
```

### Iterating Type-Level Metas

```cpp
// Typed — real typed references
type_def<Dog>{}.for_each_meta([](auto& meta_value) {
    using M = std::remove_cvref_t<decltype(meta_value)>;
    if constexpr (std::is_same_v<M, endpoint_info>)
        // meta_value.path, meta_value.method
});

// Dynamic — metadata wrapper with type checking
t.for_each_meta([](metadata meta_entry) {
    if (meta_entry.is<endpoint_info>())
        meta_entry.as<endpoint_info>().path;
    auto* help = meta_entry.try_as<help_info>();  // returns nullptr if wrong type
});
```

---

## JSON Serialization

### to_json — Struct to JSON

**Typed path with `field<>` members** — any reflected struct can be serialized:

```cpp
Dog rex;
rex.name = "Rex";
rex.age = 3;
rex.breed = "Husky";

nlohmann::json j = to_json(rex);
// {"name": "Rex", "age": 3, "breed": "Husky"}

std::string s = to_json_string(rex);
// compact JSON string

std::string pretty = to_json_string(rex, 2);
// pretty-printed with indent=2
```

**Plain structs** — pass the `type_def` so the framework knows the schema:

```cpp
struct PlainDog {
    std::string name;
    int age = 0;
    std::string breed;
};

PlainDog buddy{"Buddy", 3, "Lab"};
type_def<PlainDog> t;

nlohmann::json j = to_json(buddy, t);
// {"name": "Buddy", "age": 3, "breed": "Lab"}

std::string s = to_json_string(buddy, t);
```

**Dynamic path** — via the type_instance:

```cpp
auto t = type_def("Dog")
    .field<std::string>("name")
    .field<int>("age");

auto obj = t.create();
obj.set("name", std::string("Rex"));
obj.set("age", 3);

nlohmann::json j = obj.to_json();
// {"name": "Rex", "age": 3}

std::string s = obj.to_json_string();
std::string pretty = obj.to_json_string(2);
```

### from_json — JSON to Struct

**Typed path with `field<>` members:**

```cpp
auto j = nlohmann::json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}};
auto dog = from_json<Dog>(j);
dog.name.value;   // "Rex"
dog.age.value;    // 3

// Or from a JSON string
auto dog2 = from_json_string<Dog>(R"({"name": "Buddy", "age": 5, "breed": "Lab"})");
```

**Plain structs** — PFR discovers the fields, so `from_json<T>` works directly:

```cpp
auto buddy = from_json<PlainDog>(nlohmann::json{{"name", "Buddy"}, {"age", 5}, {"breed", "Lab"}});
buddy.name;    // "Buddy"
buddy.age;     // 5
buddy.breed;   // "Lab"
```

**Dynamic path** — via `create(json)` or `load_json`:

```cpp
auto t = type_def("Dog")
    .field<std::string>("name")
    .field<int>("age", 0);

// Create from JSON
auto obj = t.create(nlohmann::json{{"name", "Rex"}, {"age", 3}});
obj.get<std::string>("name");   // "Rex"
obj.get<int>("age");            // 3

// Or overlay JSON onto existing instance (partial updates)
auto obj2 = t.create();
obj2.load_json(nlohmann::json{{"age", 5}});
obj2.get<int>("age");            // 5
obj2.get<std::string>("name");   // "" (unchanged, kept default)
```

### Complex Types in JSON

Enums, containers, nested structs, and optionals are all handled automatically:

```cpp
enum class Color { red, green, blue };

struct Pet {
    field<std::string>                   name;
    field<Color>                         color;      // serialized as "red", "green", "blue"
    field<std::vector<std::string>>      tags;       // JSON array
    field<std::optional<std::string>>    nickname;   // omitted when empty, otherwise string
    field<std::map<std::string, int>>    scores;     // JSON object
};

Pet p;
p.name = "Rex";
p.color = Color::blue;
p.tags.value = {"good", "fluffy"};
p.nickname = "Rexy";
p.scores.value = {{"agility", 9}, {"cuteness", 10}};

auto j = to_json(p);
// {
//   "name": "Rex",
//   "color": "blue",
//   "tags": ["good", "fluffy"],
//   "nickname": "Rexy",
//   "scores": {"agility": 9, "cuteness": 10}
// }

auto restored = from_json<Pet>(j);
// Everything round-trips correctly
```

**Empty `std::optional<T>` is omitted from the JSON output** — it does not emit `null`. This matches how most protocols (JSON-RPC 2.0, OpenAPI/JSON Schema, ACP) distinguish absent from null:

```cpp
Pet stray;
stray.name = "Whiskers";
stray.color = Color::green;
// nickname is left empty (std::nullopt)

auto j = to_json(stray);
// {
//   "name": "Whiskers",
//   "color": "green",
//   "tags": [],
//   "scores": {}
//   // ← no "nickname" key at all
// }
```

On the way in, `from_json` accepts both shapes — an absent key and a `null` value both deserialize to `std::nullopt`.

### Nested Structs in JSON

Nested reflected structs serialize/deserialize recursively:

```cpp
struct Address {
    field<std::string> street;
    field<std::string> zip;
};

struct Person {
    field<std::string> name;
    field<Address>     address;
};

Person p;
p.name = "Alice";
p.address.value.street = "123 Main St";
p.address.value.zip = "97201";

auto j = to_json(p);
// {"name": "Alice", "address": {"street": "123 Main St", "zip": "97201"}}

auto restored = from_json<Person>(j);
restored.address.value.street.value;  // "123 Main St"
```

---

## Variants with `oneof<>`

`oneof<A, B, C>` is a value that holds exactly one of `A`, `B`, or `C`. Internally it's a `std::variant`, but the public surface is smaller and `to_json` / `from_json` know how to serialize it three different ways depending on what your JSON looks like.

There are three forms, picked by what the JSON looks like:

| Form | Template | Where the JSON says "which type is this?" |
|---|---|---|
| Shape-only | `oneof<A, B, C>` | Nowhere — the type is picked by which fields are present |
| Inside-object | `oneof_by_field<"key", oneof_type<A, "labelA">, ...>` | A string field inside the variant's own JSON object |
| Outside-object | `oneof_by_parent_field<&P::field, oneof_type<A, "labelA">, ...>` | A string field on the **parent** struct, beside the variant |

Every form has the same methods on the value: `is<T>()`, `as<T>()`, `match(...)`, copy/move, and **no default constructor** (the library will not pick a "first" type for you).

### Shape-only — `oneof<A, B, C>`

Use when the types have distinct field sets and the JSON has no field that names which type it is. The library figures out which one the JSON belongs to by checking which type's fields cover the JSON's keys.

```cpp
struct TextResource { std::string uri; std::string mime_type; std::string text; };
struct BlobResource { std::string uri; std::string mime_type; std::string blob; };

using Resource = oneof<TextResource, BlobResource>;

struct Envelope {
    std::string sender;
    Resource    payload = TextResource{};   // see "Default Construction" below
};

Envelope envelope{ "alice", TextResource{ "file:///hello.txt", "text/plain", "hello" } };
auto json = to_json(envelope);
// {
//   "sender": "alice",
//   "payload": { "uri": "file:///hello.txt", "mime_type": "text/plain", "text": "hello" }
// }

auto restored = from_json<Envelope>(json);
restored.payload.is<TextResource>();         // true
restored.payload.as<TextResource>()->text;   // "hello"
```

If the JSON keys don't fit any of the types — or fit more than one — `from_json` throws `std::logic_error` naming the offending key or the types that tied.

### Inside-object — `oneof_by_field<"key", oneof_type<A, "labelA">, ...>`

Use when every type shares a common JSON shape and a string field inside that object names which type it is.

The struct represents the JSON exactly: the discriminator key (`"type"` here) is a real field on each alternative struct. The library reads it on deserialize to pick which alternative the JSON is. It never invents or rewrites this field — what's in the struct is what goes to JSON.

```cpp
struct TextContent  { std::string type = "text";  std::string text; };
struct ImageContent { std::string type = "image"; std::string url; int width; int height; };

using Content = oneof_by_field<"type",
    oneof_type<TextContent,  "text">,
    oneof_type<ImageContent, "image">>;

struct Message {
    std::string sender;
    Content     content = TextContent{};
};

Message message{ "alice", ImageContent{ .url = "https://example.com/cat.png", .width = 400, .height = 300 } };
auto json = to_json(message);
// {
//   "sender": "alice",
//   "content": {
//     "type": "image",
//     "url": "https://example.com/cat.png",
//     "width": 400,
//     "height": 300
//   }
// }

auto restored = from_json<Message>(json);
restored.content.is<ImageContent>();              // true
restored.content.as<ImageContent>()->url;         // "https://example.com/cat.png"
```

The default member initializer (`std::string type = "image"`) makes designated-init like `ImageContent{.url = "..."}` produce a struct whose `type` already matches the label. If you assign a value to `type` that doesn't match the label declared in the matching `oneof_type<>`, that's what goes to JSON — the library doesn't second-guess you, but `from_json` on that JSON will fail to find an alternative for the bad value.

Errors:
- Missing the discriminator field on the JSON → throws naming the missing key and listing known labels.
- Discriminator value doesn't match any label → throws naming the bad value and listing known labels.
- Duplicate labels in the same `oneof_by_field<>` → `static_assert` at the variant declaration.

### Outside-object — `oneof_by_parent_field<&P::field, oneof_type<A, "labelA">, ...>`

Use when the discriminator lives on the parent struct, beside the JSON object that holds the variant's data.

Same trust model: the parent's discriminator field is yours. The library reads it on deserialize to pick the alternative. You set it consistently with the active alternative when constructing — the library doesn't sync it for you.

```cpp
struct OuterMessage {
    std::string sender;
    std::string content_type;     // ← the field that says which type `content` is

    oneof_by_parent_field<&OuterMessage::content_type,
        oneof_type<TextContent,  "text">,
        oneof_type<ImageContent, "image">> content = TextContent{};
};

OuterMessage message{
    "alice",
    "image",                                          // ← user keeps this in sync with `content`
    ImageContent{ .url = "https://example.com/cat.png", .width = 400, .height = 300 }
};
auto json = to_json(message);
// {
//   "sender": "alice",
//   "content_type": "image",
//   "content": {
//     "type": "image",
//     "url": "https://example.com/cat.png",
//     "width": 400,
//     "height": 300
//   }
// }
```

On deserialize, fields are visited in declaration order. `content_type` is read first as a normal string field. When the variant field is reached, the library reads `outer_message.content_type` (already populated) and uses its value to pick the alternative.

**Constraint:** the discriminator field must be declared **before** the variant field in the parent struct. Otherwise the variant's deserializer reads an empty string and throws.

### Methods on the value

Every `oneof` form exposes the same five methods.

```cpp
oneof<Dog, Cat> pet = Dog{ "Rex", 3 };

// is<T>() — is the active type T?
pet.is<Dog>();                          // true
pet.is<Cat>();                          // false

// as<T>() — pointer to the active value, or nullptr
if (auto* dog = pet.as<Dog>())          // non-null when Dog is active
    dog->age = 7;
pet.as<Cat>();                          // nullptr

// match(...) — visit the active value; like std::visit but on the oneof directly
auto label = pet.match(
    [](const Dog&) { return std::string("dog"); },
    [](const Cat&) { return std::string("cat"); }
);

// reassign — accepts any of the listed types directly
pet = Cat{ "Whiskers", true };
```

`is<T>()` and `as<T>()` are constrained — calling them with a type that isn't in the variant is a compile error.

`match` follows `std::visit`'s rule: every type in the variant must be handled by some overload, or by a generic catch-all `[](const auto&) { ... }`.

### Default Construction

`oneof<>` has no default constructor — the library will not pick a "first" type on your behalf. If you put a `oneof` inside a struct that you want to be default-constructible (or that you want to deserialize via `from_json<Parent>(json)`), give the field an explicit default initializer:

```cpp
struct Message {
    std::string sender;
    Content     content = TextContent{};   // ← required so Message is default-constructible
};
```

Without that, `Message{}` won't compile and `from_json<Message>(json)` won't work.

### Inside other types

`oneof<>` and the helpers work inside containers, optionals, and nested structs without any extra opt-in:

```cpp
using Pet = oneof<Dog, Cat>;

struct Shelter      { std::vector<Pet>                pets;        };
struct PetCarrier   { std::optional<Pet>              maybe_pet;   };
struct PetRegistry  { std::map<std::string, Pet>      by_name;     };
```

All round-trip through `to_json` / `from_json` automatically.

### Manual JSON via nlohmann hooks

When none of the three built-in forms match the JSON your protocol uses — labels that aren't strings, the selector field is several levels above the variant, anything custom — write the standard `nlohmann::json` `to_json` / `from_json` functions in the same namespace as your variant's type. They're found automatically by C++ lookup and **always win** over the library's built-in handling:

```cpp
namespace orchard {
    struct Apple { int weight_grams; };
    struct Pear  { int ripeness;     };

    using Fruit = def_type::oneof<Apple, Pear>;

    void to_json(nlohmann::json& json, const Fruit& fruit) {
        fruit.match(
            [&](const Apple& apple) {
                json = {
                    { "fruit_type",   "apple" },
                    { "weight_grams", apple.weight_grams }
                };
            },
            [&](const Pear& pear) {
                json = {
                    { "fruit_type", "pear" },
                    { "ripeness",   pear.ripeness }
                };
            }
        );
    }

    void from_json(const nlohmann::json& json, Fruit& fruit) {
        auto fruit_type = json.at("fruit_type").get<std::string>();
        if (fruit_type == "apple")
            fruit = Apple{ json.at("weight_grams").get<int>() };
        else
            fruit = Pear{ json.at("ripeness").get<int>() };
    }
}
```

If you write one, write both — the library detects them as a pair, and falls back to its default for the side you didn't write.

---

## Type-Erased JSON with `unknown`

`unknown` is a field type that holds raw JSON whose shape you don't know yet. It's the right tool when you can't enumerate the alternatives at compile time — open-set RPC dispatch, plugin parameters, partially-typed payloads — and `oneof<>` therefore doesn't fit.

A motivating example: a JSON-RPC 2.0 envelope, where `params` varies per `method` and the set of methods isn't fixed at compile time:

```cpp
struct JsonRpcRequest {
    std::string jsonrpc;
    int64_t     id;
    std::string method;
    unknown     params;
};

auto req = from_json_string<JsonRpcRequest>(R"({
    "jsonrpc": "2.0",
    "id": 1,
    "method": "describe-dog",
    "params": { "name": "Rover", "age": 12, "breed": "Golden Retriever" }
})");
```

`req.params` now holds the raw JSON for `params`. Probe it when you've decided what shape you expect:

```cpp
struct Dog { std::string name; int age; std::string breed; };

req.params.is<Dog>();              // true — shape matches Dog
req.params.as<Dog>();              // returns a Dog
req.params.try_as<Dog>();          // returns std::optional<Dog>
```

### `is<T>()` / `as<T>()` / `try_as<T>()`

The three methods follow the same semantics:

| Method | Returns | On mismatch |
|---|---|---|
| `is<T>()` | `bool` | `false` |
| `as<T>()` | `T` | throws `std::logic_error` |
| `try_as<T>()` | `std::optional<T>` | `std::nullopt` |

For reflected struct types, all three apply a **shape-match gate** before parsing — every JSON key must name a field of `T`. This mirrors how `oneof<>`'s shape-only dispatch resolves alternatives, and prevents the library's permissive deserialize-with-defaults behavior from masquerading as a successful match. For non-reflected types (primitives, containers, optionals), the check is just whether the parse would succeed.

```cpp
unknown u(nlohmann::json{{"name", "Whiskers"}, {"indoor", true}});
u.is<Dog>();   // false — "indoor" is not a Dog field
```

### Drilling in by path — `get<T>(path)` / `try_get<T>(path)` / `is<T>(path)`

`as<T>` / `try_as<T>` operate on the **whole** stored JSON — there's no path overload, because the verb means "interpret this value as `T`."

For nested access there's a separate verb:

| Method | Returns | On mismatch |
|---|---|---|
| `get<T>(path)` | `T` | throws `std::logic_error` |
| `try_get<T>(path)` | `std::optional<T>` | `std::nullopt` |
| `is<T>(path)` | `bool` | `false` |

Two path syntaxes, picked by the leading character:

- **Dotted path** (default): `"breed.name"` — friendly for object navigation.
- **JSON Pointer** (RFC 6901), when the path starts with `/`: `"/breed/name"`, `"/items/0"` — supports array indexing and edge-case keys.

Dotted paths are translated to JSON Pointer internally and walked via `nlohmann::json`, so both forms are equally fast.

```cpp
unknown u(nlohmann::json{
    {"name", "Rover"},
    {"age", 12},
    {"breed", {{"name", "Golden Retriever"}, {"origin", "Scotland"}}}
});

u.get<std::string>("name");                // "Rover"
u.get<int>("age");                         // 12
u.get<std::string>("breed.name");          // "Golden Retriever"
u.get<DogBreed>("breed");                  // pulls a sub-object out as a struct
u.get<std::string>("/items/0");            // JSON Pointer for array indexing
u.is<int>("breed.name");                   // false — value is a string
u.try_get<std::string>("breed.nope");      // nullopt — bad path
```

### Round-tripping

`unknown` round-trips through `to_json` / `from_json` automatically — `to_json(req)` emits the originally-stored JSON for the `unknown` field:

```cpp
auto round = to_json(req);
// round["params"] is identical to the JSON that came in
```

### Constructing from a typed C++ value

Assigning a typed value serializes it on the spot and stashes the resulting JSON. Subsequent `to_json` is just a dump — no re-serialize. Subsequent `as<T>()` does a fresh deserialize.

```cpp
JsonRpcRequest req{
    .jsonrpc = "2.0",
    .id      = 10,
    .method  = "create-dog",
    .params  = Dog{ "Rex", 3, "Husky" }     // serialized via to_json(Dog{...})
};

auto j = to_json(req);
// j["params"] == { "name": "Rex", "age": 3, "breed": "Husky" }
```

You can also assign a `nlohmann::json` directly:

```cpp
unknown u;
u = nlohmann::json{{"foo", 1}};
```

### Field-level metadata and validators

`unknown` plugs into `field<T, with<...>>` and the validator infrastructure like any other type:

```cpp
struct help_meta { const char* summary = ""; };

struct must_parse_as_dog {
    std::vector<validation_error> operator()(const unknown& u) const {
        if (u.is<Dog>()) return {};
        return { { .message = "params do not match Dog shape" } };
    }
};

struct CreateDogRequest {
    std::string method;
    field<unknown, with<help_meta>> params {
        .with       = {{.summary = "the dog payload"}},
        .validators = validators(must_parse_as_dog{})
    };
};
```

Validators are just plain functions taking `const unknown&` — they probe with the same `.is<T>()` / `.try_as<T>()` API as everyone else.

### Raw access

If you need to drop down to nlohmann directly:

```cpp
const nlohmann::json& raw = u.raw();
nlohmann::json&       mut = u.raw();
```

---

## Dynamic Nesting

Dynamic type_defs can nest other dynamic type_defs as fields:

```cpp
auto address_type = type_def("Address")
    .field<std::string>("street", std::string(""))
    .field<std::string>("zip", std::string(""));

auto person_type = type_def("Person")
    .field<std::string>("name")
    .field("address", address_type);   // nested type_def — no template parameter

auto person = person_type.create();
person.set("name", std::string("Alice"));

// Access nested instance
auto address = person.get<type_instance>("address");
address.set("street", std::string("123 Main St"));
address.set("zip", std::string("97201"));
person.set("address", address);
```

### Nested JSON — Dynamic

Nesting works seamlessly with JSON:

```cpp
// to_json
auto j = person.to_json();
// {"name": "Alice", "address": {"street": "123 Main St", "zip": "97201"}}

// create from JSON
auto person2 = person_type.create(nlohmann::json{
    {"name", "Bob"},
    {"address", {{"street", "456 Oak Ave"}, {"zip", "90210"}}}
});

// load_json overlay
person.load_json(nlohmann::json{
    {"address", {{"zip", "99999"}}}   // only zip changes
});
```

### 3-Level Deep Nesting

```cpp
auto leaf = type_def("Leaf").field<std::string>("value", std::string("deep"));
auto middle = type_def("Middle")
    .field<std::string>("label", std::string("mid"))
    .field("leaf", leaf);
auto root = type_def("Root")
    .field<std::string>("name", std::string("top"))
    .field("middle", middle);

auto obj = root.create();
auto j = obj.to_json();
// {"name": "top", "middle": {"label": "mid", "leaf": {"value": "deep"}}}
```

---

## Validation

```cpp
struct passwords_match {
    template <typename S>
    std::vector<validation_error> operator()(const S& s) const {
        if (s.password.value == s.confirm.value) return {};
        return {{.message = "passwords do not match", .sub_path = "confirm"}};
    }
};

struct SignupForm {
    type_validators<passwords_match> rules { passwords_match{} };

    field<std::string> password { .value = "", .validators = validators(not_empty{}) };
    field<std::string> confirm  { .value = "", .validators = validators(not_empty{}) };
};

SignupForm form;
form.password = "abc";
form.confirm  = "xyz";

if (auto result = type_def<SignupForm>{}.validate(form); !result) {
    for (auto& error : result)
        fmt::print("{}: {} ({})\n", error.path, error.message, error.validator);
}
// → confirm: passwords do not match (passwords_match)
```

`validation_result` is truthy when there are no errors, and iterates directly with range-for — `if (!result)` and `for (auto& e : result)` are the canonical idioms.

Validators are just structs with `operator()` returning `std::vector<validation_error>` — empty vector means valid, any entries mean invalid. There are two shapes:

- **Field validators** receive one field's value. Attach via `field<T>::validators` (typed) or pass `validators(...)` to `.field<V>(...)` on the dynamic builder.
- **Whole-struct validators** receive the whole instance — `const T&` (typed) or `const type_instance&` (dynamic). Attach via a `type_validators<Vs...>` marker member (typed) or `.validators(...)` on the builder (dynamic).

Both fire together when you call `valid()` / `validate()` / `parse()`. Everything below — built-ins, writing your own, multi-finding, `.code` / `.data` / `.sub_path`, "doesn't block I/O", nesting — applies to both unless noted.

### Built-in Validators

def_type ships with a few field-level built-ins in the `def_type::validations` namespace. They're domain-agnostic convenience helpers — you'll likely write your own for anything specific (see [Writing Your Own](#writing-your-own) below).

```cpp
using namespace def_type::validations;
```

| Validator | Applies to | Fails when |
|-----------|-----------|------------|
| `not_empty{}` | anything with `.empty()` — `std::string`, `std::vector<T>`, `std::set<T>`, `std::map<...>`, your own | `.empty()` returns true |
| `max_length{N}` | anything with `.size()` — `std::string`, all standard containers, your own | `.size() > N` |
| `in_range<T>{.min, .max}` | `T` satisfying `std::totally_ordered` — `int`, `int64_t`, `double`, `enum class` with operators, your own | value is outside `[min, max]` |

`in_range` is constrained on `T` and refuses to be called on anything other than `T` — no implicit narrowing, no silent widening, no sign mismatch. Pairing `in_range<int>` with a `field<double>` is a compile error at the field declaration. Deduce `T` via aggregate CTAD (either positional or designated) or spell it explicitly:

```cpp
in_range{1, 100}                          // in_range<int>
in_range{0.0, 1.0}                        // in_range<double>
in_range{.min = 1, .max = 100}            // in_range<int>
in_range<std::int64_t>{.min = 0, .max = 9}
```

Combine validators with `validators(...)`:

```cpp

// Dynamic
auto t = type_def("Config")
    .field<std::string>("token", std::string(""),
        validators(not_empty{}, max_length{50}))
    .field<int>("retries", 0,
        validators(in_range{.min = 1, .max = 999}));

// Typed
struct Config {
    field<std::string> token {
        .value = "",
        .validators = validators(not_empty{}, max_length{50})
    };
    field<int> retries {
        .value = 0,
        .validators = validators(in_range{.min = 1, .max = 999})
    };
};
```

### Writing Your Own

A validator is a struct with `operator()` returning `std::vector<validation_error>`. No base class, no registration, no macros. The framework fills `.validator` (via `nameof`) and joins your optional `.sub_path` into `.path` — you fill `.message` and any of `.code` / `.sub_path` / `.data`.

**Field validator** — receives the field's value:

```cpp
struct starts_with_uppercase {
    std::vector<validation_error> operator()(const std::string& value) const {
        if (value.empty() || std::isupper(static_cast<unsigned char>(value[0])))
            return {};
        return { { .message = std::format("'{}' must start with an uppercase letter", value) } };
    }
};
```

**Whole-struct validator** — receives the whole instance. Typed validators take `const T&` and read fields directly; templating `operator()` on `S` lets the validator be defined *before* the struct it inspects, avoiding a forward-declaration dance:

```cpp
struct passwords_match {
    template <typename S>
    std::vector<validation_error> operator()(const S& s) const {
        if (s.password.value == s.confirm.value) return {};
        return { { .message  = "passwords do not match",
                   .code     = "password_mismatch",
                   .sub_path = "confirm" } };
    }
};
```

Dynamic-path validators take `const type_instance&` and probe by field name:

```cpp
struct passwords_match {
    std::vector<validation_error> operator()(const type_instance& obj) const {
        if (obj.get<std::string>("password") == obj.get<std::string>("confirm"))
            return {};
        return { { .message  = "passwords do not match",
                   .code     = "password_mismatch",
                   .sub_path = "confirm" } };
    }
};
```

For the common "one rule, one message" validator, the designated-init form `{ { .message = "..." } }` keeps it to a single line. For more, see [Structured Findings](#structured-findings--code-data-and-multi-finding-validators).

### Attaching to Fields

**Typed path** — set `.validators` on a `field<T>`:

```cpp
struct Dog {
    field<std::string> name {
        .value = "",
        .validators = validators(not_empty{}, starts_with_uppercase{})
    };
    field<int> age {
        .value = 0,
        .validators = validators(in_range{.min = 0, .max = 25})
    };
};
```

**Dynamic path** — pass `validators(...)` to `.field<V>(...)`:

```cpp
auto t = type_def("Dog")
    .field<std::string>("name", std::string(""),
        validators(not_empty{}, starts_with_uppercase{}))
    .field<int>("age", 0,
        validators(in_range{.min = 0, .max = 25}));
```

On the dynamic builder you can also skip the default value — the field gets the type's default (`""` for string, `0` for int):

```cpp
auto t = type_def("Dog")
    .field<std::string>("name", validators(not_empty{}))
    .field<int>("age", validators(in_range{.min = 1, .max = 999}));
```

### Attaching to the Whole Struct

Whole-struct validators attach via `type_validators<Vs...>` on the typed path and `.validators(...)` on the dynamic builder.

**Typed path — `type_validators<>` marker.** It's the type-level counterpart to `meta<>`: a member that holds validation rules, discovered by the framework, invisible to everything else (not counted by `field_count`, not visited by `for_each_field`, not addressable via `set` / `get` / `has_field`, not emitted by `to_json`, ignored by `from_json`).

```cpp
struct SignupForm {
    type_validators<passwords_match> rules { passwords_match{} };

    field<std::string> password { .value = "", .validators = validators(not_empty{}) };
    field<std::string> confirm  { .value = "", .validators = validators(not_empty{}) };
    field<bool>        terms    { .value = false };
};

SignupForm form;
form.password = "abc";
form.confirm  = "xyz";

auto result = type_def<SignupForm>{}.validate(form);
result.error_count();              // 1
result.errors()[0].path;           // "confirm"
result.errors()[0].validator;      // "passwords_match"
```

Multiple rules in one marker, or multiple markers — both additive:

```cpp
// One marker, several rules
type_validators<passwords_match, terms_accepted> rules {
    passwords_match{},
    terms_accepted{.required = true}
};

// Or split into multiple markers — group however reads best
struct SignupForm {
    type_validators<passwords_match>  cross_field { passwords_match{}  };
    type_validators<terms_accepted>   policy      { terms_accepted{}   };
    // ... fields ...
};
```

Stateless validators degrade to empty braces: `type_validators<must_be_uppercase> rules{};`.

**Dynamic path — `.validators(...)` on the builder.** Variadic and chainable, both forms additive:

```cpp
auto signup_t = type_def("SignupForm")
    .field<std::string>("password", std::string(""), validators(not_empty{}))
    .field<std::string>("confirm",  std::string(""), validators(not_empty{}))
    .field<bool>("terms", false)
    .validators(passwords_match{});

// Equivalent forms:
.validators(rule_a{}, rule_b{})              // variadic
.validators(rule_a{}).validators(rule_b{})   // chained
```

**How `.path` is built for whole-struct findings.** The framework joins the parent field path with the validator-supplied `.sub_path`:

| Parent prefix | `.sub_path` | Resulting `.path` |
|---|---|---|
| empty (top-level) | absent | `""` |
| empty (top-level) | `"confirm"` | `"confirm"` |
| `"address"` (nested) | absent | `"address"` |
| `"address"` (nested) | `"zip"` | `"address.zip"` |

To mark *multiple* offending fields, return multiple findings — each with its own `.sub_path` (see [Structured Findings](#structured-findings--code-data-and-multi-finding-validators)).

**Querying which types carry whole-struct rules:**

```cpp
type_def<SignupForm>{}.has_validators();      // bool — any whole-struct rules?
type_def<SignupForm>{}.validator_count();     // size_t — total across all markers

signup_t.has_validators();                    // same on the dynamic path
signup_t.validator_count();
```

### Running Validators

Two entry points. **`valid()`** for a quick yes/no — short-circuits on first failure:

```cpp
type_def<SignupForm>{}.valid(form);    // true / false
signup_t.create().valid();             // dynamic equivalent
```

**`validate()`** for the full report — every field validator, every whole-struct validator, every level of nesting:

```cpp
if (auto result = type_def<SignupForm>{}.validate(form); !result) {
    for (auto& error : result)
        fmt::print("{}: {} ({})\n", error.path, error.message, error.validator);
}
```

`validation_result` surface:

| API | Returns | Notes |
|---|---|---|
| `bool(result)` / `result.ok()` | `bool` | true when no errors |
| `!result` | `bool` | true when there *are* errors — the canonical conditional |
| `result.error_count()` | `std::size_t` | |
| `result.errors()` | `const std::vector<validation_error>&` | explicit vector access |
| `begin()` / `end()` | iterators | makes range-for work on the result directly |

Each `validation_error` carries:

```cpp
error.message;        // std::string  — human-readable, required
error.path;           // std::string  — final field path, e.g. "address.zip"
error.validator;      // std::string  — validator struct name (via nameof), e.g. "not_empty"

// Optional — populated only when the validator opted in:
error.code;           // std::optional<std::string> — stable id for this finding
error.sub_path;       // std::optional<std::string> — position inside the value
error.data;           // std::optional<unknown>     — typed user struct with extras
```

**Who fills what:** validators fill `.message` (required) and may fill `.code`, `.sub_path`, `.data`. The framework fills `.path` (field name joined with `sub_path`) and `.validator` (the struct's name via `nameof`). Anything a validator writes into `.path` or `.validator` gets overwritten.

### Structured Findings — `.code`, `.data`, and Multi-Finding Validators

Sometimes one validator has more than one thing to say. A password-policy check can fail for "too short," "missing uppercase," and "missing digit" all at the same time — and you want each of those as a separate `validation_error` so consumers (UI, telemetry, i18n) can act on them individually.

The validator just returns a vector with more than one entry:

```cpp
struct password_policy {
    std::vector<validation_error> operator()(const std::string& v) const {
        std::vector<validation_error> findings;
        if (v.size() < 8)
            findings.push_back({.message = "must be at least 8 characters",
                                .code = "too_short"});
        bool has_upper = false, has_digit = false;
        for (char c : v) {
            if (c >= 'A' && c <= 'Z') has_upper = true;
            if (c >= '0' && c <= '9') has_digit = true;
        }
        if (!has_upper)
            findings.push_back({.message = "must contain an uppercase letter",
                                .code = "missing_uppercase"});
        if (!has_digit)
            findings.push_back({.message = "must contain a digit",
                                .code = "missing_digit"});
        return findings;
    }
};
```

All three findings carry `error.validator == "password_policy"` and the same field path — but each has a distinct `.code` the consumer can switch on:

```cpp
for (auto& error : result.errors()) {
    if (error.code == "too_short")           ui.show_strength_meter();
    else if (error.code == "missing_uppercase") telemetry.bump("pw.no_upper");
    else if (error.code == "missing_digit")     telemetry.bump("pw.no_digit");
}
```

**`.data` — typed structured extras.** When a finding has data the consumer might want as something other than text (numeric bounds, a list of valid choices, a referenced field), attach a user struct via `unknown`:

```cpp
struct too_short_data          { int min; int actual; };
struct missing_char_class_data { std::string class_name; };

struct password_policy_with_data {
    std::vector<validation_error> operator()(const std::string& v) const {
        std::vector<validation_error> findings;
        if (v.size() < 8)
            findings.push_back({
                .message = "must be at least 8 characters",
                .code    = "too_short",
                .data    = unknown{too_short_data{.min = 8, .actual = (int)v.size()}}
            });
        // ...
        return findings;
    }
};
```

The consumer reads the typed data back with the same `.is<T>()` / `.try_as<T>()` API used everywhere else in the library:

```cpp
for (auto& error : result.errors()) {
    if (auto data = error.data ? error.data->try_as<too_short_data>() : std::nullopt; data)
        ui.show_hint(std::format("Need {} chars, got {}", data->min, data->actual));
}
```

The struct type can be anything reflected — primitives, containers, nested reflected structs all work.

**`.sub_path` — pointing inside a value.** When a validator looks at structured data (`unknown`, a nested struct, a vector) and finds an issue at a *position inside* the value, it reports that position in `.sub_path`. The framework concatenates it with the field's path using a dot:

```cpp
struct must_have_inner_field {
    std::vector<validation_error> operator()(const unknown& u) const {
        std::vector<validation_error> findings;
        if (!u.is<std::string>("name"))
            findings.push_back({.message = "missing inner 'name'",
                                .code = "missing_name",
                                .sub_path = "name"});
        if (!u.is<int>("age"))
            findings.push_back({.message = "missing inner 'age'",
                                .code = "missing_age",
                                .sub_path = "age"});
        return findings;
    }
};

// On a field named "params", the framework produces:
//   error.path == "params.name"     error.sub_path == "name"
//   error.path == "params.age"      error.sub_path == "age"
```

Sub-paths compose with nested validation too — a sub-path of `"name"` from a validator on `params` inside an `inner` struct ends up as `"inner.params.name"`.

The same multi-finding pattern is how a whole-struct validator points at several offending fields at once — one finding per field, each with its own `.sub_path`:

```cpp
struct passwords_match {
    template <typename S>
    std::vector<validation_error> operator()(const S& s) const {
        if (s.password.value == s.confirm.value) return {};
        return {
            {.message = "passwords do not match", .code = "mismatch", .sub_path = "password"},
            {.message = "passwords do not match", .code = "mismatch", .sub_path = "confirm"},
        };
    }
};
```

### Validators Don't Block Other Operations

Invalid values can still be set, retrieved, and serialized — validation is advisory, not enforcement:

```cpp
auto obj = t.create();
obj.set("name", std::string(""));   // allowed, even though not_empty will fail
obj.get<std::string>("name");       // ""
auto j = obj.to_json();             // {"name": "", "age": 0} — works fine
obj.valid();                         // false
```

### Validators Mixed with Metas

Validators and `with<>` metas compose freely on the same field, and `type_validators<>` sits alongside `meta<>` on the same struct without conflict:

```cpp
auto t = type_def("Dog")
    .field<std::string>("name", std::string(""),
        validators(not_empty{}),
        with<help_info>({.summary = "Dog's name"}))
    .field<int>("age", 0,
        validators(in_range{.min = 1, .max = 999}),
        with<cli_meta>({.cli = {.short_flag = 'a'}}),
        with<help_info>({.summary = "Age in years"}));

t.field("name").has_meta<help_info>();  // true
t.field("age").has_meta<cli_meta>();    // true

auto obj = t.create();
obj.valid();  // false — name is "", age is 0
```

### Nested Validation

Validation recurses into nested type_defs with **dotted paths**. Both field validators and whole-struct validators fire during outer validation, with paths prefixed by the parent field name.

**Dynamic nesting:**

```cpp
auto address_type = type_def("Address")
    .field<std::string>("street", std::string(""), validators(not_empty{}))
    .field<std::string>("zip", std::string(""), validators(not_empty{}));

auto person_type = type_def("Person")
    .field<std::string>("name", std::string("Alice"))
    .field("address", address_type);

auto person = person_type.create();
auto result = person.validate();

result.error_count();           // 2
result.errors()[0].path;        // "address.street"
result.errors()[1].path;        // "address.zip"
```

**Typed nesting** — nested `field<Struct>` types with validators recurse automatically:

```cpp
struct ValidatedAddress {
    field<std::string> street {.value = "", .validators = validators(not_empty{})};
    field<std::string> zip    {.value = "", .validators = validators(not_empty{})};
};

struct ValidatedPerson {
    field<std::string>          name {.value = "", .validators = validators(not_empty{})};
    field<ValidatedAddress>     address;
};

ValidatedPerson person;
person.name = "Alice";
auto result = type_def<ValidatedPerson>{}.validate(person);
// Errors at "address.street" and "address.zip"
```

**3-level deep dotted paths** work across all paths:

```cpp
// "middle.leaf.tag" — three levels deep
```

---

## Parse — JSON with Reporting

`parse()` is like `from_json` but returns a `parse_result<T>` with detailed information about what happened:

### parse_result<T>

```cpp
auto t = type_def("Dog")
    .field<std::string>("name", std::string(""), validators(not_empty{}))
    .field<int>("age")
    .field<std::string>("breed");

auto result = t.parse(nlohmann::json{
    {"name", ""},        // present but fails validation
    {"age", 3},          // present, no validator
    {"unknown", "wat"}   // extra key
    // breed is missing
});

result.valid();                    // false — validation error
result.has_extra_keys();           // true — "unknown"
result.has_missing_fields();       // true — "breed"

result.extra_keys();               // ["unknown"]
result.missing_fields();           // ["breed"]
result.validation_errors();        // [{path: "name", validator: "not_empty", ...}]

// The object is always fully populated — even when invalid
result->get<std::string>("name");  // ""
result->get<int>("age");           // 3
result->get<std::string>("breed"); // "" (default — it was missing)
```

### Value Access on parse_result

```cpp
// operator-> for field access
result->get<std::string>("name");

// operator* for the instance itself
type_instance& instance = *result;

// checked_value() — throws if !valid()
auto& safe = result.checked_value();   // throws if validation failed
```

### parse_options — Configurable Strictness

By default, `parse()` never throws. Use `parse_options` to make it strict:

```cpp
// Throw on extra keys
t.parse(json, {.reject_extra_keys = true});   // throws parse_error if extra keys

// Throw on missing fields
t.parse(json, {.require_all_fields = true});  // throws parse_error if fields missing

// Throw on validation errors
t.parse(json, {.require_valid = true});       // throws parse_error if validators fail

// All three at once
t.parse(json, {.strict = true});              // throws on any issue
```

### parse_error — Structured Exception

When `parse_options` trigger a throw, you get a `parse_error` (extends `std::logic_error`) with all the details:

```cpp
try {
    t.parse(json, {.strict = true});
} catch (const def_type::parse_error& e) {
    e.what();                 // human-readable message
    e.extra_keys();           // vector<string>
    e.missing_fields();       // vector<string>
    e.validation_errors();    // vector<validation_error>
}
```

### Type Mismatch Handling in Parse

When JSON has the wrong type for a field (e.g. string where int is expected), `parse()` gracefully keeps the default — it does not throw:

```cpp
auto t = type_def("Dog")
    .field<std::string>("name", std::string("default_name"))
    .field<int>("age", 42);

auto result = t.parse(json{{"name", "Rex"}, {"age", "not a number"}});
result->get<std::string>("name");   // "Rex"
result->get<int>("age");            // 42 — kept default
```

### Parse on Both Paths

```cpp
// Dynamic
auto result = type_def("Dog")
    .field<std::string>("name")
    .parse(json{{"name", "Rex"}});

// Typed
auto result = type_def<Dog>{}.parse(json{{"name", "Rex"}, {"age", 3}});
result->name.value;  // "Rex"
```

---

## PFR vs Manual Registration

def_type uses [PFR (Boost.PFR)](https://github.com/boostorg/pfr) for automatic field name discovery when available. PFR inspects aggregate structs at compile time — no registration needed. This is how `type_def<T>` auto-discovers both `field<>` wrapped and plain struct members.

When PFR is **not available** (or for non-aggregate types), you register fields manually via `struct_info<T>()`:

```cpp
struct MyStruct {
    field<std::string> name;
    field<int>         age;
};

// Only needed when PFR is not available
#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<MyStruct>() {
    return def_type::field_info<MyStruct>("name", "age");
}
#endif
```

The field names in `field_info` must match the member declaration order exactly.

---

## type_instance — Dynamic Runtime Objects

Dynamic type_defs produce instances via `create()`:

```cpp
auto t = type_def("Event")
    .field<std::string>("title")
    .field<int>("count", 100);

auto obj = t.create();

obj.get<int>("count");              // 100 (from default)
obj.set("title", std::string("Party"));
obj.get<std::string>("title");      // "Party"
obj.has("title");                   // true
obj.has("nope");                    // false

obj.type().name();                  // "Event"
obj.type().field_count();           // 2
```

### Multiple instances are independent

```cpp
auto a = t.create();
auto b = t.create();

a.set("title", std::string("Party A"));
b.set("title", std::string("Party B"));

a.get<std::string>("title");  // "Party A"
b.get<std::string>("title");  // "Party B"
```

---

## Safety

`def_type` is strict. If you do something wrong, you get a `std::logic_error` with a clear message:

- **Unknown field**: `set()`, `get()`, `field()` throw for names that don't exist
- **Type mismatch**: `set()` with wrong type throws, value is **not modified**
- **Empty field name**: throws on all operations
- **Absent meta**: `meta<M>()` throws on dynamic path when M isn't present
- **Meta member names**: `set()`, `get()`, `has_field()` correctly ignore `meta<>` members
- **Case-sensitive**: `has_field("Name")` is not the same as `has_field("name")`

```cpp
t.set(rex, "nonexistent", 42);       // throws — unknown field
t.set(rex, "name", 42);              // throws — type mismatch
t.get<int>(rex, "name");             // throws — wrong type on get
t.set(rex, "endpoint", 42);          // throws — meta, not a field
type_def("X").field<int>("x").meta<tag_info>();  // throws — absent meta
```

---

## Dependencies

- **Required:** [nlohmann/json](https://github.com/nlohmann/json), [fmt](https://github.com/fmtlib/fmt), [unordered_dense](https://github.com/martinus/unordered_dense), [nameof](https://github.com/Neargye/nameof)
- **Optional:** [PFR](https://github.com/boostorg/pfr) (automatic field names), [magic_enum](https://github.com/Neargye/magic_enum) (enum name serialization)

## Building

```bash
xmake f -m release -c -y
xmake build -a
xmake run tests-def_type
```
