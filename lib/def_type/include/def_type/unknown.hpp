#pragma once

#include <nlohmann/json.hpp>

#include <concepts>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace def_type {

    // Forward declarations — bodies live in later headers that the umbrella
    // header pulls in after unknown.hpp.  Template member functions of
    // `unknown` call these in dependent context, so definitions are needed
    // only at point of instantiation (phase 2), not here.
    namespace detail {
        template <typename T>
        nlohmann::json value_to_json(const T&);

        template <typename T>
        void value_from_json(const nlohmann::json&, T&);

        // Defined in <def_type/detail/unknown_reflect.hpp> (needs reflect.hpp,
        // which depends on field.hpp — kept out of unknown.hpp to break cycle).
        template <typename T>
        bool unknown_shape_matches(const nlohmann::json& j);
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
