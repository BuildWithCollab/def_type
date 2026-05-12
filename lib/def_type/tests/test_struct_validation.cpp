#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

#include <def_type.hpp>

using namespace def_type;
using namespace def_type::validations;
using json = nlohmann::json;

// ═════════════════════════════════════════════════════════════════════════
// Dynamic-side struct validators (receive const type_instance&)
// ═════════════════════════════════════════════════════════════════════════

struct dyn_passwords_match {
    std::vector<validation_error> operator()(const type_instance& obj) const {
        auto p = obj.get<std::string>("password");
        auto c = obj.get<std::string>("confirm");
        if (p == c) return {};
        return {{.message = "passwords do not match",
                 .code = "password_mismatch",
                 .sub_path = "confirm"}};
    }
};

struct dyn_terms_accepted {
    std::vector<validation_error> operator()(const type_instance& obj) const {
        if (obj.get<bool>("terms")) return {};
        return {{.message = "terms must be accepted", .code = "terms_required"}};
    }
};

struct dyn_two_findings {
    std::vector<validation_error> operator()(const type_instance& obj) const {
        std::vector<validation_error> findings;
        if (obj.get<std::string>("password") != obj.get<std::string>("confirm")) {
            findings.push_back({.message = "mismatch", .code = "mm", .sub_path = "password"});
            findings.push_back({.message = "mismatch", .code = "mm", .sub_path = "confirm"});
        }
        return findings;
    }
};

struct dyn_zip_5_digits {
    std::vector<validation_error> operator()(const type_instance& obj) const {
        auto zip = obj.get<std::string>("zip");
        if (zip.size() == 5) return {};
        return {{.message = "zip must be 5 digits", .code = "bad_zip", .sub_path = "zip"}};
    }
};

struct dyn_throws {
    std::vector<validation_error> operator()(const type_instance&) const {
        throw std::runtime_error("boom");
    }
};

// ═════════════════════════════════════════════════════════════════════════
// Typed-side struct validators (generic — match any struct with .password / .confirm / etc.)
// ═════════════════════════════════════════════════════════════════════════

struct passwords_match_rule {
    template <typename S>
    std::vector<validation_error> operator()(const S& s) const {
        if (s.password.value == s.confirm.value) return {};
        return {{.message = "passwords do not match",
                 .code = "password_mismatch",
                 .sub_path = "confirm"}};
    }
};

struct terms_accepted_rule {
    template <typename S>
    std::vector<validation_error> operator()(const S& s) const {
        if (s.terms.value) return {};
        return {{.message = "terms must be accepted", .code = "terms_required"}};
    }
};

struct two_findings_rule {
    template <typename S>
    std::vector<validation_error> operator()(const S& s) const {
        std::vector<validation_error> findings;
        if (s.password.value != s.confirm.value) {
            findings.push_back({.message = "mismatch", .code = "mm", .sub_path = "password"});
            findings.push_back({.message = "mismatch", .code = "mm", .sub_path = "confirm"});
        }
        return findings;
    }
};

struct zip_5_digits_rule {
    template <typename A>
    std::vector<validation_error> operator()(const A& a) const {
        if (a.zip.value.size() == 5) return {};
        return {{.message = "zip must be 5 digits", .code = "bad_zip", .sub_path = "zip"}};
    }
};

struct throwing_rule {
    template <typename S>
    std::vector<validation_error> operator()(const S&) const {
        throw std::runtime_error("boom");
    }
};

// Stateful validator — proves config is preserved into the marker tuple
struct must_have_length {
    std::size_t expected = 0;
    template <typename S>
    std::vector<validation_error> operator()(const S& s) const {
        if (s.name.value.size() == expected) return {};
        return {{.message = "wrong length", .code = "len", .sub_path = "name"}};
    }
};

// ═════════════════════════════════════════════════════════════════════════
// Typed structs
// ═════════════════════════════════════════════════════════════════════════

