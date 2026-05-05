# Proposal: `def_type::variant<Alts...>`

> Supersedes `POLYMORPHIC_TOOLS_PROPOSAL.md`. This proposal is about def_type's public API only — what the library adds, what it does, and what its users have to write. It does not prescribe how any particular consumer should model their domain.

## Summary

Add one class template to def_type: `def_type::variant<Alts...>` — a discriminated union of value types, with first-class JSON serialization that follows the same "free for the common case" contract as the rest of def_type.

## The class

```cpp
namespace def_type {
    template <typename... Alts>
    class variant {
    public:
        template <typename T> variant(T v);

        // Active-alternative queries
        template <typename T> T*       as();
        template <typename T> const T* as() const;
        template <typename T> bool     is() const;

        // Pattern match — overload set of lambdas, one per alternative
        template <typename... Fs> auto match(Fs... fs);
        template <typename... Fs> auto match(Fs... fs) const;
    };
}
```

That's the entire public surface of the type itself. No macros. No CNTTP. No required members on the alternative structs. No registration.

## Usage at a glance

```cpp
struct Dog { std::string name; int age; };
struct Cat { std::string name; bool indoor; };

using Pet = def_type::variant<Dog, Cat>;

Pet p = Dog{ .name = "Rex", .age = 3 };

if (p.is<Dog>())     /* ... */;
if (auto* d = p.as<Dog>()) /* ... */;

p.match(
    [](const Dog& d) { /* d.name, d.age */ },
    [](const Cat& c) { /* c.name, c.indoor */ }
);
```

Mutating arms get mutable references:

```cpp
p.match(
    [](Dog& d) { d.age += 1; },
    [](Cat& c) { c.indoor = true; }
);
```

A generic catch-all is allowed:

```cpp
p.match(
    [](const Dog& d)        { /* dog-specific */ },
    [](const auto& other)   { /* everything else */ }
);
```

## Serialization — always free

`def_type::to_json(v)` for any `variant` dispatches to the active alternative and runs the existing reflected-struct codec on it. The active alternative is known at runtime; there is no ambiguity to resolve.

```cpp
Pet p = Dog{ .name = "Rex", .age = 3 };
auto j = def_type::to_json(p);
// {"name": "Rex", "age": 3}
```

