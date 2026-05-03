# Proposal: Typed Polymorphic Tool Payloads

`tool_input` and `tool_response` are opaque JSON strings on both the Agentic and CollabMini sides. Every consumer parses the string by hand, pattern-matches on `tool_name`, and hopes the shape is right. There is no compile-time coupling between the tag and the payload shape.

This proposal replaces those fields with typed discriminated variants and adds one primitive to def_type to support them.

## The change

Today:

```cpp
struct AgentEvent {
    std::string tool_name;
    std::string tool_input;     // JSON string
    std::string tool_response;  // JSON string
    // ...
};
```

Proposed:

```cpp
namespace tools {
    struct Read { std::string file_path; int offset = 0; int limit = 0; };
    struct Bash { std::string command; int timeout = 0; };
    struct Edit { std::string file_path; std::string old_str; std::string new_str; };
    struct Grep { std::string pattern; std::string path; };
    // ...one struct per known tool...
}

namespace tool_results {
    struct Read { std::string file_path; int content_length = 0; };
    struct Bash { std::string output; std::optional<int> exit_code; bool interrupted = false; };
    struct Edit { std::string file_path; std::string diff; };
    // ...one per known tool...
}

using ToolInput    = def_type::variant<tools::Read, tools::Bash, tools::Edit, tools::Grep, /* ... */>;
using ToolResponse = def_type::variant<tool_results::Read, tool_results::Bash, tool_results::Edit, /* ... */>;

struct AgentEvent {
    std::string  tool_name;      // stays — used for display and filtering
    ToolInput    tool_input;     // was std::string
    ToolResponse tool_response;  // was std::string
    // ...
};
```

The same change applies to `EventInfo` and `StreamingEvent` in the CollabMini bridge DTOs.

## The new def_type primitive

`def_type::variant<Alts...>` — a class template def_type needs to grow. Sketch of what it must provide:

```cpp
namespace def_type {
    template <typename... Alts>
    class variant {
    public:
        template <typename T> variant(T v);

        // Query the active alternative
        template <typename T> T*       as();
        template <typename T> const T* as() const;
        template <typename T> bool     is() const;

        // Pattern match over all alternatives
        template <typename... Fs> auto match(Fs... fs);
        template <typename... Fs> auto match(Fs... fs) const;

        // JSON codec — called by def_type::to_json / def_type::from_json
        //   - to_json   : emits the active alternative's fields + a tag
        //   - from_json : reads the tag, picks the matching alternative, parses its fields
        // Tag is the alternative's struct name (e.g. "Read" for tools::Read),
        // extracted at compile time via type-name introspection.
    };
}
```

Nothing magical. One class template, one JSON codec specialization. No macros, no operator overloading, no CNTTP string tricks, no library-convention members on the user's structs.

## Usage

Construction:

```cpp
AgentEvent ev;
ev.event_type = "tool_use";
ev.tool_name  = "Read";
ev.tool_input = tools::Read{ .file_path = "foo.cpp", .offset = 10 };
```

Read a single case:

```cpp
if (auto* r = ev.tool_input.as<tools::Read>()) {
    log(r->file_path);
}
```

Exhaustive match:

```cpp
ev.tool_input.match(
    [](const tools::Read& r) { log_read(r.file_path); },
    [](const tools::Bash& b) { log_bash(b.command); },
    [](const tools::Edit& e) { log_edit(e.file_path); }
);
```

## What this replaces

- `JSON.parse(toolInput)` / `JSON.parse(toolResponse)` at every consumer — gone. Parsing happens once, inside `def_type::from_json`.
- Per-consumer parser helpers (13 in the React renderers, plus handlers on the C++ side) — gone.
- Hand-maintained alignment between the `tool_name` string and the payload shape — now enforced by the variant.

## What this does not change

- `tool_name` stays on the event as a display/filter field.
- The Agentic DB schema still stores `tool_input` and `tool_response` as TEXT columns holding JSON. Only the in-memory C++ types change; serialization produces the same (or equivalent) JSON on the wire.
- Provider-specific tool payloads that are not in the normalized spec land in an `Unknown` alternative of the variant. No feature regression for tools we don't model yet.

## Out of scope

- Runtime plugin registration of new variants. The tool set is closed and driven by the normalized spec.
- Replacing `tool_name` as the display field. It remains a plain string alongside the typed variant.
- Changes to any transport or bridge dispatch code. def_type's JSON codec handles serialization end to end.