struct SignupFormOneRule {
    type_validators<passwords_match_rule> rules{passwords_match_rule{}};
    field<std::string> password{.value = ""};
    field<std::string> confirm{.value = ""};
    field<bool>        terms{.value = false};
};

struct SignupFormTwoRules {
    type_validators<passwords_match_rule, terms_accepted_rule> rules{
        passwords_match_rule{}, terms_accepted_rule{}};
    field<std::string> password{.value = ""};
    field<std::string> confirm{.value = ""};
    field<bool>        terms{.value = false};
};

// Two separate type_validators<> members — additive
struct SignupFormSplitMarkers {
    type_validators<passwords_match_rule> r1{passwords_match_rule{}};
    type_validators<terms_accepted_rule>  r2{terms_accepted_rule{}};
    field<std::string> password{.value = ""};
    field<std::string> confirm{.value = ""};
    field<bool>        terms{.value = false};
};

// No struct rules — baseline
struct PlainSignup {
    field<std::string> password{.value = ""};
    field<std::string> confirm{.value = ""};
};

// Struct + field validators on same type
struct SignupFormFieldAndStruct {
    type_validators<passwords_match_rule> rules{passwords_match_rule{}};
    field<std::string> password{
        .value = "", .validators = validators(not_empty{})};
    field<std::string> confirm{
        .value = "", .validators = validators(not_empty{})};
};

// Stateful — preserves config
struct NamedThing {
    type_validators<must_have_length> rules{must_have_length{.expected = 5}};
    field<std::string> name{.value = ""};
};

// Nested
struct AddressWithRule {
    type_validators<zip_5_digits_rule> rules{zip_5_digits_rule{}};
    field<std::string> street{.value = ""};
    field<std::string> zip{.value = ""};
};

struct PersonWithNestedAddress {
    field<std::string>     name{.value = "Alice"};
    field<AddressWithRule> address;
};

// Both parent and child carry struct rules
struct name_must_be_alice {
    template <typename P>
    std::vector<validation_error> operator()(const P& p) const {
        if (p.name.value == "Alice") return {};
        return {{.message = "must be Alice", .code = "name_alice", .sub_path = "name"}};
    }
};

struct PersonRuleAndAddressRule {
    type_validators<name_must_be_alice> rules{name_must_be_alice{}};
    field<std::string>     name{.value = ""};
    field<AddressWithRule> address;
};

// Parent struct rule + child field validator (field on Address, not struct)
struct AddressFieldOnly {
    field<std::string> street{.value = ""};
    field<std::string> zip{.value = "", .validators = validators(not_empty{})};
};

struct PersonStructRuleChildFieldRule {
    type_validators<name_must_be_alice> rules{name_must_be_alice{}};
    field<std::string>      name{.value = ""};
    field<AddressFieldOnly> address;
};

// 3-level deep
struct InnerWithRule {
    type_validators<zip_5_digits_rule> rules{zip_5_digits_rule{}};
    field<std::string> zip{.value = ""};
};
struct MiddleWithInner {
    field<InnerWithRule> inner;
};
struct OuterWithMiddle {
    field<MiddleWithInner> middle;
};

// Empty marker — degenerate case
struct EmptyMarkerStruct {
    type_validators<> rules{};
    field<std::string> name{.value = "Rex"};
};

// Throws from inside a struct-level rule
struct ThrowingStruct {
    type_validators<throwing_rule> rules{throwing_rule{}};
    field<std::string> name{.value = ""};
};

// ═════════════════════════════════════════════════════════════════════════
// [validation][struct][shape] — contract semantics
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("struct shape: empty findings → ok", "[validation][struct][shape]") {
    auto t = type_def("X")
        .field<bool>("terms", true)
        .validators(dyn_terms_accepted{});
    auto obj = t.create();
    REQUIRE(obj.validate().ok());
}

