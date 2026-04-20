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
auto buddy = from_json<Dog>(R"({"name": "Buddy", "age": 5, "breed": "Lab"})");
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
result.errors()[0].constraint;    // "in_range"
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
auto dog2 = from_json<Dog>(R"({"name": "Buddy", "age": 5, "breed": "Lab"})");
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
    field<std::optional<std::string>>    nickname;   // null or string
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

### Built-in Validators

def_type ships with a few validators in the `def_type::validations` namespace. These are convenience validators — you'll likely write your own for anything domain-specific (see [Custom Validators](#custom-validators) below).

```cpp
using namespace def_type::validations;
```

| Validator | Applies to | Fails when |
|-----------|-----------|------------|
| `not_empty{}` | `std::string` | String is empty |
| `max_length{N}` | `std::string` | String length exceeds N |
| `in_range{.min, .max}` | `int` | Value is outside [min, max] |

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

### Dynamic — Validators Without Default Value

You can attach validators without providing a default — the field gets the type's default (e.g. `""` for string, `0` for int):

```cpp
auto t = type_def("Dog")
    .field<std::string>("name", validators(not_empty{}))
    .field<int>("age", validators(in_range{.min = 1, .max = 999}));
```

### Custom Validators

Writing your own validator is trivial — it's just a struct with `operator()`. The contract:

- Takes `const T&` (the field value)
- Returns `std::optional<std::string>` — `std::nullopt` means valid, a string means invalid (the string is the error message)

That's it. No base class, no registration, no macros:

```cpp
struct starts_with_uppercase {
    std::optional<std::string> operator()(const std::string& value) const {
        if (value.empty() || std::isupper(static_cast<unsigned char>(value[0])))
            return std::nullopt;
        return std::format("'{}' must start with an uppercase letter", value);
    }
};

// Use them just like built-in validators:
auto t = type_def("Dog")
    .field<std::string>("name", validators(not_empty{}, starts_with_uppercase{}))
    .field<int>("age", validators(in_range{.min = 0, .max = 25}));
```

### valid() — Quick Check

Returns `true` if all validators pass:

```cpp
// Dynamic
auto obj = t.create();
obj.valid();            // false — name is "", age is 0
obj.set("name", std::string("Rex"));
obj.set("age", 3);
obj.valid();            // true

// Typed
ValidatedDog dog;
type_def<ValidatedDog> dog_t;
dog_t.valid(dog);       // false
dog.name = "Rex";
dog.age = 3;
dog_t.valid(dog);       // true
```

### validate() — Detailed Errors

Returns a `validation_result` with all errors:

```cpp
auto result = obj.validate();  // or: dog_t.validate(dog)

result.ok();                   // true if no errors
result.error_count();          // number of errors
!result;                       // same as !result.ok() — operator bool

for (auto& error : result.errors()) {
    error.path;       // field name, e.g. "name"
    error.constraint; // validator name, e.g. "not_empty"
    error.message;    // human-readable message
}
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

Validators and `with<>` metas compose freely on the same field:

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

Validation recurses into nested type_defs with **dotted paths**:

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
result.validation_errors();        // [{path: "name", constraint: "not_empty", ...}]

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
