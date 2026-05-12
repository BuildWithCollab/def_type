#pragma once

#include <def_type/reflect.hpp>
#include <def_type/unknown.hpp>

#include <nlohmann/json.hpp>

namespace def_type::detail {

    // Shape-match gate. For reflected structs, succeeds when every JSON
    // key names a field of T (mirrors oneof<>'s shape-only dispatch).
    // Non-reflected T (primitives, containers, etc.) have no field schema —
    // those rely on parse-attempt alone.
    //
    // Separated from unknown.hpp to break the include cycle:
    //   field.hpp → validation_error.hpp → unknown.hpp → reflect.hpp → field.hpp
    template <typename T>
    bool unknown_shape_matches(const nlohmann::json& j) {
        if constexpr (reflected_struct<T>) {
            if (!j.is_object()) return false;
            auto fields = field_names<T>();
            for (auto it = j.begin(); it != j.end(); ++it) {
                bool found = false;
                for (auto fname : fields)
                    if (fname == it.key()) { found = true; break; }
                if (!found) return false;
            }
            return true;
        } else {
            return true;
        }
    }

}  // namespace def_type::detail