TEST_CASE("struct shape: one finding → one error with validator name", "[validation][struct][shape]") {
    auto t = type_def("X")
        .field<bool>("terms", false)
        .validators(dyn_terms_accepted{});
    auto obj = t.create();
    auto r = obj.validate();

    REQUIRE(r.error_count() == 1);
    REQUIRE(r.errors()[0].message == "terms must be accepted");
    REQUIRE(r.errors()[0].validator == "dyn_terms_accepted");
    REQUIRE(r.errors()[0].code == "terms_required");
    REQUIRE_FALSE(r.errors()[0].sub_path.has_value());
    REQUIRE(r.errors()[0].path == "");
}

TEST_CASE("struct shape: multi-finding from one validator", "[validation][struct][shape]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string("a"))
        .field<std::string>("confirm", std::string("b"))
        .validators(dyn_two_findings{});
    auto obj = t.create();
    auto r = obj.validate();

    REQUIRE(r.error_count() == 2);
    REQUIRE(r.errors()[0].validator == "dyn_two_findings");
    REQUIRE(r.errors()[1].validator == "dyn_two_findings");
    REQUIRE(r.errors()[0].path == "password");
    REQUIRE(r.errors()[1].path == "confirm");
}

TEST_CASE("struct shape: sub_path becomes bare path at top level", "[validation][struct][shape]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string("a"))
        .field<std::string>("confirm", std::string("b"))
        .validators(dyn_passwords_match{});
    auto obj = t.create();
    auto r = obj.validate();

    REQUIRE(r.error_count() == 1);
    REQUIRE(r.errors()[0].path == "confirm");
    REQUIRE(r.errors()[0].sub_path == "confirm");
}

// ═════════════════════════════════════════════════════════════════════════
// [validation][struct][dynamic]
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("struct dynamic: variadic validators in one call", "[validation][struct][dynamic]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string("a"))
        .field<std::string>("confirm", std::string("b"))
        .field<bool>("terms", false)
        .validators(dyn_passwords_match{}, dyn_terms_accepted{});
    auto obj = t.create();
    auto r = obj.validate();
    REQUIRE(r.error_count() == 2);
}

TEST_CASE("struct dynamic: chained .validators() are additive", "[validation][struct][dynamic]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string("a"))
        .field<std::string>("confirm", std::string("b"))
        .field<bool>("terms", false)
        .validators(dyn_passwords_match{})
        .validators(dyn_terms_accepted{});
    auto obj = t.create();
    auto r = obj.validate();
    REQUIRE(r.error_count() == 2);
}

TEST_CASE("struct dynamic: fires alongside field validators", "[validation][struct][dynamic]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string(""), validators(not_empty{}))
        .field<std::string>("confirm", std::string(""), validators(not_empty{}))
        .validators(dyn_passwords_match{});
    auto obj = t.create();
    auto r = obj.validate();
    // Two field-level not_empty failures. Both fields equal (both empty), so
    // the struct rule passes — total 2.
    REQUIRE(r.error_count() == 2);
}

TEST_CASE("struct dynamic: fixing values clears struct findings", "[validation][struct][dynamic]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string("a"))
        .field<std::string>("confirm", std::string("b"))
        .validators(dyn_passwords_match{});
    auto obj = t.create();
    REQUIRE_FALSE(obj.valid());
    obj.set("confirm", std::string("a"));
    REQUIRE(obj.valid());
}

TEST_CASE("struct dynamic: valid() false when only struct rule fails", "[validation][struct][dynamic]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string("a"))
        .field<std::string>("confirm", std::string("b"))
        .validators(dyn_passwords_match{});
    REQUIRE_FALSE(t.create().valid());
}

TEST_CASE("struct dynamic: no validators registered → no struct findings", "[validation][struct][dynamic]") {
    auto t = type_def("X").field<std::string>("name", std::string("Rex"));
    auto obj = t.create();
    REQUIRE(obj.validate().ok());
    REQUIRE_FALSE(t.has_validators());
    REQUIRE(t.validator_count() == 0);
}

