#pragma once

#include <nlohmann/json.hpp>

#include <concepts>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <def_type/reflect.hpp>

namespace def_type {

    // Forward declarations — defined in <def_type/json.hpp>.
    // The umbrella header includes json.hpp after unknown.hpp, so these
    // become visible at the point any unknown member template is instantiated.
    namespace detail {
        template <typename T>
        nlohmann::json value_to_json(const T&);

        template <typename T>
        void value_from_json(const nlohmann::json&, T&);

        // Shape-match gate. For reflected structs, succeeds when every JSON
        // key names a field of T (mirrors oneof<>'s shape-only dispatch).
        // Non-reflected T (primitives, containers, etc.) have no field schema —
        // those rely on parse-attempt alone.
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
    }

    class unknown {
        nlohmann::json json_;

        static nlohmann::json::json_pointer to_pointer(std::string_view path) {
            if (path.empty())
                return nlohmann::json::json_pointer("");
            if (path.front() == '/')
                return nlohmann::json::json_pointer(std::string(path));
            std::string out;
            out.reserve(path.size() + 1);
            out += '/';
            for (char c : path) out += (c == '.' ? '/' : c);
            return nlohmann::json::json_pointer(std::move(out));
        }

    public:
        unknown()                                = default;
        unknown(const unknown&)                  = default;
        unknown(unknown&&) noexcept              = default;
        unknown& operator=(const unknown&)       = default;
        unknown& operator=(unknown&&) noexcept   = default;

        unknown(const nlohmann::json& j) : json_(j) {}
        unknown(nlohmann::json&& j) noexcept : json_(std::move(j)) {}

        unknown& operator=(const nlohmann::json& j) { json_ = j; return *this; }
        unknown& operator=(nlohmann::json&& j) noexcept { json_ = std::move(j); return *this; }

        template <typename T>
            requires (!std::same_as<std::remove_cvref_t<T>, unknown>
                      && !std::same_as<std::remove_cvref_t<T>, nlohmann::json>)
        unknown(const T& v) : json_(detail::value_to_json(v)) {}

        template <typename T>
            requires (!std::same_as<std::remove_cvref_t<T>, unknown>
                      && !std::same_as<std::remove_cvref_t<T>, nlohmann::json>)
        unknown& operator=(const T& v) {
            json_ = detail::value_to_json(v);
            return *this;
        }

        const nlohmann::json& raw() const noexcept { return json_; }
        nlohmann::json&       raw() noexcept       { return json_; }

        template <typename T>
        T as() const {
            if (!detail::unknown_shape_matches<T>(json_))
                throw std::logic_error("unknown::as<T>: JSON shape does not match T");
            T out{};
            detail::value_from_json(json_, out);
            return out;
        }

        template <typename T>
        bool is() const noexcept {
            if (!detail::unknown_shape_matches<T>(json_)) return false;
            try {
                T tmp{};
                detail::value_from_json(json_, tmp);
                return true;
            } catch (...) {
                return false;
            }
        }

        template <typename T>
        std::optional<T> try_as() const {
            if (!detail::unknown_shape_matches<T>(json_)) return std::nullopt;
            try {
                T tmp{};
                detail::value_from_json(json_, tmp);
                return tmp;
            } catch (...) {
                return std::nullopt;
            }
        }

        template <typename T>
        bool is(std::string_view path) const noexcept {
            try {
                const auto& sub = json_.at(to_pointer(path));
                if (!detail::unknown_shape_matches<T>(sub)) return false;
                T tmp{};
                detail::value_from_json(sub, tmp);
                return true;
            } catch (...) {
                return false;
            }
        }

        template <typename T>
        T get(std::string_view path) const {
            const auto& sub = json_.at(to_pointer(path));
            if (!detail::unknown_shape_matches<T>(sub))
                throw std::logic_error("unknown::get<T>(path): JSON shape at path does not match T");
            T out{};
            detail::value_from_json(sub, out);
            return out;
        }

        template <typename T>
        std::optional<T> try_get(std::string_view path) const {
            try {
                const auto& sub = json_.at(to_pointer(path));
                if (!detail::unknown_shape_matches<T>(sub)) return std::nullopt;
                T tmp{};
                detail::value_from_json(sub, tmp);
                return tmp;
            } catch (...) {
                return std::nullopt;
            }
        }

        friend bool operator==(const unknown& a, const unknown& b) noexcept {
            return a.json_ == b.json_;
        }
    };

}  // namespace def_type