The user writes nothing. This works for any variant whose alternatives are types def_type already knows how to serialize (which, given def_type's reflection, is "any plain or `field<>`-using struct, plus the same set of primitives, containers, optionals, and enums supported elsewhere").

## Deserialization — free when the alternatives have distinct shapes

A new branch is added to def_type's `value_from_json` cascade for `variant<Alts...>`. The algorithm:

1. Take the set of keys present in the incoming JSON object.
2. For each alternative, ask: "are all of those JSON keys actual fields on this alternative's struct?" Call this *fitting*.
3. Count fitting alternatives.
4. **Exactly one fits** → deserialize the JSON into that alternative using the existing reflected-struct deserializer. Done.
5. **Zero fit** → throw `std::logic_error`, naming the JSON key that doesn't belong to any alternative.
6. **More than one fits** → throw `std::logic_error`, naming the alternatives that tied.

### Worked example — alternatives with distinct shapes

```cpp
struct TextResource { std::string uri; std::string mimeType; std::string text; };
struct BlobResource { std::string uri; std::string mimeType; std::string blob; };

using Resource = def_type::variant<TextResource, BlobResource>;

auto r = def_type::from_json<Resource>(nlohmann::json{
    {"uri", "file:///x.py"},
    {"mimeType", "text/x-python"},
    {"text", "print('hi')"}
});

r.is<TextResource>();   // true
r.as<TextResource>()->text;   // "print('hi')"
```

`TextResource` fits (all of `{uri, mimeType, text}` are fields on it). `BlobResource` does not fit (the JSON has `text`, which is not a field on `BlobResource`). Exactly one alternative fits → done. The user writes nothing.

The same variant round-trips through `to_json` losslessly.

### What "can't decide" actually means

Two cases, both with clear errors:

**Zero fits** — the JSON has an unrecognized key:

```cpp
def_type::from_json<Resource>(json{ {"uri", "..."}, {"weird", 1} });
// throws std::logic_error:
//   "from_json<Resource>: no alternative fits — JSON key 'weird'
//    is not a field on TextResource or BlobResource"
```

**Multiple fit** — the JSON's keys are a subset of more than one alternative's fields:

```cpp
struct A { std::string name; };
struct B { std::string name; int extra; };
using V = def_type::variant<A, B>;

def_type::from_json<V>(json{ {"name", "x"} });
// throws std::logic_error:
//   "from_json<V>: multiple alternatives fit (A, B) — cannot disambiguate"
```

No silent first-match-wins. No guessing.

## Override path — for variants the default can't decide

When the default algorithm cannot pick an alternative — most commonly when the discriminator is a tag that lives outside the alternative's own field set, or when alternatives share a shape — the user provides a normal nlohmann ADL overload. def_type's variant branch checks for it via a `requires` expression and uses it if present.

```cpp
struct Click { int x; int y; };
struct Drag  { int x; int y; int dx; int dy; };
struct Wheel { int x; int y; int delta; };

using MouseEvent = def_type::variant<Click, Drag, Wheel>;

void from_json(const nlohmann::json& j, MouseEvent& e) {
    auto type = j.at("type").get<std::string>();
    if (type == "click")
        e = Click{ .x = j.at("x"), .y = j.at("y") };
    else if (type == "drag")
        e = Drag{ .x = j.at("x"), .y = j.at("y"),
                  .dx = j.at("dx"), .dy = j.at("dy") };
    else if (type == "wheel")
        e = Wheel{ .x = j.at("x"), .y = j.at("y"),
                   .delta = j.at("delta") };
    else
        throw std::runtime_error("unknown mouse event: " + type);
}
```

For tag-based wire formats, `to_json` typically gets a matching overload too, since the default serializer would emit the alternative's fields without the discriminator tag:

```cpp
void to_json(nlohmann::json& j, const MouseEvent& e) {
    e.match(
        [&](const Click& c) { j = {{"type","click"}, {"x",c.x}, {"y",c.y}}; },
        [&](const Drag&  d) { j = {{"type","drag"},  {"x",d.x}, {"y",d.y},
                                    {"dx",d.dx}, {"dy",d.dy}}; },
        [&](const Wheel& w) { j = {{"type","wheel"}, {"x",w.x}, {"y",w.y},
                                    {"delta",w.delta}}; }
    );
}
```

This is the same nlohmann extension idiom anyone using nlohmann directly already knows. It is not a def_type-specific mechanism.

## What this adds to def_type's public API

1. `def_type::variant<Alts...>` — the class above, with `as<>`, `is<>`, `match`, implicit construction.
2. A `value_to_json` branch for `variant<Alts...>` that dispatches on the active alternative.
3. A `value_from_json` branch for `variant<Alts...>` that runs the fitting algorithm, defers to a user `from_json(const nlohmann::json&, V&)` overload if one exists, and throws structured errors for the zero-fit and multi-fit cases.
4. Two new exception messages — "no alternative fits" and "multiple alternatives fit" — both `std::logic_error`.

That is the complete diff to the public API.

## What this does NOT add

- No new free functions (`pick<>`, `select_*`, `accepts<>`, etc.). The override path uses nlohmann's existing ADL pattern.
- No new traits or specialization chains for users to fill in. No CNTTP. No required member functions on alternative structs.
- No new wire-format vocabulary in def_type itself. There is no `"$type"` key, no built-in tag scheme, no opinion about how user JSON should be shaped. Variants whose alternatives have distinct field shapes just work; variants whose alternatives don't, the user owns the wire format via the overload.
- No transport, dispatch, or framework-level concerns. `variant<>` is a value type. What you put in it and how you wire it through your application is not def_type's business.

## Compatibility with existing def_type features

A `variant<...>` value can be used:

- As a plain struct member, alongside any other field types.
- Inside a `field<variant<...>>` if the user wants validators or `with<>` metas attached to it.
- As an alternative inside another variant (nested variants).
- As the value type of a container (`std::vector<variant<...>>`, `std::map<std::string, variant<...>>`, etc.) — the existing container codecs handle it.
- Inside `std::optional<variant<...>>` — the existing optional codec handles it.

No special opt-in is required for any of these compositions. They fall out of the same cascade as everything else.

## Out of scope

- Runtime registration of new alternatives. The alternative list is fixed at the point `using V = def_type::variant<...>` is written.
- A built-in tag-based default wire format. Variants with overlapping shapes require a user overload; def_type does not invent a discriminator.
- Changes to any other part of def_type. Nothing in `type_def`, `field<>`, `meta<>`, validation, or `parse` needs to change to support this.
