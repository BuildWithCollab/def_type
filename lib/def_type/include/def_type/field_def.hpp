#pragma once

#include <any>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

#include <def_type/detail/discovery.hpp>

namespace def_type {

// ── field_def — read-only view into a dynamic field ─────────────

class field_def {
    const detail::dynamic_field_def* def_;

public:
    explicit field_def(const detail::dynamic_field_def* d) : def_(d) {}

    std::string_view name() const { return def_->name; }

    bool has_default() const { return def_->has_default; }

    template <typename V>
    V default_value() const {
        auto* p = std::any_cast<V>(&def_->default_value);
        if (!p) throw std::logic_error(
            "field '" + std::string(name()) + "': default_value type mismatch");
        return *p;
    }

    template <typename M>
    bool has_meta() const {
        for (auto& e : def_->metas)
            if (e.type == typeid(M)) return true;
        return false;
    }

    template <typename M>
    M meta() const {
        for (auto& e : def_->metas)
            if (e.type == typeid(M))
                return *std::any_cast<M>(&e.value);
        throw std::logic_error(
            "field '" + std::string(name()) + "': no meta of requested type");
    }

    template <typename M>
    std::size_t meta_count() const {
        std::size_t n = 0;
        for (auto& e : def_->metas)
            if (e.type == typeid(M)) ++n;
        return n;
    }

    template <typename M>
    std::vector<M> metas() const {
        std::vector<M> result;
        for (auto& e : def_->metas)
            if (e.type == typeid(M)) {
                auto* p = std::any_cast<M>(&e.value);
                if (!p) throw std::logic_error(
                    "field '" + std::string(name()) + "': meta storage corrupted");
                result.push_back(*p);
            }
        return result;
    }
};

// ── metadata — typed access to a type-erased meta ────────────────────

class metadata {
    const std::any* value_;
    std::type_index type_;

public:
    explicit metadata(const std::any* v, const std::type_index& t)
        : value_(v), type_(t) {}

    template <typename M>
    const M& as() const {
        auto* p = std::any_cast<M>(value_);
        if (!p) throw std::logic_error("metadata: type mismatch in as<>()");
        return *p;
    }

    template <typename M>
    const M* try_as() const { return std::any_cast<M>(value_); }

    template <typename M>
    bool is() const { return type_ == typeid(M); }
};

// ── field_value — typed access to a type-erased value ─────────────────

class field_value {
    std::any* value_;
    friend class type_instance;
    explicit field_value(std::any* v) : value_(v) {}

public:
    template <typename V>
    V& as() {
        auto* p = std::any_cast<V>(value_);
        if (!p) throw std::logic_error("field_value: type mismatch in as<>()");
        return *p;
    }

    template <typename V>
    const V& as() const {
        auto* p = std::any_cast<V>(value_);
        if (!p) throw std::logic_error("field_value: type mismatch in as<>()");
        return *p;
    }

    template <typename V>
    V* try_as() { return std::any_cast<V>(value_); }

    template <typename V>
    const V* try_as() const { return std::any_cast<V>(value_); }
};

class const_field_value {
    const std::any* value_;
    friend class type_instance;
    explicit const_field_value(const std::any* v) : value_(v) {}

public:
    template <typename V>
    const V& as() const {
        auto* p = std::any_cast<V>(value_);
        if (!p) throw std::logic_error("const_field_value: type mismatch in as<>()");
        return *p;
    }

    template <typename V>
    const V* try_as() const { return std::any_cast<V>(value_); }
};

}  // namespace def_type