// ═════════════════════════════════════════════════════════════════════════
// [validation][struct][typed]
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("struct typed: single type_validators member runs the rule", "[validation][struct][typed]") {
    SignupFormOneRule f;
    f.password = "a";
    f.confirm = "b";
    type_def<SignupFormOneRule> t;
    auto r = t.validate(f);
    REQUIRE(r.error_count() == 1);
    REQUIRE(r.errors()[0].path == "confirm");
    REQUIRE(r.errors()[0].validator == "passwords_match_rule");
}

TEST_CASE("struct typed: type_validators<V1,V2> runs both", "[validation][struct][typed]") {
    SignupFormTwoRules f;
    f.password = "a";
    f.confirm = "b";
    f.terms = false;
    auto r = type_def<SignupFormTwoRules>{}.validate(f);
    REQUIRE(r.error_count() == 2);
}

TEST_CASE("struct typed: two separate type_validators<> members are additive", "[validation][struct][typed]") {
    SignupFormSplitMarkers f;
    f.password = "a";
    f.confirm = "b";
    f.terms = false;
    auto r = type_def<SignupFormSplitMarkers>{}.validate(f);
    REQUIRE(r.error_count() == 2);
}

TEST_CASE("struct typed: empty type_validators<> compiles and runs no rules", "[validation][struct][typed]") {
    EmptyMarkerStruct e;
    type_def<EmptyMarkerStruct> t;
    REQUIRE(t.has_validators() == false);
    REQUIRE(t.validator_count() == 0);
    REQUIRE(t.validate(e).ok());
}

TEST_CASE("struct typed: stateful validator preserves its config", "[validation][struct][typed]") {
    NamedThing n;
    n.name = "1234";  // length 4, expected 5
    auto r = type_def<NamedThing>{}.validate(n);
    REQUIRE(r.error_count() == 1);

    NamedThing m;
    m.name = "12345";
    REQUIRE(type_def<NamedThing>{}.validate(m).ok());
}

TEST_CASE("struct typed: fixing values makes valid() true", "[validation][struct][typed]") {
    SignupFormOneRule f;
    f.password = "a";
    f.confirm = "b";
    type_def<SignupFormOneRule> t;
    REQUIRE_FALSE(t.valid(f));
    f.confirm = "a";
    REQUIRE(t.valid(f));
}

TEST_CASE("struct typed: struct + field validators compose", "[validation][struct][typed]") {
    SignupFormFieldAndStruct f;  // both empty, passwords "match" but both empty
    type_def<SignupFormFieldAndStruct> t;
    auto r = t.validate(f);
    // 2 field not_empty failures, 0 struct (a == a)
    REQUIRE(r.error_count() == 2);

    f.password = "a";
    f.confirm = "b";
    r = t.validate(f);
    // 0 field failures (both non-empty), 1 struct failure
    REQUIRE(r.error_count() == 1);
    REQUIRE(r.errors()[0].validator == "passwords_match_rule");
}

// ═════════════════════════════════════════════════════════════════════════
// [validation][struct][typed][marker-discipline] — invisible everywhere except validation
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("marker: field_count excludes type_validators member", "[validation][struct][typed][marker-discipline]") {
    type_def<SignupFormOneRule> t;
    REQUIRE(t.field_count() == 3);  // password, confirm, terms
}

TEST_CASE("marker: field_names omits the marker member", "[validation][struct][typed][marker-discipline]") {
    type_def<SignupFormOneRule> t;
    auto names = t.field_names();
    REQUIRE(names.size() == 3);
    for (auto& n : names) REQUIRE(n != "rules");
}

TEST_CASE("marker: has_field('rules') is false", "[validation][struct][typed][marker-discipline]") {
    type_def<SignupFormOneRule> t;
    REQUIRE_FALSE(t.has_field("rules"));
}

