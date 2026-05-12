#pragma once

#include <optional>
#include <string>

#include <def_type/unknown.hpp>

namespace def_type {

// ── validation_error — shared across all paths ──────────────────────────
//
// Validator authors fill: message, code (optional), sub_path (optional),
//                         data (optional, any user struct via `unknown`).
// Framework fills:        path (field name joined with sub_path),
//                         validator (validator struct name via nameof).
// Anything a validator writes into .path or .validator is overwritten.

struct validation_error {
    std::string                message;
    std::optional<std::string> code;
    std::optional<std::string> sub_path;
    std::optional<unknown>     data;

    std::string path;
    std::string validator;
};

}  // namespace def_type
