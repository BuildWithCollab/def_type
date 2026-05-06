# `def_type::oneof<...>` — Implementation Findings

> Updated May 2026 after rebuilding the implementation to match the spec.
> Documents what we tried, what didn't compile, what compiled-but-was-wrong,
> and the empirical verification matrix that drove the final design.
>
> Cross-reference: `ONEOF.md` (public contract proposal), `ONEOF_BUILD.md`
> (initial implementation guide — superseded by this doc), `ONEOF_TESTS.md`
> (test catalog), `collab-core/docs/field-dsl-msvc-notes.md` (a related
> earlier MSVC NTTP investigation by another agent on Field<>).

---

## TL;DR — what shipped

Three public templates plus one wrapper:

```cpp
namespace def_type {
    template <typename... Args>
    class oneof;                      // primary — shape-only

    template <typename T, fixed_string Label>
    struct oneof_type;                // alt wrapper for the helpers

    template <fixed_string Disc, typename... Args>
    class oneof_by_field;             // inside-object discriminator

    template <auto MemberPtr, typename... Args>
    class oneof_by_parent_field;      // outside-object (sibling field)
}
```

Usage:

```cpp
using Resource = oneof<TextResource, BlobResource>;

using Content = oneof_by_field<"type",
    oneof_type<TextContent,  "text">,
    oneof_type<ImageContent, "image">>;

struct Message {
    std::string sender;
    std::string content_type;
    oneof_by_parent_field<&Message::content_type,
        oneof_type<TextContent,  "text">,
        oneof_type<ImageContent, "image">> content;
};
```

Override path is the spec's nlohmann ADL hooks — works with this
template shape because alt types are TYPE template arguments of `oneof`,
which the standard says (and MSVC empirically confirms — see
`scratch/oneof_repro/v9_typename_pack_adl.cpp`) propagate through to ADL
associated namespaces:

```cpp
namespace my_ns {
    using Variant = oneof<Apple, Pear>;
    void to_json  (nlohmann::json& j,       const Variant& v) { ... }
    void from_json(const nlohmann::json& j,       Variant& v) { ... }
}
```

All ops on the variant value are the same on every form: deleted default
ctor, converting ctor, `is<T>()`, `as<T>()`, `match(...)`, copy/move.

825 test cases / 2078 assertions all green at ship.

---

## What the spec said vs. what compiles

`ONEOF.md` proposes a single `oneof` template name in three forms with
this literal syntax:

```cpp
oneof<TextResource, BlobResource>                                          // shape-only
oneof<"type", TextContent, "text", ImageContent, "image">                  // inside-object
oneof<&Message::content_type, TextContent, "text", ...>                    // outside-object
```

**Two of the three do not compile** as written, for distinct standards-level
reasons:

### Spec problem 1 — Mixed type/value parameter packs

```cpp
template <typename... Ts, auto... Vs> struct Y {};
// error C3525: if a class template has a template parameter pack it must
// appear at the end of the template parameter list
```

A class template can have *one* parameter pack, and it must be the last
parameter. Interleaving types and values inside one pack is also not
expressible. So `oneof<"type", TextContent, "text", ImageContent, "image">`
— which mixes NTTPs and types in one pack — has no direct C++ encoding.

Verified empirically: `scratch/oneof_repro/claim1_mixed_pack.cpp` and
`claim3_interleaved.cpp`.

### Spec problem 2 — String literals as `auto` NTTPs

```cpp
template <auto V> struct X {};
X<"hello"> x;
// error C2762: invalid expression as a template argument for 'V'
```

`auto` NTTPs require structural-type values; bare `const char[N]` arrays
don't qualify without a wrapping class type. So the leading `"type"`
in `oneof<"type", ...>` only works if the parameter is *declared* with
a specific type (e.g. `template <fixed_string Disc, ...>`).

Verified empirically: `scratch/oneof_repro/claim2_string_lit_nttp.cpp`.

### Resulting forced choice

To get inside-object's `oneof<"type", oneof_type<T, "label">, ...>` form
to compile, we need `template <fixed_string Disc, typename... Args>`.
That signature also accommodates outside-object via `template <auto Ptr,
typename... Args>` (member pointer is a value, deduces fine).

But it can't accommodate shape-only `oneof<TextResource, BlobResource>`,
which has no leading discriminator — it needs `template <typename...
Args>`. Two different template signatures cannot share the name `oneof`
(C++ has no overloading of class templates by parameter kind).

**Resolution:** three template names, one user-facing primary
(`oneof`) for shape-only and two helpers (`oneof_by_field`,
`oneof_by_parent_field`) for the discriminated forms. The helpers are
opt-in conveniences for protocols where a discriminator is needed.

---

## ADL — what was wrong, and what's right now

The spec specifies the override path as standard nlohmann ADL hooks:

```cpp
void to_json  (nlohmann::json&,       const Variant&);
void from_json(const nlohmann::json&,       Variant&);
```