TEST_CASE("marker: set('rules', ...) throws like a missing field", "[validation][struct][typed][marker-discipline]") {
    SignupFormOneRule f;
    type_def<SignupFormOneRule> t;
    REQUIRE_THROWS_AS(t.set(f, "rules", std::string("x")), std::logic_error);
}

TEST_CASE("marker: get('rules') throws like a missing field", "[validation][struct][typed][marker-discipline]") {
    SignupFormOneRule f;
    type_def<SignupFormOneRule> t;
    REQUIRE_THROWS_AS(t.get<std::string>(f, "rules"), std::logic_error);
}

TEST_CASE("marker: to_json output omits the marker member", "[validation][struct][typed][marker-discipline]") {
    SignupFormOneRule f;
    f.password = "p";
    f.confirm = "p";
    auto j = to_json(f);
    REQUIRE(j.is_object());
    REQUIRE_FALSE(j.contains("rules"));
    REQUIRE(j.contains("password"));
    REQUIRE(j.contains("confirm"));
    REQUIRE(j.contains("terms"));
}

TEST_CASE("marker: from_json silently ignores a 'rules' key in input", "[validation][struct][typed][marker-discipline]") {
    auto j = json{{"password", "p"}, {"confirm", "p"}, {"terms", true},
                  {"rules", "should be ignored"}};
    auto f = from_json<SignupFormOneRule>(j);
    REQUIRE(std::string(f.password) == "p");
    REQUIRE(std::string(f.confirm) == "p");
    REQUIRE(bool(f.terms) == true);
}

TEST_CASE("marker: for_each_field does not visit the marker", "[validation][struct][typed][marker-discipline]") {
    SignupFormOneRule f;
    std::vector<std::string> visited;
    type_def<SignupFormOneRule>{}.for_each_field(f,
        [&](std::string_view name, auto&) { visited.emplace_back(name); });
    REQUIRE(visited.size() == 3);
    for (auto& v : visited) REQUIRE(v != "rules");
}

// ═════════════════════════════════════════════════════════════════════════
// [validation][struct][nested] — path prefixing
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("nested typed: inner struct rule's path is prefixed by parent field", "[validation][struct][nested]") {
    PersonWithNestedAddress p;
    p.address.value.zip = "123";  // length 3, fails
    auto r = type_def<PersonWithNestedAddress>{}.validate(p);
    REQUIRE(r.error_count() == 1);
    REQUIRE(r.errors()[0].path == "address.zip");
    REQUIRE(r.errors()[0].validator == "zip_5_digits_rule");
}

TEST_CASE("nested typed: 3-level deep path is fully prefixed", "[validation][struct][nested]") {
    OuterWithMiddle o;
    o.middle.value.inner.value.zip = "1";
    auto r = type_def<OuterWithMiddle>{}.validate(o);
    REQUIRE(r.error_count() == 1);
    REQUIRE(r.errors()[0].path == "middle.inner.zip");
}

TEST_CASE("nested dynamic: inner struct rule fires with prefix", "[validation][struct][nested]") {
    auto addr = type_def("Address")
        .field<std::string>("zip", std::string(""))
        .validators(dyn_zip_5_digits{});
    auto person = type_def("Person")
        .field<std::string>("name", std::string("Alice"))
        .field("address", addr);

    auto p = person.create();
    auto r = p.validate();
    REQUIRE(r.error_count() == 1);
    REQUIRE(r.errors()[0].path == "address.zip");
    REQUIRE(r.errors()[0].validator == "dyn_zip_5_digits");
}

