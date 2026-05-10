# def_type — feedback from a real consumer

> **Source:** [`acp-cpp`](https://github.com/BuildWithCollab/acp-cpp) — a C++23 SDK for the [Agent Client Protocol](https://agentclientprotocol.com/). The SDK uses `def_type` as its protocol-types layer (~80 schema types, several polymorphic unions, `_meta` extension fields on every type).
>
> **Version exercised:** `def_type 1.3.2` (xmake registry, BuildWithCollab).
>
> **Scope of this feedback:** what came up while building two vertical slices of acp-cpp:
> - **Slice 1** ("hello agent"): hand-rolled subset of ~10 protocol types, JSON round-trip tests, in-memory + real-stdio integration tests.
> - **Slice 2** ("fs/read_text_file back-channel"): added 3 more types, restored capability fields, exercised polymorphic carriers more deeply.
>
> Total of 8 module partitions defining ~12 reflected structs + 2 `oneof_by_field<>` polymorphic unions, plus ~95 round-trip and back-channel tests across the suite. All green at time of writing.
>
> **What this feedback is not:** a comprehensive review. ACP++ exercises a specific subset of `def_type`. See *Coverage caveat* below for what wasn't tested.

---

## TL;DR — priority list

If I were ranking by ACP impact × fix size:

| # | Issue | Severity | Fix size | ACP impact |
|---|---|---|---|---|
| **1** | `std::optional<T>` empty emits `null` instead of omitting the JSON key | High | ~5 LOC | **Massive** — affects ~every protocol type |
| **2** | Empty structs fail `reflected_struct` and trigger a wall of MSVC template errors | Medium | ~3 LOC or doc | Medium — recurring modeling pattern |
| **3** | `def_type::to_json` / `from_json` reject top-level `oneof` types | Low–Med | ~10 LOC | Test ergonomics |
| **4** | Diagnostic messages on constraint failures are dense MSVC template walls | Low | ~3 `static_assert`s | Saves debug time per hit |
| **5** | Documentation gaps around optional-on-the-wire, empty-struct constraint, transitive oneof default-init | Doc-only | — | Onboarding |

**#1 is the standout.** It's a one-line behavioral change inside the `value_to_json` reflected-struct loop and it unblocks `_meta` round-trip preservation across the entire SDK.

---

## Issue 1 — `std::optional<T>` empty emits `null` instead of omitting the key

### Severity: High. ACP-blocking until worked around.

### Repro

```cpp
struct ErrorEnvelope {
    int                              code;
    std::string                      message;
    std::optional<def_type::unknown> data;   // optional per JSON-RPC 2.0
};

ErrorEnvelope e{.code = -32603, .message = "Internal error"};
nlohmann::json j = def_type::to_json(e);
```

### Expected

```json
{"code": -32603, "message": "Internal error"}
```
The `data` key is **absent** because the field is empty.

### Actual

```json
{"code": -32603, "message": "Internal error", "data": null}
```
The `data` key is present with a `null` value.

### Why this matters

For most protocols, **absent ≠ null**:

- **JSON-RPC 2.0** says `error.data` is *optional* — meaning the field SHOULD be absent from the wire, not present-but-null. (acp-cpp's `JsonRpcError` already wraps `data` in `optional<unknown>` and the connection module's comment explicitly calls this out: *"the field should be absent from the wire JSON when not provided, not present-but-empty."*)
- **ACP** has `_meta: object | null` on every protocol type for vendor extensions. `_meta: null` everywhere is noise, may break stricter validators on the peer side, and conflates "I have no extension data" with "I'm explicitly signalling null."
- Any **OpenAPI / JSON Schema** that uses `required` for required fields: optional fields that emit null when empty trip up validators that expected the field to be omitted.

### Root cause

Two-pronged interaction inside `lib/def_type/include/def_type/json.hpp`:

```cpp
// json.hpp:158-160 — value_to_json for optional
if constexpr (is_optional_v<T>) {
    if (v.has_value()) return value_to_json(*v);
    return nlohmann::json(nullptr);   // ← always returns null for empty
}
```

…combined with the reflected-struct iteration at `json.hpp:173-178` that **unconditionally assigns** the produced JSON to the field's key:

```cpp
} else if constexpr (reflected_struct<T>) {
    nlohmann::json j = nlohmann::json::object();
    type_def<T>{}.for_each_field(v, [&](std::string_view name, const auto& value) {
        j[std::string(name)] = value_to_json(value);   // ← always sets the key
    });
    return j;
}
```

So an empty optional becomes `null`, which then gets written to the JSON object under its key.

The reverse direction (`from_json`) handles all three cases correctly already: absent key → `nullopt`, key present and null → `nullopt`, key present and value → set. Only the to-JSON direction is asymmetric.

### Suggested fix

Special-case `is_optional_v` *during the reflected-struct iteration*:

```cpp
} else if constexpr (reflected_struct<T>) {
    nlohmann::json j = nlohmann::json::object();
    type_def<T>{}.for_each_field(v, [&](std::string_view name, const auto& value) {
        using V = std::remove_cvref_t<decltype(value)>;
        if constexpr (is_optional_v<V>) {
            if (value.has_value())
                j[std::string(name)] = value_to_json(*value);
            // else: leave the key absent
        } else {
            j[std::string(name)] = value_to_json(value);
        }
    });
    return j;
}
```

This keeps `value_to_json(optional)` itself unchanged for the *standalone* case (where someone explicitly serializes an `optional<T>` and reasonably expects `null` for empty); only the *struct-field* case changes its behavior to omit-when-empty, which is what every protocol I'm aware of actually wants.

### If you want the old behavior reachable

Two options for the rare protocol that does want an explicit `null` on the wire:

- **field annotation:** `field<std::optional<T>, with<emit_null_if_empty>>`. Per-field, surgical.
- **type-level opt-in:** a new wrapper `def_type::nullable<T>` whose `value_to_json` always returns `null` for empty. Same semantics as `std::optional` but signals intent.

Either is fine; I'd default to the per-field annotation if anyone needs it. The default should be omit.

### Workaround currently in acp-cpp

`_meta` was dropped from every type entirely. Slice 1 also dropped `clientCapabilities` from `InitializeRequest` because the empty cap struct made it impossible to even build (see Issue 2), and reintroducing `optional<ClientCapabilities>` would have produced `"clientCapabilities": null` on every handshake.

The downstream cost: ACP's `_meta` extension carrier is silently dropped on round-trip across the whole SDK. Any vendor-supplied `_meta` data on inbound messages gets lost on re-serialize. This is acknowledged as a known forward-compat hole in `docs/VERTICAL_SLICE_HELLO_AGENT_RESULTS.md` of acp-cpp.

---

## Issue 2 — Empty structs fail `reflected_struct` with a wall of template errors

### Severity: Medium. Hit twice in slice 1 alone (placeholder caps + placeholder `McpServer`).

### Repro

```cpp
struct ClientCapabilities {};   // empty placeholder; will grow over versions

struct InitializeRequest {
    int                protocolVersion = 1;
    ClientCapabilities clientCapabilities;
};

def_type::to_json(InitializeRequest{});  // ❌ won't compile
```

### Expected

Either:
- `{"protocolVersion": 1, "clientCapabilities": {}}` — empty struct round-trips as `{}`, OR
- A clear, top-of-error-output `static_assert` saying "def_type cannot reflect empty structs; add at least one member or write nlohmann ADL hooks."

### Actual

A 30+-line MSVC template instantiation chain ending with:

```
the concept 'def_type::detail::reflected_struct<acp::ClientCapabilities>' evaluated to false
...
note: see reference to function template instantiation '...value_to_json<ClientCapabilities>' being compiled
note: 'nlohmann::json_abi_v3_12_0::basic_json<...>::basic_json':
      no overloaded function could convert all the argument types
```

The actual problem (`ClientCapabilities` is empty, PFR can't reflect it) is not visible anywhere in the diagnostic output. I had to read `value_to_json`'s `if constexpr` chain to figure it out.

### Why this matters

- **Modeling pattern:** Many schema types start out empty and grow over versions. ACP has at least three:
  - `AgentCapabilities` — empty until the spec adds agent-side caps.
  - `EmbeddedResourceResource` carrier types in some scenarios.
  - Placeholder element types for `vector<T>`-typed fields when the slice doesn't populate them (acp-cpp's `McpServer` is currently `struct McpServer { std::string name; }` *purely* because PFR refused an empty version, and the slice never actually constructs one).
- **Discoverability:** the README's "PFR vs Manual Registration" section discusses what PFR does, but never mentions the "≥1 member" constraint explicitly. New users will hit this without warning.

### Root cause

`json.hpp:156-189`'s `value_to_json` chain has no branch for "aggregate but empty." It falls through every check and lands at `json.hpp:187`:

```cpp
} else {
    return nlohmann::json(v);   // ← line 187, no nlohmann conversion exists for empty struct
}
```

`reflected_struct<T>` (in `reflect.hpp`) presumably checks something like `field_count > 0`, which is false for empty aggregates.

### Suggested fix (in priority order)

**Option A — handle empty aggregates as `{}`:** add a branch *before* the `reflected_struct` branch in `value_to_json`:

```cpp
} else if constexpr (std::is_aggregate_v<T> && std::is_empty_v<T>) {
    return nlohmann::json::object();
} else if constexpr (reflected_struct<T>) {
    // existing reflected-struct branch
}
```

…and the symmetric for `value_from_json`: if `T` is an empty aggregate and the JSON is an object, default-construct `T` and ignore extra keys.

This makes `struct Foo {}` round-trip as `{}`, which is the right behavior for "I have a placeholder I'll grow later."

**Option B — friendlier diagnostic only:** wrap the public `to_json`/`from_json` entry points with a clarifying `static_assert`:

```cpp
template <typename T>
nlohmann::json to_json(const T& obj) {
    static_assert(detail::reflected_struct<T>,
        "def_type::to_json requires T to be a reflected aggregate "
        "with at least one field. Common causes: T is empty (PFR cannot "
        "reflect zero-field aggregates), or T is a oneof (use it inside "
        "a carrier struct, see README §oneof).");
    return detail::value_to_json(obj);
}
```

Option A is better because it's the right behavior; Option B is a consolation prize if implementing A is awkward.

### Workaround currently in acp-cpp

- `ClientCapabilities` was dropped from `InitializeRequest` in slice 1 entirely; restored in slice 2 only when it acquired a real `fs` field.
- `AgentCapabilities` is still defined as `struct AgentCapabilities {};` but is **never serialized** anywhere — slice 1+2's `InitializeResponse` carries no caps field.
- `McpServer` was given a placeholder `std::string name` field purely to pass PFR. The slice never sets it.

---

## Issue 3 — `def_type::to_json` / `from_json` reject top-level `oneof` types

### Severity: Low to medium. Affects test ergonomics primarily.

### Repro

```cpp
using ContentBlock = def_type::oneof_by_field<
    "type",
    def_type::oneof_type<TextContentBlock, "text">>;

ContentBlock block = TextContentBlock{.text = "hi"};
nlohmann::json j = def_type::to_json(block);   // ❌ constraint failure
auto round = def_type::from_json<ContentBlock>(j);   // ❌ constraint failure
```

### Expected

The oneof serializes/deserializes through its registered dispatch (the same one that fires when it's a *field* of a reflected struct).

### Actual

`to_json` is declared with the constraint `template <detail::reflected_struct T>` (`json.hpp:298`), and `from_json` likewise (`json.hpp:311`). A `oneof` is a variant, not a reflected struct, so neither overload matches. MSVC reports a no-matching-function error.

The internal machinery to handle this *exists* — `value_to_json` already has an `is_any_oneof_v` branch (`json.hpp:169-170`) that delegates to `oneof_dispatch_to_json`. It's just not reachable from the public API.

### Why this matters

- **Unit testing isolated polymorphic round-trips.** Wanting to write "this exact JSON parses to the right `oneof` variant; this oneof serializes to this exact JSON" is a normal test shape. Right now it requires either:
  - Wrapping the oneof in a carrier struct just for the test (acp-cpp ended up doing this — `SessionNotification` carries `SessionUpdate` so the test exercises both at once, but that's noise in the test).
  - Reaching into `def_type::detail::value_to_json` directly (works, but `detail` is a hint).
- **Codegen output.** A generator that emits a oneof type alias (`using ContentBlock = oneof_by_field<...>;`) might reasonably want to emit a free-standing `to_json`/`from_json` overload for that type — and the natural way to do that is to call `def_type::to_json`. Right now that route is blocked.

### Suggested fix

Add public overloads constrained on `is_any_oneof_v`:

```cpp
template <typename T>
    requires detail::is_any_oneof_v<T>
nlohmann::json to_json(const T& v) {
    return detail::oneof_dispatch_to_json(v);
}
```

`from_json` is trickier because `oneof` has no default constructor (intentional — see Issue 5 in the doc-gaps section). Two options:

- **Take an initial value:** `T from_json(const nlohmann::json& j, T initial)`. Caller pre-constructs with their preferred default variant; `from_json` overlays the JSON.
- **Out-parameter overload:** `void from_json(const nlohmann::json& j, T& out)`. Caller default-initializes `T` themselves before calling. (This pattern matches what `value_from_json` already does internally.)

Either works. The out-parameter form is cleaner if you don't want to bake "user supplies a default" into the public API.

### Workaround currently in acp-cpp

Standalone polymorphic round-trip tests were dropped. Coverage relies on the carrier-struct tests (e.g. `SessionNotification` round-trip exercises `SessionUpdate` exhaustively). This works but is less surgical than testing each variant in isolation.

---

## Issue 4 — Diagnostic messages on constraint failures are MSVC template walls

### Severity: Low. Quality-of-life.

### Symptom

Every time I hit Issue 1, 2, or 3, MSVC produced a diagnostic of 30–200 lines of template instantiation context, with the actual problem message — usually a single line saying "the concept evaluated to false" — buried somewhere in the middle.

### Examples I hit during slice work

- "no matching overloaded function" pointing into `nlohmann/json.hpp` (Issue 2 — empty struct fell through to `nlohmann::json(v)` constructor)
- "the concept 'def_type::detail::reflected_struct<X>' evaluated to false" (Issue 3 — top-level oneof)
- "function was implicitly deleted because a base class invokes a deleted or inaccessible function" (covered in README, but still surprising in the wall of context)

### Suggested fix

Wrap public entry points with `static_assert` that names the problem and points at common causes. Example:

```cpp
template <typename T>
nlohmann::json to_json(const T& obj) {
    static_assert(detail::reflected_struct<T>,
        "def_type::to_json requires T to be a reflected aggregate (PFR-"
        "discoverable struct with >= 1 member). Common causes:\n"
        "  - T is empty: PFR cannot reflect zero-field aggregates. Add "
        "    at least one member, or write nlohmann::to_json/from_json "
        "    ADL hooks for T.\n"
        "  - T is a oneof<...> or oneof_by_field<...>: wrap it in a "
        "    carrier struct (recommended), or call "
        "    def_type::detail::value_to_json directly.\n"
        "  - T is missing a required field<>/oneof default initializer: "
        "    see README 'Default Construction' under oneof.");
    return detail::value_to_json(obj);
}
```

Same shape on `from_json` and `to_json_string`. Once these fire, the template wall above is suppressed and the user sees exactly what's wrong as the *first* line of diagnostic output.

### Bonus

The `oneof` no-default-ctor errors (`function was implicitly deleted because a base class invokes a deleted or inaccessible function`) could be improved by giving `oneof_impl_base` a `static_assert(false, ...)`-style explicit deletion message in C++20+ via concepts, e.g.:

```cpp
oneof_impl_base() requires false = delete;
```

…with a `requires` clause that's `false` and a comment explaining what the user needs to do (give the member an explicit default initializer). I'm not sure if `= delete("message")` is in your supported standard, but if it is, that would be even cleaner.

---

## Documentation gaps

These work as designed, but I didn't find them in the README quickly.

### Doc gap 5 — Empty-struct constraint not stated explicitly

The README's **PFR vs Manual Registration** section says "PFR inspects aggregate structs at compile time — no registration needed." It does *not* say "your struct must have at least one member." That constraint should be on the first page where types are introduced.

Suggested addition in **Quick Start**, near the first `struct Dog` example:

> ⚠️ **PFR requires at least one member.** A truly empty struct (`struct Foo {};`) cannot be reflected. If you have an empty placeholder type that will grow over versions, give it a single placeholder field (e.g. `field<bool> _placeholder = false;`) until you have real fields, or write nlohmann ADL hooks for it.

### Doc gap 6 — `optional<T>` wire shape is not specified

The README mentions `std::optional<T>` is supported and shows it in the **Complex Types in JSON** section, but doesn't tell the reader **what shape an empty `std::optional<T>` produces on the wire** — null, absent, or something configurable.

Right now it's `null` (Issue 1). Whether you fix Issue 1 or not, this needs a sentence in the README:

> `std::optional<T>` empty fields currently serialize to JSON `null` (the field is present with a null value). If your protocol distinguishes "absent" from "null" — most do — you'll want to either [link to fix] / [link to workaround pattern] / etc.

### Doc gap 7 — `oneof` transitive-default rule

The README's **Default Construction** section under `oneof<>` says:

> If you put a `oneof` inside a struct that you want to be default-constructible, give the field an explicit default initializer.

This is correct but **also applies transitively up multiple struct levels**. If `A` contains a `oneof` and `B` contains `A`, then `B` also needs to default-init its `A` member — because `A`'s default ctor was deleted, which deletes `B`'s, which deletes anything containing `B`, etc.

Concrete example (from acp-cpp, simplified):

```cpp
struct AgentMessageChunkUpdate {
    std::string  sessionUpdate = "agent_message_chunk";
    ContentBlock content;   // ❌ deletes AMCU's default ctor
};

struct SessionNotification {
    std::string             sessionId;
    SessionUpdate           update = AgentMessageChunkUpdate{};  // ❌ AMCU has no
                                                                 //    default ctor
};
```

The fix is one-line-per-oneof:

```cpp
struct AgentMessageChunkUpdate {
    std::string  sessionUpdate = "agent_message_chunk";
    ContentBlock content       = TextContentBlock{};   // ★ default-init the oneof
};
```

Worth either (a) a worked 2-level nesting example in the README, or (b) — for codegen consumers — an explicit note that the generator must emit these initializers automatically.

### Doc gap 8 — Public API surface for oneof serialization

After hitting Issue 3, I had to grep `def_type/json.hpp` to learn that `value_to_json` does dispatch to `oneof_dispatch_to_json`, but that the *public* `to_json` is constrained to `reflected_struct` and won't accept the oneof directly. That's not in the README.

Either fix Issue 3 (preferred) or add a section explaining "oneofs are serialized via their carrier struct; if you need to serialize a oneof in isolation, call X."

### Doc gap 9 — `unknown` round-trip with `_meta`

The README has a great section on `unknown`. The natural use case in spec-driven protocols is `_meta` extensibility: `optional<unknown> _meta` on every type, round-trip preserved. The README's `unknown` section uses an RPC-params example, which is great, but doesn't show the `_meta` pattern explicitly. With Issue 1 fixed, this becomes the *recommended* pattern for protocol modeling — worth a paragraph.

---

## What worked great

Calling these out so they don't get refactored away.

### ✅ Aggregate + designated-init UX

```cpp
struct PromptResponse {
    StopReason stopReason = StopReason::end_turn;
};

PromptResponse{};                                       // works
PromptResponse{.stopReason = StopReason::cancelled};    // works
def_type::from_json<PromptResponse>(...);               // works
def_type::to_json(resp);                                // works
```

Zero annotations, zero registrations, zero `BOOST_PFR_FIELDS`-style macros. This is the headline UX win and it lives up to the billing. Don't lose this.

### ✅ magic_enum integration

```cpp
enum class StopReason { end_turn, cancelled, max_tokens, max_turn_requests, refusal };
def_type::to_json(PromptResponse{.stopReason = StopReason::end_turn});
// {"stopReason": "end_turn"}
```

Just worked. Wire shape matches the spec exactly with no annotations. Beautiful.

### ✅ `oneof_by_field` discriminator handling

The discriminator field on each variant struct (`std::string type = "text"`) is the wire field; you write the struct, def_type does the rest. No registration, no manual `to_json` per variant.

The transitive-default-init pain (Issue 5 / Doc gap 7) is real but small in scope — once you know the rule, it's mechanical.

### ✅ camelCase preservation

Field names go to the wire verbatim. Critical for ACP (the spec is camelCase; acp-cpp's settled foundation decision was "field names match the wire JSON exactly"). Worked perfectly out of the box.

### ✅ `task<T>` / coroutine integration

Not directly a `def_type` win, but: I never had to fight conversion code from inside a coroutine. `auto resp = def_type::from_json<T>(co_await something)` just works.

### ✅ Compile speed

Per-module-partition compile times for the typed structs were a fraction of a second on MSVC. Slice 1 introduced 8 new `.cppm` partitions and didn't measurably change incremental build time. PFR's compile cost is real but bounded.

---

## Coverage caveat — what acp-cpp didn't exercise

So you don't over-index on the absence of feedback elsewhere:

- `parse()` and the `parse_options` strictness modes — never used.
- `field<T>` wrappers — every field in acp-cpp is a plain member.
- `with<>` field-level metadata — not used.
- `meta<T>` type-level metadata — not used.
- Validators (built-in or custom) — not used.
- The dynamic path (`type_def("Name").field<T>(...)`) — not used.
- `type_instance` — not used.
- Manual `nlohmann::to_json` / `from_json` ADL hooks alongside `def_type` — not used.
- Map types (`std::map<std::string, T>`) — not used.
- Boost.PFR vs manual `struct_info<T>` registration — only PFR exercised.
- 3-level deep nested types — only 2-level (e.g. `SessionNotification` contains `SessionUpdate` which contains `ContentBlock`).

If `def_type` has rough edges in any of those areas, my feedback won't have caught them. The slices stayed deliberately thin.

---

## Concrete next-step suggestions

If you wanted to rank a single "do this first" item: **fix Issue 1.** It's a five-line behavioral change in one function, and it removes a real interop concern with stricter ACP/JSON-RPC peers across the entire `acp-cpp` SDK. It also unblocks the `_meta` round-trip pattern, which is the natural way to handle protocol extensibility in spec-driven libraries.

After that, in order:

1. **Issue 2 — empty struct support.** Same shape (small `if constexpr` branch), enables a common modeling pattern.
2. **Issue 4 — friendlier `static_assert`s.** Saves debug time on every future hit of Issues 1/2/3 even after they're fixed (because users will still occasionally pass the wrong type and want to know fast).
3. **Doc gaps 5–9.** Cheap to land; smooth onboarding for the next consumer who isn't going to read `json.hpp` to figure out what's wrong.
4. **Issue 3 — top-level oneof.** Nice-to-have for testing; once codegen is the dominant emitter for polymorphic types, the workaround becomes invisible.

---

## How this feedback came together

If it's useful: this was written after one focused session of building two vertical slices of acp-cpp on top of `collab.jsonrpc` (which itself uses `def_type` for `unknown`). The slices ship in commits `a9fcc55` (slice 1, hello agent) through `406888d` (slice 2, fs back-channel) on `main`. The retrospective doc `docs/VERTICAL_SLICE_HELLO_AGENT_RESULTS.md` in acp-cpp captures the same issues from acp-cpp's side (less prescriptive about fixes; more about workarounds shipped).

If you want primary-source repro of any of the above, the round-trip tests in `lib/acp.types/tests/test_round_trip.cpp` and back-channel tests in `lib/acp.connection/tests/test_back_channel.cpp` of acp-cpp exercise everything that came up.

Happy to provide repros, sample code, or whatever else helps. 🏴‍☠️