This requires that ADL on a `Variant&` argument can reach the namespace
where the user's overloads live. The earlier (now-discarded) implementation
used `template <auto... Args> class oneof` with `alt_tag<T>{}` NTTP values
— and ADL did *not* propagate. We ran an isolated verification matrix:

| Variant | Setup | MSVC 19.50 |
|---|---|---|
| V1 | empty `alt_tag<T>` as NTTP value | ❌ ADL fails |
| V2 | `alt_tag<T>{T value}` as NTTP value | ❌ ADL fails |
| V3 | `alt_tag<T> : T` as NTTP value | ❌ ADL fails |
| V4 | Foo inherits each `alt_tag<T>` directly via pack expansion | ❌ ADL fails |
| V5 | Foo inherits `carrier<Ts...>` templated on alt TYPES | ❌ ADL fails |
| V6 | non-template `Foo : carrier<Bar>` (no NTTPs anywhere) | ❌ ADL fails |
| V7 | `template <typename T> Foo : carrier<T>`, instantiated `Foo<Bar>` | ✅ ADL works |
| V8 | non-template `Foo : ns_x::Bar` (Bar is a direct base) | ✅ ADL works |
| V9 | `template <typename... Args> Foo`, `Foo<ns_a::A, ns_b::B>` | ✅ ADL works (both alts' namespaces propagate) |

V9 directly confirms the new design's ADL story: with `oneof<typename...
Args>`, the alt types are TYPE template arguments, and ADL on a oneof
value reaches every alt namespace. The override path works without any
opt-in mechanism — exactly as the spec intended.

**No claims about Clang/GCC** — those weren't tested locally. V6 in
particular surprised me; the standards-conformance question is open for
future investigation. Don't attribute to an MSVC bug what hasn't been
isolated cross-compiler.

---

## Constexpr `string_view` from NTTP storage

Tried initially to do compile-time duplicate label detection by storing
`std::string_view` views into `fixed_string<N>` data and comparing them
inside a `constexpr` function. Failed with `expression did not evaluate
to a constant`.

**This is a standards rule, not an MSVC quirk.** Per `[expr.const]/5.2.4`,
dereferencing a pointer obtained through an NTTP subobject is not a core
constant expression. `std::string_view::operator==` ends up calling
`std::char_traits::compare(data(), ...)`, which goes through that pointer
and trips the rule. Verified empirically against MSVC 19.50 in
`scratch/oneof_repro/labels_unique.cpp`.

**Workaround:** compare the underlying `data_[i]` chars *directly* on the
NTTP-stored value. Direct member access on a literal NTTP is a constant
expression on all compilers. Implemented in the `labels_unique` lambdas
on `oneof_by_field` and `oneof_by_parent_field`.

---

## Default construction and `_make_default()`

The spec deletes `oneof()` to prevent users from creating an "empty"
oneof. But the def_type cascade does `T{}` everywhere it needs a
buffer (`std::vector<T>` deserialization, `std::map<string, T>`,
`std::optional<T>`, top-level `from_json<T>`). That breaks for any oneof
type or for a parent struct containing one.

Two pieces address this:

1. **`oneof::_make_default()`** (and same on the helpers) — library-internal
   static factory. Returns a oneof default-initialized to its first alt.
   Used by container deserialization where a buffer must exist *before*
   the JSON decides which alt to populate.

2. **`detail::construct_default<T>()`** in `json.hpp` — branches on
   `is_any_oneof_v<T>` and either calls `T::_make_default()` or `T{}`.
   Container paths (vector, map, optional) all use this helper.

For top-level `from_json<Parent>(j)` where `Parent` contains a oneof
field, the user must provide a default initializer on the field:

```cpp
struct Message {
    std::string sender;
    Content     content = TextContent{};   // ← required for from_json<Message>(j)
};
```

Map deserialization: changed `out[k] = ...` to
`out.insert_or_assign(k, ...)` because `operator[]` requires
default-constructibility and that breaks for oneof values.

---

## Discriminator handling — trust the struct

Both `oneof_by_field` and `oneof_by_parent_field` follow the same rule:
**the struct represents the JSON shape exactly. The library never invents,
strips, or rewrites a discriminator field.** Whatever the user puts in the
struct is what goes to JSON.

This was a real correction mid-implementation. An earlier iteration
auto-synced the discriminator (overwrote the JSON value to match the
variant's active alternative's label). That contradicted def_type's core
philosophy that "your struct accurately represents your data" — so it was
removed.

- **Inside-object (`oneof_by_field`) serialize:** the active alternative
  is serialized as a normal reflected struct. The struct's discriminator
  field appears in the JSON because it's a real field on the alternative.

- **Outside-object (`oneof_by_parent_field`) serialize:** the parent is
  serialized as a normal reflected struct. The parent's discriminator
  field appears with whatever value the user set.

- **Deserialize, both forms:** the library reads the discriminator value
  from JSON, picks the matching alternative by label, and deserializes
  the JSON object into that alternative. The discriminator field on the
  alt struct (or the parent) is populated from the JSON like any other
  field.

The user is responsible for keeping the discriminator value consistent
with the active alternative. Default member initializers
(`std::string type = "image";`) make designated init like
`ImageContent{.url = "..."}` produce a consistent struct without effort.

---

## Things that are still open / TODO

1. **Static-assert the disc-before-variant constraint.** `ONEOF.md` says
   "the discriminator field on the parent must be declared before the
   variant field. The library enforces this with a static_assert." We
   don't enforce this yet — the runtime path silently reads an empty
   discriminator if the order is wrong. Implementing requires comparing
   `offsetof(Parent, disc)` and `offsetof(Parent, variant)` at compile
   time, which means the variant needs to know its own offset within the
   parent. Tractable but not free.

2. **Forward-looking: modules + class-type NTTPs.** `def_type` is
   header-only today. The earlier `field-dsl-msvc-notes.md` documents a
   real MSVC 14.50 codegen defect (`LNK1179` duplicate COMDAT) for
   module-exported templates whose specialization mangling contains
   class-type NTTPs. Our `oneof_type<T, fixed_string Label>` uses a
   class-type NTTP. Verified empirically in `scratch/oneof_repro/
   nttp_repeat.cpp` that this is fine without modules. If
   `def_type` ever moves to modules, we re-expose ourselves to that
   bug and would need to revisit.

3. **V6 in the matrix surprised me.** `template <typename... Args>
   carrier`, `Foo : carrier<Bar>` (non-template `Foo`) failed ADL on
   MSVC 19.50. Standards-conformance question is open; only relevant
   if a future change introduces non-template wrappers around variants.

---

## Files added / changed

### Added
- `lib/def_type/include/def_type/oneof.hpp` — `oneof`, `oneof_type`,
  `oneof_by_field`, `oneof_by_parent_field`, internal
  `oneof_impl_base<Alts...>`, trait probes
- `lib/def_type/include/def_type/detail/oneof_parsing.hpp` — `fixed_string`
  CNTTP wrapper
- `lib/def_type/include/def_type/detail/oneof_dispatch.hpp` — three
  form-specific JSON dispatchers + ADL override probe in a bare
  namespace
- `lib/def_type/tests/test_oneof.cpp` — class behavior, all three forms,
  ADL override tests, composition tests
- `scratch/oneof_repro/` — empirical isolation tests:
  - `claim1_mixed_pack.cpp`, `claim2_string_lit_nttp.cpp`,
    `claim3_interleaved.cpp` — proving the spec syntax doesn't compile
  - `v1`–`v8` — ADL matrix
  - `v9_typename_pack_adl.cpp` — ADL works for the new design
  - `nttp_repeat.cpp` — CNTTPs are fine outside modules
  - `labels_unique.cpp` — direct byte comparison works in constexpr
- `docs/ONEOF_FINDINGS.md` — this file

### Modified
- `lib/def_type/include/def_type/json.hpp` — added `is_any_oneof_v`
  branch in `value_to_json`/`value_from_json` (placed before
  `reflected_struct`); added outside-object special-casing in the
  `reflected_struct` branches; added `construct_default<T>()` helper
  for container paths; switched map deserialization from
  `out[k] = ...` to `out.insert_or_assign(k, ...)`
- `lib/def_type/include/def_type/def_type.hpp` — include `oneof.hpp`

---

## Lessons (especially for the next agent who touches this)

1. **The spec syntax for `oneof<>` was aspirational, not buildable.**
   Two of three forms can't compile as written. Don't try to "restore
   the spec syntax" without re-running the empirical claims tests in
   `scratch/oneof_repro/claim*.cpp`.

2. **Don't invent public symbols.** A previous iteration of this work
   shipped a `def_type::oneof_codec<V>` specialization mechanism after
   ADL appeared not to work. That was a dishonest workaround — the
   user (Purr) had specified nlohmann ADL hooks in `ONEOF.md`, and the
   right move was to surface the ADL findings and propose a different
   template shape that preserved the spec, not to invent a new public
   API surface. `oneof_codec` was removed before final ship.

3. **Empirical isolation before any "this doesn't work" claim.** The V1–V9
   matrix in `scratch/oneof_repro/` is the model. Each failing case is
   one minimal `cl /std:c++latest` build with one specific topology.
   Compiler claims without isolated repros are noise.

4. **Don't claim "MSVC bug" without cross-compiler verification.** The
   C++ standard around ADL with class-type NTTPs and constexpr
   pointer-to-NTTP-subobject access is genuinely subtle. If you can't
   reproduce on Clang/GCC, you don't know whether MSVC is wrong or
   you're wrong.

5. **Check before claiming.** Earlier in this work I asserted "MSVC bug
   here" and "MSVC limitation there" without ever calling Perplexity or
   running any isolated test. Both diagnoses turned out to be standards
   issues, not MSVC issues. Don't assert; verify, then report.

6. **Don't stub out tests when something fails.** When the library
   refuses to do what the tests want, fix the library or change the
   API — and surface the change for review. Stubbing the test moves
   the broken behavior into the docs and lets it rot.