// ═════════════════════════════════════════════════════════════════════════
// [validation][struct][parse] — parse() integration
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("parse dynamic: struct rule failure shows up in validation_errors", "[validation][struct][parse]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string(""))
        .field<std::string>("confirm", std::string(""))
        .validators(dyn_passwords_match{});

    auto r = t.parse(json{{"password", "a"}, {"confirm", "b"}});
    REQUIRE_FALSE(r.valid());
    REQUIRE(r.validation_errors().size() == 1);
    REQUIRE(r.validation_errors()[0].validator == "dyn_passwords_match");
}

TEST_CASE("parse dynamic: require_valid throws on struct rule failure", "[validation][struct][parse]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string(""))
        .field<std::string>("confirm", std::string(""))
        .validators(dyn_passwords_match{});

    REQUIRE_THROWS_AS(
        t.parse(json{{"password", "a"}, {"confirm", "b"}}, {.require_valid = true}),
        parse_error);
}

TEST_CASE("parse dynamic: strict trips on struct rule failure", "[validation][struct][parse]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string(""))
        .field<std::string>("confirm", std::string(""))
        .validators(dyn_passwords_match{});

    REQUIRE_THROWS_AS(
        t.parse(json{{"password", "a"}, {"confirm", "b"}}, {.strict = true}),
        parse_error);
}

TEST_CASE("parse typed: struct rule failure surfaces in validation_errors", "[validation][struct][parse]") {
    auto r = type_def<SignupFormOneRule>{}.parse(
        json{{"password", "a"}, {"confirm", "b"}, {"terms", true}});
    REQUIRE_FALSE(r.valid());
    REQUIRE(r.validation_errors().size() == 1);
    REQUIRE(r.validation_errors()[0].path == "confirm");
}

TEST_CASE("parse typed: require_valid on struct rule throws parse_error carrying findings", "[validation][struct][parse]") {
    try {
        type_def<SignupFormOneRule>{}.parse(
            json{{"password", "a"}, {"confirm", "b"}, {"terms", true}},
            {.require_valid = true});
        FAIL("expected parse_error");
    } catch (const parse_error& e) {
        REQUIRE(e.validation_errors().size() == 1);
        REQUIRE(e.validation_errors()[0].validator == "passwords_match_rule");
    }
}

// ═════════════════════════════════════════════════════════════════════════
// [validation][struct][query] — runtime introspection
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("query dynamic: has_validators / validator_count", "[validation][struct][query]") {
    auto t = type_def("X")
        .field<bool>("terms", false)
        .validators(dyn_terms_accepted{}, dyn_passwords_match{});
    REQUIRE(t.has_validators());
    REQUIRE(t.validator_count() == 2);
}

TEST_CASE("query typed: validator_count sums across multiple markers", "[validation][struct][query]") {
    type_def<SignupFormSplitMarkers> t;
    REQUIRE(t.has_validators());
    REQUIRE(t.validator_count() == 2);
}

TEST_CASE("query typed: has_validators is false on a struct without markers", "[validation][struct][query]") {
    type_def<PlainSignup> t;
    REQUIRE_FALSE(t.has_validators());
    REQUIRE(t.validator_count() == 0);
}

// ═════════════════════════════════════════════════════════════════════════
// [validation][struct][interactions] — cross-cutting
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("interactions: struct validators do not block set/get/to_json", "[validation][struct][interactions]") {
    auto t = type_def("X")
        .field<std::string>("password", std::string("a"))
        .field<std::string>("confirm", std::string("b"))
        .validators(dyn_passwords_match{});
    auto obj = t.create();

    obj.set("password", std::string("x"));
    REQUIRE(obj.get<std::string>("password") == "x");
    auto j = obj.to_json();
    REQUIRE(j["password"] == "x");
    REQUIRE_FALSE(obj.valid());  // still invalid
}

TEST_CASE("interactions: two struct validators of identical type both run", "[validation][struct][interactions]") {
    auto t = type_def("X")
        .field<bool>("terms", false)
        .validators(dyn_terms_accepted{}, dyn_terms_accepted{});
    auto r = t.create().validate();
    REQUIRE(r.error_count() == 2);
    REQUIRE(r.errors()[0].validator == "dyn_terms_accepted");
    REQUIRE(r.errors()[1].validator == "dyn_terms_accepted");
}

