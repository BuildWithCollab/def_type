#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <def_type/field.hpp>

namespace def_type {

struct validation_result {
    explicit operator bool() const { return errors_.empty(); }
    bool ok() const { return errors_.empty(); }

    const std::vector<validation_error>& errors() const { return errors_; }
    std::size_t error_count() const { return errors_.size(); }

    auto begin() const { return errors_.begin(); }
    auto end() const { return errors_.end(); }

    // Builder — used internally by validate()
    void add(validation_error error) { errors_.push_back(std::move(error)); }

private:
    std::vector<validation_error> errors_;
};

template <typename T>
struct parse_result {
    T value;

    // --- Error checks ---
    bool valid() const { return validation_errors_.empty(); }
    bool has_extra_keys() const { return !extra_keys_.empty(); }
    bool has_missing_fields() const { return !missing_fields_.empty(); }

    // --- Data access ---
    const std::vector<std::string>& extra_keys() const { return extra_keys_; }
    const std::vector<std::string>& missing_fields() const { return missing_fields_; }
    const std::vector<validation_error>& validation_errors() const { return validation_errors_; }

    // --- Value access ---
    T& operator*() { return value; }
    const T& operator*() const { return value; }
    T* operator->() { return &value; }
    const T* operator->() const { return &value; }

    T& checked_value() {
        if (!valid())
            throw std::logic_error("parse_result: validation errors exist");
        return value;
    }
    const T& checked_value() const {
        if (!valid())
            throw std::logic_error("parse_result: validation errors exist");
        return value;
    }

    // --- Builder (internal) ---
    std::vector<std::string> extra_keys_;
    std::vector<std::string> missing_fields_;
    std::vector<validation_error> validation_errors_;
};

struct parse_options {
    bool reject_extra_keys  = false;
    bool require_all_fields = false;
    bool require_valid      = false;
    bool strict             = false;  // sets all three to true
};

class parse_error : public std::logic_error {
public:
    parse_error(std::string message,
                std::vector<std::string> extra_keys,
                std::vector<std::string> missing_fields,
                std::vector<validation_error> validation_errors)
        : std::logic_error(std::move(message))
        , extra_keys_(std::move(extra_keys))
        , missing_fields_(std::move(missing_fields))
        , validation_errors_(std::move(validation_errors)) {}

    const std::vector<std::string>& extra_keys() const { return extra_keys_; }
    const std::vector<std::string>& missing_fields() const { return missing_fields_; }
    const std::vector<validation_error>& validation_errors() const { return validation_errors_; }

private:
    std::vector<std::string> extra_keys_;
    std::vector<std::string> missing_fields_;
    std::vector<validation_error> validation_errors_;
};

}  // namespace def_type
