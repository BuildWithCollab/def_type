# Building `def_type::oneof<...>`

Implementation guide. Pair with `ONEOF.md` (the public contract) and `ONEOF_TESTS.md` (the test catalog). If anything in this document conflicts with `ONEOF.md`, treat `ONEOF.md` as the source of truth and stop to ask.

---

## Scope of changes

A new feature added alongside everything else in def_type. Nothing in `type_def`, `field<>`, `meta<>`, `validation`, `parse`, or any of the existing reflection machinery needs to change to support `oneof<...>`. The integration is additive:

- One new header (`include/def_type/oneof.hpp`).
- Two new branches in the existing `value_to_json` / `value_from_json` cascade (`include/def_type/json.hpp`).
- One new include in the umbrella `def_type.hpp`.

If you find yourself editing `typed_type_def.hpp`, `dynamic_type_def.hpp`, `field.hpp`, `meta.hpp`, `validation.hpp`, or `parse.hpp` to make `oneof<...>` work, stop. The design intends `oneof<...>` to be reachable purely through the existing recursive descent in `value_to_json` / `value_from_json`.

---

## File layout

```
lib/def_type/include/def_type/
    oneof.hpp                 ← new
    json.hpp                  ← edit: two new branches
    def_type.hpp              ← edit: include oneof.hpp
    detail/
        oneof_parsing.hpp     ← new (compile-time alternation parsing)
        oneof_dispatch.hpp    ← new (compile-time alt → label map and dispatch)

lib/def_type/tests/
    test_oneof.cpp            ← new (class behavior, simple JSON)
    test_oneof_json.cpp       ← new (JSON shapes per form)
    test_oneof_composition.cpp ← new (variant inside field<>, containers, etc.)
```

Split rationale: `oneof.hpp` is the public-facing class; `detail/oneof_parsing.hpp` and `detail/oneof_dispatch.hpp` keep the constexpr machinery out of the public header. Tests are split for the same reason `test_field.cpp` and `test_field_json.cpp` are split — class behavior is one concern, JSON shape coverage is another.

---

## The class

```cpp
namespace def_type {
    namespace detail {
        // Overload helper for std::visit
        template <class... Ts> struct overload : Ts... { using Ts::operator()...; };
        template <class... Ts> overload(Ts...) -> overload<Ts...>;

        template <typename T> inline constexpr bool is_oneof_v = false;
        template <auto... Args> inline constexpr bool is_oneof_v<oneof<Args...>> = true;
    }

    template <auto... Args>
    class oneof {
        // Args are parsed by detail::oneof_parsing into:
        //   - form: shape_only | inside_object | outside_object
        //   - discriminator key (string) for inside-object
        //   - discriminator member pointer for outside-object
        //   - alts: tuple of types
        //   - labels: tuple of fixed_strings (empty for shape-only)

        using Parsed = detail::parsed_args<Args...>;
        using Alts   = typename Parsed::alts;     // std::tuple<TextContent, ImageContent, ...>
        std::variant<...flatten Alts...> v_;

    public:
        oneof() = delete;

        template <typename T>
            requires Parsed::template is_alt<std::remove_cvref_t<T>>
        oneof(T&& val) : v_(std::forward<T>(val)) {}

        template <typename T>
            requires Parsed::template is_alt<T>
        T*       as()       { return std::get_if<T>(&v_); }
        template <typename T>
            requires Parsed::template is_alt<T>
        const T* as() const { return std::get_if<T>(&v_); }
        template <typename T>
            requires Parsed::template is_alt<T>
        bool     is() const { return std::holds_alternative<T>(v_); }

        template <typename... Fs>
        auto match(Fs&&... fs) {
            return std::visit(detail::overload{std::forward<Fs>(fs)...}, v_);
        }
        template <typename... Fs>
        auto match(Fs&&... fs) const {
            return std::visit(detail::overload{std::forward<Fs>(fs)...}, v_);
        }
    };
}
```