TEST_CASE("interactions: throwing struct validator propagates from validate (dynamic)", "[validation][struct][interactions]") {
    auto t = type_def("X")
        .field<bool>("terms", false)
        .validators(dyn_throws{});
    auto obj = t.create();
    REQUIRE_THROWS_AS(obj.validate(), std::runtime_error);
    REQUIRE_THROWS_AS(obj.valid(),    std::runtime_error);
}

TEST_CASE("interactions: throwing struct validator propagates from validate (typed)", "[validation][struct][interactions]") {
    ThrowingStruct s;
    REQUIRE_THROWS_AS(type_def<ThrowingStruct>{}.validate(s), std::runtime_error);
    REQUIRE_THROWS_AS(type_def<ThrowingStruct>{}.valid(s),    std::runtime_error);
}

// ═════════════════════════════════════════════════════════════════════════
// [validation][struct][nested] — both-sides and mixed-shape cases
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("nested typed: parent and child both carry struct rules", "[validation][struct][nested]") {
    PersonRuleAndAddressRule p;
    // name="" violates parent rule (must be Alice), zip="" violates child rule
    auto r = type_def<PersonRuleAndAddressRule>{}.validate(p);
    REQUIRE(r.error_count() == 2);

    bool saw_parent = false, saw_child = false;
    for (auto& e : r.errors()) {
        if (e.path == "name"          && e.validator == "name_must_be_alice") saw_parent = true;
        if (e.path == "address.zip"   && e.validator == "zip_5_digits_rule")  saw_child  = true;
    }
    REQUIRE(saw_parent);
    REQUIRE(saw_child);
}

TEST_CASE("nested typed: parent struct rule + child field validator coexist", "[validation][struct][nested]") {
    PersonStructRuleChildFieldRule p;
    // name="" trips parent struct rule, zip="" trips child field's not_empty
    auto r = type_def<PersonStructRuleChildFieldRule>{}.validate(p);
    REQUIRE(r.error_count() == 2);

    bool saw_struct = false, saw_field = false;
    for (auto& e : r.errors()) {
        if (e.path == "name"        && e.validator == "name_must_be_alice") saw_struct = true;
        if (e.path == "address.zip" && e.validator == "not_empty")          saw_field  = true;
    }
    REQUIRE(saw_struct);
    REQUIRE(saw_field);
}

// ═════════════════════════════════════════════════════════════════════════
// Field validator throwing — convention pin
//
// `validation.hpp` has no try/catch and no existing test exercised this.
// Pinning the behavior here: exceptions from a field validator propagate
// out of validate() / valid(), same as struct-level validators.
// ═════════════════════════════════════════════════════════════════════════

struct field_throws {
    std::vector<validation_error> operator()(const std::string&) const {
        throw std::runtime_error("field boom");
    }
};

struct StructWithThrowingField {
    field<std::string> name{.value = "x", .validators = validators(field_throws{})};
};

TEST_CASE("convention: field validator exception propagates (dynamic)", "[validation][shape][interactions]") {
    auto t = type_def("X")
        .field<std::string>("name", std::string("x"), validators(field_throws{}));
    auto obj = t.create();
    REQUIRE_THROWS_AS(obj.validate(), std::runtime_error);
    REQUIRE_THROWS_AS(obj.valid(),    std::runtime_error);
}

TEST_CASE("convention: field validator exception propagates (typed)", "[validation][shape][interactions]") {
    StructWithThrowingField s;
    REQUIRE_THROWS_AS(type_def<StructWithThrowingField>{}.validate(s), std::runtime_error);
    REQUIRE_THROWS_AS(type_def<StructWithThrowingField>{}.valid(s),    std::runtime_error);
}