Notes:

- `auto... Args` is intentional. The first arg may be a CNTTP `fixed_string`, a member pointer, or absent (just types follow). The parsing helper resolves which form was meant.
- `oneof()` is `= delete`d. No default construction. `std::vector<oneof<...>>::resize(n)` will not compile — that's intentional. Users wrap in `std::optional<oneof<...>>` if "absent" is meaningful.
- `std::variant` is private; never expose it. `match` is the only visit primitive.
- `as<T>` / `is<T>` are constrained by `requires Parsed::template is_alt<T>` so calling `as<UnrelatedType>()` is a compile error, not a silent `nullptr`.

---

## Parsing the template arguments

`detail::parsed_args<Args...>` decides which form the user requested. The first argument (if any) determines the form:

| First arg type | Form | Parsing |
|---|---|---|
| (nothing — only types follow) | shape-only | All args are types; no labels. |
| `fixed_string<N>` | inside-object | First arg is the discriminator key. Remaining args alternate `(Type, fixed_string)`. |
| Member pointer (`T C::*`) | outside-object | First arg is the discriminator member pointer; remaining args alternate `(Type, fixed_string)`. |

The alternation parser (`detail/oneof_parsing.hpp`) takes the tail and produces:

- `using alts = std::tuple<...>;` — the alt types in declaration order
- `static constexpr std::array labels = { ... };` — parallel array of `string_view` (or `fixed_string`) — labels in declaration order, indexed identically to `alts`
- `template <typename T> static constexpr bool is_alt = ...;` — concept helper
- `template <typename T> static constexpr std::size_t index_of = ...;` — alt → index
- `static constexpr std::size_t alt_for_label(std::string_view) noexcept;` — runtime lookup, returns `npos` on miss

For shape-only, `labels` is empty and `alt_for_label` is unused.

The parsing helper enforces the following at compile time:

- All alts are reflected structs (so `value_to_json` / `value_from_json` knows how to handle them) **OR** the user has supplied a `from_json` / `to_json` ADL overload (detected via `requires`). If neither, `static_assert` with a message naming the offending alt.
- For inside-object and outside-object: the alt list and label list have equal length (i.e. the alternation is well-formed pairs).
- For inside-object and outside-object: every label is unique within the `oneof`. Duplicate label → `static_assert` naming the duplicated string.
- For outside-object: the member pointer's pointee type is `std::string` (or convertible to one in a way the codepath supports). Anything else → `static_assert`.

---

## Cascade integration in `json.hpp`

Add two `else if constexpr (is_oneof_v<T>)` branches to `detail::value_to_json` and `detail::value_from_json`. Place them **before** the `reflected_struct` branch — `oneof` itself is not a reflected struct; this avoids the wrong path being taken.

### `value_to_json`

```cpp
} else if constexpr (is_oneof_v<T>) {
    // 1. ADL override always wins.
    if constexpr (requires(nlohmann::json& jj, const T& vv) { to_json(jj, vv); }) {
        nlohmann::json j;
        to_json(j, v);   // unqualified — ADL
        return j;
    }
    // 2. Default behavior depends on form.
    return oneof_to_json(v);   // form-dispatched
}
```

`oneof_to_json` lives in `detail/oneof_dispatch.hpp` and switches on `T::Parsed::form`:

- **shape-only** — `return v.match([](const auto& alt){ return value_to_json(alt); });`
- **inside-object** — `match` on the active alt, call `value_to_json` to get its JSON, then inject `j[key] = labels[index_of_active]`. Key is the CNTTP carried in `T::Parsed`.
- **outside-object** — same as shape-only on the variant alone (just emit the active alt's JSON). The parent struct's serialization writes the discriminator field separately as a normal string field; the variant doesn't see the parent's JSON. The variant serializer **does** mutate the parent's discriminator field before the parent gets to it (see "Outside-object serialization" below).

### `value_from_json`

```cpp
} else if constexpr (is_oneof_v<T>) {
    if constexpr (requires(const nlohmann::json& jj, T& vv) { from_json(jj, vv); }) {
        from_json(j, out);   // unqualified — ADL
        return;
    }
    oneof_from_json(j, out);   // form-dispatched
}
```

`oneof_from_json` switches on form:

- **shape-only** — collect JSON keys, run the fitting algorithm exactly as documented in `ONEOF.md`. Throw `std::logic_error` for zero-fit / multi-fit.
- **inside-object** — read `j.at(key).get<std::string>()`. Look up the alt index via `Parsed::alt_for_label`. If `npos`, throw `std::logic_error` listing known labels. Otherwise, deserialize the JSON into the chosen alt with `value_from_json`. Skip the discriminator key during alt deserialization (since the alt struct does not declare it; the existing reflected-struct deserializer already ignores keys that aren't fields).
- **outside-object** — see below.

### Outside-object serialization — the only awkward part

The variant lives as a field on a parent struct. When the parent's `to_json` runs, it visits fields in declaration order. The discriminator field comes first (constraint enforced by static_assert — see below), so it has *some* value already (whatever the user set or default-initialized). When the variant field is reached, `oneof_to_json` does **not** emit the discriminator key into the variant's own JSON (the discriminator lives outside the variant's nested object). It does, however, need the parent's discriminator field to reflect the active alt's label.

Two viable strategies:

**Strategy A: variant's serializer writes through the member pointer.** When the variant is being serialized, before the alt's JSON is emitted, the variant looks up its annotation (`&Parent::discriminator_field`) and writes the active alt's label into the parent. The parent then emits that string normally when it reaches the discriminator field. Problem: the variant only has access to *itself* during `value_to_json`, not the parent.

**Strategy B: parent struct's reflection handles the linkage.** When the parent's reflection visits a field of type `oneof<&Parent::field, ...>`, it special-cases: it looks at the oneof's annotation, fetches the active alt's label, and overwrites the parent's discriminator field *before* serialization continues. This puts the linkage logic on the parent's serialization side.

Strategy B is the implementation. The hook point is in the typed-path serialization in `typed_type_def.hpp` — wait, the doc says "no edits to typed_type_def.hpp." Reconsider: actually the variant's `value_to_json` *does* receive a reference (or value) of just itself, and the parent's serializer iterates fields independently. The simplest implementation is:

**Strategy C: variant is read-only at JSON time, parent writes the discriminator field naively.** The user is responsible for keeping `parent.content_type` in sync with `parent.content`. The library does not auto-fix it. The output JSON contains whatever the user put in `parent.content_type`. If they put garbage, garbage round-trips.

This is the simplest implementation and the one to ship. It also means: outside-object serialization is **just** the parent struct's normal field-by-field reflection, with the variant emitting the active alt's JSON exactly as in shape-only mode. The "linkage" only matters on deserialize.

Document this clearly in `ONEOF.md` under outside-object: "The user is responsible for assigning a consistent value to the discriminator field whenever they assign the variant. The library does not synchronize them on serialize." If this turns out to be a real ergonomic problem in practice, revisit with Strategy B as a follow-up.

> **Note:** `ONEOF.md` currently says the library overwrites `m.content_type` with the truth before serialization. Update `ONEOF.md` to match Strategy C, OR implement Strategy B (which requires a hook in the parent's reflected-struct serializer in `value_to_json`'s `reflected_struct` branch — examine the field type, if it's a `oneof` with an outside-object annotation, use the variant's active label to set the linked field before continuing). Pick one and make the docs match.

### Outside-object deserialization

Fields are visited in declaration order by the existing reflected-struct deserializer. The discriminator field is read first as a normal `std::string`. When the variant field is reached, `oneof_from_json` is given the JSON value for that field's nested object — but it also needs the already-deserialized discriminator value from the parent. There are a few ways to wire this:

**Approach 1 — second pass.** Have the parent's deserializer pass the full parent JSON and the partially-built parent struct to the variant's deserializer. The variant looks up `parent.content_type` (the deserialized value) and uses it.

**Approach 2 — annotation-driven sibling lookup.** The variant's deserializer accesses the partially-built parent struct via the member pointer it was annotated with. This requires the variant's deserializer signature to include a parent-struct reference, which the existing cascade does not have.

**Approach 3 — read sibling JSON directly.** The variant's deserializer takes the parent JSON object (not just the variant's own subtree) and reads the discriminator key from it. The annotation tells the variant *which JSON key* to read. This requires the parent's deserializer to call into the variant's deserializer differently than it calls into other fields.

Approach 3 is the cleanest. Implement it as: the reflected-struct deserializer in `value_from_json`, when it encounters a field of type `oneof<&Parent::field, ...>`, does **not** do the usual `value_from_json(j[field_name], target)` call. Instead, it dispatches to `oneof_from_json_outside(j_parent, target, parent_discriminator_field_name)` where `j_parent` is the entire parent JSON object. The variant uses `j_parent.at(parent_discriminator_field_name)` to read the discriminator value, then `j_parent.at(variant_field_name)` for the alt's nested object.

This is the one place where the reflected-struct deserializer has to know about `oneof` — and only specifically for the outside-object form. It's a small, contained special case.

If the discriminator value doesn't match any label → `std::logic_error` listing known labels.

---

## Static_asserts

Enforce at compile time:

- All alts in a oneof are reflected structs OR a user ADL `from_json` / `to_json` is present. (Detect ADL with `requires`.)
- Inside-object / outside-object: alternation is well-formed `(Type, fixed_string)` pairs.
- Inside-object / outside-object: labels are unique within the oneof.
- Outside-object: member pointer's class is the parent struct (concrete check at use site, where the variant field's declaration sees the enclosing class).
- Outside-object: member pointer's pointee type is `std::string`.
- Outside-object: the discriminator field is declared **before** the variant field in the parent struct.
  - Detection: compare member offsets — `offsetof(Parent, discriminator_field) < offsetof(Parent, variant_field)`. Both are constexpr in this context (with PFR's offset machinery — see `detail/pfr_backend.hpp` for what's already available).

---

## Override detection

The override-wins-always rule applies to all three forms. Detection is via `requires` expressions on the unqualified two-arg `from_json` / `to_json` shapes:

```cpp
template <typename V>
concept has_user_from_json = requires (const nlohmann::json& j, V& v) {
    from_json(j, v);
};

template <typename V>
concept has_user_to_json = requires (nlohmann::json& j, const V& v) {
    to_json(j, v);
};
```

Inside the `is_oneof_v<T>` branches, check the corresponding concept first; if true, defer to ADL and return.

The single-arg `def_type::from_json<T>(const json&)` does not match either probe (different signature), so there's no risk of the library's own template being mistaken for a user override.

---

## Compile-time alternation parser

The alternation parser is the most fiddly piece. Given variadic `auto... Args` with a known prefix (CNTTP string or member pointer or nothing) and a tail of `(Type, fixed_string)` pairs, it must:

1. Detect the prefix shape and route to the correct form.
2. For inside-object and outside-object: peel pairs from the tail, emitting `std::tuple<Types...>` for the alt list and `std::array<fixed_string, N>` for the labels.

C++23's parameter pack expansion plus `std::index_sequence` is enough. Sketch:

```cpp
template <auto First, auto... Rest>
struct prefix_dispatch {
    using First_t = decltype(First);
    static constexpr bool is_cntp_string = is_fixed_string_v<First_t>;
    static constexpr bool is_member_ptr  = std::is_member_pointer_v<First_t>;

    using parsed = std::conditional_t<is_cntp_string, parse_inside<First, Rest...>,
                   std::conditional_t<is_member_ptr,  parse_outside<First, Rest...>,
                                                       parse_shape_only<First, Rest...>>>;
};
```

For pair extraction, an even/odd index trick:

```cpp
template <auto... Pairs>
struct pair_extract {
    template <std::size_t... I>
    static auto types_impl(std::index_sequence<I...>) {
        return std::tuple<get_arg_t<2*I, Pairs...>...>{};
    }
    template <std::size_t... I>
    static constexpr auto labels_impl(std::index_sequence<I...>) {
        return std::array{ get_arg<2*I + 1, Pairs...>()... };
    }
    using types = decltype(types_impl(std::make_index_sequence<sizeof...(Pairs)/2>{}));
    static constexpr auto labels = labels_impl(std::make_index_sequence<sizeof...(Pairs)/2>{});
};
```

A `fixed_string<N>` literal class needs to exist somewhere in `def_type::detail` — write it minimally; do not import a third-party one. Constructor takes `const char(&)[N]`, stores in a `std::array<char, N>`. Equality and conversion to `std::string_view` are the only operations needed by the variant code.

---

## Build / run

The repository uses xmake. After adding the new headers and tests, the standard cycle is:

```bash
xmake f -m release -c -y
xmake build -a
xmake run tests-def_type
```

Run that loop after every meaningful change. Do not declare success without all green.

---

## Non-goals — do not do these

These were considered and rejected. If a future agent thinks one of them is a clever improvement, it isn't.

- **Do not auto-derive labels from C++ struct names.** Labels are stated explicitly in the `oneof<...>` declaration. No `nameof_short_type<T>()` magic, no `snake_case` transform.
- **Do not infer the discriminator field by structural pattern matching across alts** ("the field name common to all alts with distinct string defaults"). Inference is guessing; the library does not guess.
- **Do not introduce a per-alt wrapper type like `def_type::named<T, "label">` or `def_type::alt<T, "label">`.** Such wrappers create a CNTTP string that can multiply across the codebase — they were tried in earlier iterations and rejected. Labels live only on the single `oneof<...>` declaration line, via the inline alternation.
- **Do not allow default construction** of `oneof<...>`. `oneof()` is `= delete`d. The library will not pick a "first" alt for the user.
- **Do not expose `std::variant`** through the public API of `oneof`. No `_underlying()`, no friend hatches, no `get_if<>` overloads. `match` is the visit primitive.
- **Do not add macros** anywhere in this feature.
- **Do not introduce a runtime registry** (no `inline const auto _setup = ...` patterns, no `std::type_index` maps).
- **Do not require alts to inherit from a common base** or implement any virtual interface.
- **Do not couple `oneof<...>` to a specific MCP / RPC / API protocol.** Keep examples and types neutral (Dog/Cat, TextContent/ImageContent style).
- **Do not invent a discriminator key name on the user's behalf** (no default `"type"` / `"$type"` / `"kind"`). The user states the key in the `oneof<...>` declaration.
- **Do not add an override-detection helper for partial overrides** (e.g. user provides `from_json` only). If a user provides one ADL function, they own the round-trip and must provide both. Document this; do not work around it.

---

## Where to look in the existing code

- `lib/def_type/include/def_type/json.hpp` — the value-level cascade you'll add branches to. Read the existing branches first to match their idiom.
- `lib/def_type/include/def_type/typed_type_def.hpp` — for understanding how the reflected-struct path works; the outside-object deserialize hook plugs into the reflected-struct branch in `value_from_json`, not into this header directly.
- `lib/def_type/include/def_type/detail/discovery.hpp` — `reflected_struct` concept, `field_names<T>()`, default-value retrieval.
- `lib/def_type/include/def_type/detail/pfr_backend.hpp` — PFR-backed offset / member access helpers if you need them for the offset-comparison static_assert in outside-object form.
- `lib/def_type/tests/test_from_json.cpp`, `test_to_json.cpp`, `test_field_json.cpp` — existing test idioms for `Catch2`. Match their style when writing new tests.
