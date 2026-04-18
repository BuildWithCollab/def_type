#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>

#include <nlohmann/json.hpp>

#include <string>

#include <def_type/def_type.hpp>

using namespace def_type;
using namespace def_type::validations;
using json = nlohmann::json;

// ═════════════════════════════════════════════════════════════════════════
// Spike Test 1: single validator on dynamic field, valid() returns false
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("spike: single validator on dynamic field", "[validation][spike]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}));

    auto dog = dog_type.create();
    // name is "" — not_empty should fail

    REQUIRE(!dog.valid());
}

// ═════════════════════════════════════════════════════════════════════════
// Spike Test 2: validate() returns errors with details
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("spike: validate() returns errors with details", "[validation][spike]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}));

    auto dog = dog_type.create();
    auto result = dog.validate();

    REQUIRE(!result);
    REQUIRE(result.error_count() == 1);
    REQUIRE(result.errors()[0].path == "name");
    REQUIRE(result.errors()[0].constraint == "not_empty");
    REQUIRE(!result.errors()[0].message.empty());
}

// ═════════════════════════════════════════════════════════════════════════
// Spike Test 3: multiple validators on one field
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("spike: multiple validators on one field", "[validation][spike]") {
    auto config_type = type_def("Config")
        .field<std::string>("token", std::string(""),
            validators(not_empty{}, max_length{10}));

    auto config = config_type.create();
    config.set("token", std::string("this-is-way-too-long-for-the-limit"));

    auto result = config.validate();
    // not_empty passes (it's not empty), but max_length fails
    REQUIRE(result.error_count() == 1);

    // Now set it empty — not_empty fails, max_length still passes (0 <= 10)
    config.set("token", std::string(""));
    auto result2 = config.validate();
    REQUIRE(result2.error_count() == 1);

    // Set to something long AND valid for not_empty — only max_length fails
    config.set("token", std::string("abcdefghijk"));  // 11 chars > 10
    auto result3 = config.validate();
    REQUIRE(result3.error_count() == 1);

    // Set to valid for both — no errors
    config.set("token", std::string("ok"));
    auto result4 = config.validate();
    REQUIRE(result4.ok());
}

// ═════════════════════════════════════════════════════════════════════════
// Spike Test 4: fields without validators always pass
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("spike: fields without validators always pass", "[validation][spike]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}))
        .field<int>("age")            // no validators
        .field<std::string>("breed"); // no validators

    auto dog = dog_type.create();
    dog.set("age", 3);
    dog.set("breed", std::string("Husky"));
    // name is still "" — only that one should fail

    auto result = dog.validate();
    REQUIRE(result.error_count() == 1);
    REQUIRE(result.errors()[0].path == "name");
}

// ═════════════════════════════════════════════════════════════════════════
// Spike Test 5: fixing values makes validation pass
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("spike: valid() becomes true after fixing values", "[validation][spike]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}))
        .field<int>("age", 0,
            validators(positive{}));

    auto dog = dog_type.create();
    REQUIRE(!dog.valid());

    dog.set("name", std::string("Rex"));
    REQUIRE(!dog.valid()); // age still 0

    dog.set("age", 3);
    REQUIRE(dog.valid()); // both pass now

    auto result = dog.validate();
    REQUIRE(result.ok());
    REQUIRE(result.error_count() == 0);
}

// ═════════════════════════════════════════════════════════════════════════
// Spike Test 6: validators don't interfere with normal field operations
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("spike: validators don't affect set/get/to_json", "[validation][spike]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}))
        .field<int>("age", 0,
            validators(positive{}));

    auto dog = dog_type.create();

    // Invalid values can still be set and retrieved
    dog.set("name", std::string(""));
    REQUIRE(dog.get<std::string>("name") == "");

    // to_json still works even when invalid
    auto j = dog.to_json();
    REQUIRE(j["name"] == "");
    REQUIRE(j["age"] == 0);

    // Schema queries unaffected
    REQUIRE(dog.type().field_count() == 2);
    REQUIRE(dog.type().field_names()[0] == "name");
}

// ═════════════════════════════════════════════════════════════════════════
// Validators mixed with metas in the same .field() call
// ═════════════════════════════════════════════════════════════════════════

struct spike_help_info {
    const char* summary = "";
};

struct spike_cli_meta {
    struct { char short_flag = '\0'; } cli;
};

TEST_CASE("spike: validators and with<> metas in same field call", "[validation][dynamic]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}),
            with<spike_help_info>({.summary = "Dog's name"}))
        .field<int>("age", 0,
            validators(positive{}),
            with<spike_cli_meta>({.cli = {.short_flag = 'a'}}),
            with<spike_help_info>({.summary = "Age in years"}));

    // Validators work
    auto dog = dog_type.create();
    REQUIRE(!dog.valid());

    dog.set("name", std::string("Rex"));
    dog.set("age", 3);
    REQUIRE(dog.valid());

    // Metas are accessible
    REQUIRE(dog_type.field("name").has_meta<spike_help_info>());
    REQUIRE(std::string_view{dog_type.field("name").meta<spike_help_info>().summary} == "Dog's name");
    REQUIRE(dog_type.field("age").has_meta<spike_cli_meta>());
    REQUIRE(dog_type.field("age").meta<spike_cli_meta>().cli.short_flag == 'a');
    REQUIRE(dog_type.field("age").has_meta<spike_help_info>());
}

TEST_CASE("spike: metas before validators in same field call", "[validation][dynamic]") {
    // Order shouldn't matter — metas first, then validators
    auto config_type = type_def("Config")
        .field<std::string>("token", std::string("secret"),
            with<spike_help_info>({.summary = "API token"}),
            validators(not_empty{}, max_length{50}));

    auto config = config_type.create();
    REQUIRE(config.valid());

    // Meta still accessible
    REQUIRE(config.type().field("token").has_meta<spike_help_info>());
    REQUIRE(std::string_view{config.type().field("token").meta<spike_help_info>().summary} == "API token");

    // Validator still works
    config.set("token", std::string(""));
    REQUIRE(!config.valid());
}

// ═════════════════════════════════════════════════════════════════════════
// Custom user-defined validators
// ═════════════════════════════════════════════════════════════════════════

struct starts_with_uppercase {
    std::optional<std::string> operator()(const std::string& value) const {
        if (value.empty() || std::isupper(static_cast<unsigned char>(value[0])))
            return std::nullopt;
        return std::format("'{}' must start with an uppercase letter", value);
    }
};

struct in_range {
    int minimum;
    int maximum;
    std::optional<std::string> operator()(int value) const {
        if (value >= minimum && value <= maximum)
            return std::nullopt;
        return std::format("{} must be between {} and {}", value, minimum, maximum);
    }
};

TEST_CASE("spike: custom validator — basic contract", "[validation][dynamic][custom]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string("rex"),
            validators(starts_with_uppercase{}));

    auto dog = dog_type.create();
    REQUIRE(!dog.valid());

    auto result = dog.validate();
    REQUIRE(result.error_count() == 1);
    REQUIRE(result.errors()[0].path == "name");
    REQUIRE(result.errors()[0].constraint == "starts_with_uppercase");
    REQUIRE(result.errors()[0].message.find("uppercase") != std::string::npos);

    dog.set("name", std::string("Rex"));
    REQUIRE(dog.valid());
}

TEST_CASE("spike: custom validator with parameters", "[validation][dynamic][custom]") {
    auto dog_type = type_def("Dog")
        .field<int>("age", 0,
            validators(in_range{.minimum = 0, .maximum = 30}));

    auto dog = dog_type.create();
    REQUIRE(dog.valid());  // 0 is in range [0, 30]

    dog.set("age", 31);
    REQUIRE(!dog.valid());

    auto result = dog.validate();
    REQUIRE(result.errors()[0].constraint == "in_range");
    REQUIRE(result.errors()[0].message.find("30") != std::string::npos);

    dog.set("age", 15);
    REQUIRE(dog.valid());
}

TEST_CASE("spike: custom validators mixed with built-ins", "[validation][dynamic][custom]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}, starts_with_uppercase{}))
        .field<int>("age", 0,
            validators(positive{}, in_range{.minimum = 1, .maximum = 30}));

    auto dog = dog_type.create();

    // Empty name fails not_empty (starts_with_uppercase passes on empty by design)
    auto result = dog.validate();
    REQUIRE(result.error_count() >= 2);  // name: not_empty, age: positive + in_range overlap

    dog.set("name", std::string("Rex"));
    dog.set("age", 5);
    REQUIRE(dog.valid());
}

// ═════════════════════════════════════════════════════════════════════════
// Nested validation — dotted paths
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("spike: nested validation — inner field fails", "[validation][dynamic][nested]") {
    auto address_type = type_def("Address")
        .field<std::string>("street", std::string(""),
            validators(not_empty{}))
        .field<std::string>("zip", std::string(""),
            validators(not_empty{}));

    auto person_type = type_def("Person")
        .field<std::string>("name", std::string("Alice"))
        .field("address", address_type);

    auto person = person_type.create();
    // name is valid, but address.street and address.zip are both ""

    auto result = person.validate();
    REQUIRE(!result);
    REQUIRE(result.error_count() == 2);
    REQUIRE(result.errors()[0].path == "address.street");
    REQUIRE(result.errors()[1].path == "address.zip");
}

TEST_CASE("spike: nested validation — outer and inner both fail", "[validation][dynamic][nested]") {
    auto address_type = type_def("Address")
        .field<std::string>("zip", std::string(""),
            validators(not_empty{}));

    auto person_type = type_def("Person")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}))
        .field("address", address_type);

    auto person = person_type.create();

    auto result = person.validate();
    REQUIRE(result.error_count() == 2);

    // Outer field reported without prefix
    REQUIRE(result.errors()[0].path == "name");
    // Inner field reported with dotted prefix
    REQUIRE(result.errors()[1].path == "address.zip");
}

TEST_CASE("spike: nested validation — valid() short-circuits across nesting", "[validation][dynamic][nested]") {
    auto inner_type = type_def("Inner")
        .field<int>("value", 0, validators(positive{}));

    auto outer_type = type_def("Outer")
        .field<std::string>("label", std::string("ok"))
        .field("child", inner_type);

    auto outer = outer_type.create();
    REQUIRE(!outer.valid());  // child.value is 0, fails positive

    auto child = outer.get<type_instance>("child");
    child.set("value", 5);
    outer.set("child", child);
    REQUIRE(outer.valid());
}

TEST_CASE("spike: nested validation — 3-level deep dotted paths", "[validation][dynamic][nested]") {
    auto leaf_type = type_def("Leaf")
        .field<std::string>("tag", std::string(""),
            validators(not_empty{}));

    auto middle_type = type_def("Middle")
        .field<std::string>("label", std::string("ok"))
        .field("leaf", leaf_type);

    auto root_type = type_def("Root")
        .field<std::string>("name", std::string("root"))
        .field("middle", middle_type);

    auto root = root_type.create();

    auto result = root.validate();
    REQUIRE(result.error_count() == 1);
    REQUIRE(result.errors()[0].path == "middle.leaf.tag");
}

TEST_CASE("spike: nested validation — fixing nested value clears errors", "[validation][dynamic][nested]") {
    auto address_type = type_def("Address")
        .field<std::string>("street", std::string(""),
            validators(not_empty{}));

    auto person_type = type_def("Person")
        .field<std::string>("name", std::string("Alice"))
        .field("address", address_type);

    auto person = person_type.create();
    REQUIRE(!person.valid());

    auto address = person.get<type_instance>("address");
    address.set("street", std::string("123 Main St"));
    person.set("address", address);

    REQUIRE(person.valid());
    REQUIRE(person.validate().ok());
}

// ═════════════════════════════════════════════════════════════════════════
// parse_result — from_json with reporting
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("spike: parse() returns object and no issues on clean JSON", "[validation][dynamic][parse]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name")
        .field<int>("age");

    auto result = dog_type.parse(json{{"name", "Rex"}, {"age", 3}});

    REQUIRE(result.valid());
    REQUIRE(!result.has_extra_keys());
    REQUIRE(!result.has_missing_fields());
    REQUIRE(result->get<std::string>("name") == "Rex");
    REQUIRE(result->get<int>("age") == 3);
}

TEST_CASE("spike: parse() detects extra keys", "[validation][dynamic][parse]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name")
        .field<int>("age");

    auto result = dog_type.parse(json{
        {"name", "Rex"}, {"age", 3},
        {"color", "brown"}, {"weight", 25}
    });

    // Extra keys are informational — valid() still true
    REQUIRE(result.valid());
    REQUIRE(result.has_extra_keys());
    REQUIRE(result.extra_keys().size() == 2);

    // Object is still fully populated
    REQUIRE(result->get<std::string>("name") == "Rex");
}

TEST_CASE("spike: parse() detects missing fields", "[validation][dynamic][parse]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name")
        .field<int>("age", 5)
        .field<std::string>("breed");

    auto result = dog_type.parse(json{{"name", "Rex"}});

    // Missing fields are informational — valid() still true
    REQUIRE(result.valid());
    REQUIRE(result.has_missing_fields());
    REQUIRE(result.missing_fields().size() == 2);  // age and breed

    // Missing fields get defaults
    REQUIRE(result->get<int>("age") == 5);
    REQUIRE(result->get<std::string>("breed").empty());
}

TEST_CASE("spike: parse() reports validation errors", "[validation][dynamic][parse]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}))
        .field<int>("age", 0,
            validators(positive{}));

    auto result = dog_type.parse(json{{"name", ""}, {"age", -5}});

    // Validation errors make valid() false
    REQUIRE(!result.valid());
    REQUIRE(result.validation_errors().size() == 2);

    // But the object is still fully populated with the parsed values
    REQUIRE(result->get<std::string>("name") == "");
    REQUIRE(result->get<int>("age") == -5);
}

TEST_CASE("spike: parse() — all three categories at once", "[validation][dynamic][parse]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}))
        .field<int>("age")
        .field<std::string>("breed");

    auto result = dog_type.parse(json{
        {"name", ""},        // present but fails validation
        {"age", 3},          // present, no validator
        {"unknown", "wat"}   // extra key
        // breed is missing
    });

    REQUIRE(!result.valid());                    // validation error
    REQUIRE(result.has_extra_keys());            // "unknown"
    REQUIRE(result.has_missing_fields());         // "breed"
    REQUIRE(result.validation_errors().size() == 1);
    REQUIRE(result.extra_keys().size() == 1);
    REQUIRE(result.missing_fields().size() == 1);
}

TEST_CASE("spike: parse() — operator* and operator-> access", "[validation][dynamic][parse]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name")
        .field<int>("age");

    auto result = dog_type.parse(json{{"name", "Rex"}, {"age", 3}});

    // operator-> for field access
    REQUIRE(result->get<std::string>("name") == "Rex");

    // operator* for the instance itself
    type_instance& instance = *result;
    REQUIRE(instance.get<int>("age") == 3);
}

TEST_CASE("spike: parse() — checked_value() throws on validation errors", "[validation][dynamic][parse]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}));

    auto result = dog_type.parse(json{{"name", ""}});
    REQUIRE_THROWS(result.checked_value());

    auto good_result = dog_type.parse(json{{"name", "Rex"}});
    REQUIRE_NOTHROW(good_result.checked_value());
}

TEST_CASE("spike: parse() — validation does NOT run by default", "[validation][dynamic][parse]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}));

    // parse() should still populate validation_errors — it runs validation
    // But it doesn't THROW. The data is always returned.
    auto result = dog_type.parse(json{{"name", ""}});

    // valid() reflects the validation state
    REQUIRE(!result.valid());
    // But we got the object
    REQUIRE(result->get<std::string>("name") == "");
}

// ═════════════════════════════════════════════════════════════════════════
// Typed path — validators on field<T>
// ═════════════════════════════════════════════════════════════════════════

struct ValidatedDog;
struct ParseableDog;
struct HybridValidatedDog;

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<ValidatedDog>() {
    return def_type::field_info<ValidatedDog>("name", "age", "breed");
}

template <>
constexpr auto def_type::struct_info<ParseableDog>() {
    return def_type::field_info<ParseableDog>("name", "age", "breed");
}

template <>
constexpr auto def_type::struct_info<HybridValidatedDog>() {
    return def_type::field_info<HybridValidatedDog>("name", "age", "breed");
}
#endif

struct ValidatedDog {
    field<std::string> name {
        .value = "",
        .validators = validators(not_empty{})
    };
    field<int> age {
        .value = 0,
        .validators = validators(positive{})
    };
    field<std::string> breed;  // no validators
};

TEST_CASE("typed: valid() returns false when validators fail", "[validation][typed]") {
    ValidatedDog dog;
    type_def<ValidatedDog> dog_type;

    REQUIRE(!dog_type.valid(dog));
}

TEST_CASE("typed: valid() returns true when all pass", "[validation][typed]") {
    ValidatedDog dog;
    dog.name = "Rex";
    dog.age = 3;

    type_def<ValidatedDog> dog_type;
    REQUIRE(dog_type.valid(dog));
}

TEST_CASE("typed: validate() collects all errors", "[validation][typed]") {
    ValidatedDog dog;
    type_def<ValidatedDog> dog_type;

    auto result = dog_type.validate(dog);
    REQUIRE(!result);
    REQUIRE(result.error_count() == 2);  // name: not_empty, age: positive
    REQUIRE(result.errors()[0].path == "name");
    REQUIRE(result.errors()[1].path == "age");
}

TEST_CASE("typed: validate() reports constraint names", "[validation][typed]") {
    ValidatedDog dog;
    type_def<ValidatedDog> dog_type;

    auto result = dog_type.validate(dog);
    REQUIRE(result.errors()[0].constraint == "not_empty");
    REQUIRE(result.errors()[1].constraint == "positive");
}

TEST_CASE("typed: fields without validators always pass", "[validation][typed]") {
    ValidatedDog dog;
    dog.name = "Rex";
    dog.age = 3;
    // breed has no validators — doesn't affect valid()

    type_def<ValidatedDog> dog_type;
    REQUIRE(dog_type.valid(dog));
}

TEST_CASE("typed: fixing values makes validation pass", "[validation][typed]") {
    ValidatedDog dog;
    type_def<ValidatedDog> dog_type;

    REQUIRE(!dog_type.valid(dog));

    dog.name = "Rex";
    REQUIRE(!dog_type.valid(dog));  // age still 0

    dog.age = 5;
    REQUIRE(dog_type.valid(dog));

    auto result = dog_type.validate(dog);
    REQUIRE(result.ok());
}

// ═════════════════════════════════════════════════════════════════════════
// Hybrid path — validators on .field(&T::member, "name", validators(...))
// ═════════════════════════════════════════════════════════════════════════

struct HybridValidatedDog {
    std::string name;
    int         age = 0;
    std::string breed;
};

TEST_CASE("hybrid: valid() returns false when validators fail", "[validation][hybrid]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name",
            validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age",
            validators(positive{}))
        .field(&HybridValidatedDog::breed, "breed");

    HybridValidatedDog dog;
    REQUIRE(!dog_type.valid(dog));
}

TEST_CASE("hybrid: valid() returns true when all pass", "[validation][hybrid]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name",
            validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age",
            validators(positive{}));

    HybridValidatedDog dog;
    dog.name = "Rex";
    dog.age = 3;
    REQUIRE(dog_type.valid(dog));
}

TEST_CASE("hybrid: validate() collects all errors", "[validation][hybrid]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name",
            validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age",
            validators(positive{}));

    HybridValidatedDog dog;
    auto result = dog_type.validate(dog);

    REQUIRE(!result);
    REQUIRE(result.error_count() == 2);
    REQUIRE(result.errors()[0].path == "name");
    REQUIRE(result.errors()[0].constraint == "not_empty");
    REQUIRE(result.errors()[1].path == "age");
    REQUIRE(result.errors()[1].constraint == "positive");
}

TEST_CASE("hybrid: validators mixed with with<> metas", "[validation][hybrid]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name",
            validators(not_empty{}),
            with<spike_help_info>({.summary = "Dog's name"}))
        .field(&HybridValidatedDog::age, "age",
            with<spike_cli_meta>({.cli = {.short_flag = 'a'}}),
            validators(positive{}));

    HybridValidatedDog dog;
    REQUIRE(!dog_type.valid(dog));

    // Metas still accessible
    REQUIRE(dog_type.field("name").has_meta<spike_help_info>());
    REQUIRE(dog_type.field("age").has_meta<spike_cli_meta>());

    dog.name = "Rex";
    dog.age = 3;
    REQUIRE(dog_type.valid(dog));
}

TEST_CASE("hybrid: fixing values makes validation pass", "[validation][hybrid]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name",
            validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age",
            validators(positive{}));

    HybridValidatedDog dog;
    REQUIRE(!dog_type.valid(dog));

    dog.name = "Rex";
    dog.age = 5;
    REQUIRE(dog_type.valid(dog));
    REQUIRE(dog_type.validate(dog).ok());
}

// ═════════════════════════════════════════════════════════════════════════
// parse_result on typed path — parse<T>(json)
// ═════════════════════════════════════════════════════════════════════════

struct ParseableDog {
    field<std::string> name {
        .value = "",
        .validators = validators(not_empty{})
    };
    field<int> age {
        .value = 0,
        .validators = validators(positive{})
    };
    field<std::string> breed;
};

TEST_CASE("typed parse: clean JSON returns valid result", "[validation][typed][parse]") {
    auto result = type_def<ParseableDog>{}.parse(json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}});

    REQUIRE(result.valid());
    REQUIRE(!result.has_extra_keys());
    REQUIRE(!result.has_missing_fields());
    REQUIRE(result->name.value == "Rex");
    REQUIRE(result->age.value == 3);
}

TEST_CASE("typed parse: detects extra keys", "[validation][typed][parse]") {
    auto result = type_def<ParseableDog>{}.parse(json{
        {"name", "Rex"}, {"age", 3}, {"breed", "Husky"},
        {"color", "brown"}
    });

    REQUIRE(result.valid());
    REQUIRE(result.has_extra_keys());
    REQUIRE(result.extra_keys().size() == 1);
    REQUIRE(result.extra_keys()[0] == "color");
}

TEST_CASE("typed parse: detects missing fields", "[validation][typed][parse]") {
    auto result = type_def<ParseableDog>{}.parse(json{{"name", "Rex"}});

    // age defaults to 0, which fails positive{} — so valid() is false
    REQUIRE(!result.valid());
    REQUIRE(result.has_missing_fields());
    REQUIRE(result.missing_fields().size() == 2);  // age and breed
}

TEST_CASE("typed parse: reports validation errors", "[validation][typed][parse]") {
    auto result = type_def<ParseableDog>{}.parse(json{{"name", ""}, {"age", -5}, {"breed", "Husky"}});

    REQUIRE(!result.valid());
    REQUIRE(result.validation_errors().size() == 2);
    REQUIRE(result->name.value == "");   // value preserved
    REQUIRE(result->age.value == -5);    // value preserved
}

TEST_CASE("typed parse: all three categories at once", "[validation][typed][parse]") {
    auto result = type_def<ParseableDog>{}.parse(json{
        {"name", ""},          // fails not_empty
        {"unknown", "wat"}     // extra key
        // age and breed missing
    });

    REQUIRE(!result.valid());
    REQUIRE(result.has_extra_keys());
    REQUIRE(result.has_missing_fields());
    REQUIRE(result.validation_errors().size() >= 1);
}

TEST_CASE("typed parse: checked_value throws on invalid", "[validation][typed][parse]") {
    auto result = type_def<ParseableDog>{}.parse(json{{"name", ""}, {"age", 0}});
    REQUIRE_THROWS(result.checked_value());

    auto good = type_def<ParseableDog>{}.parse(json{{"name", "Rex"}, {"age", 3}});
    REQUIRE_NOTHROW(good.checked_value());
}

// ═════════════════════════════════════════════════════════════════════════
// parse_result on hybrid path — dog_t.parse(json)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("hybrid parse: clean JSON returns valid result", "[validation][hybrid][parse]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name", validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age", validators(positive{}))
        .field(&HybridValidatedDog::breed, "breed");

    auto result = dog_type.parse(json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}});

    REQUIRE(result.valid());
    REQUIRE(!result.has_extra_keys());
    REQUIRE(!result.has_missing_fields());
    REQUIRE(result->name == "Rex");
    REQUIRE(result->age == 3);
}

TEST_CASE("hybrid parse: detects extra keys", "[validation][hybrid][parse]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name")
        .field(&HybridValidatedDog::age, "age");

    auto result = dog_type.parse(json{{"name", "Rex"}, {"age", 3}, {"unknown", true}});

    REQUIRE(result.valid());
    REQUIRE(result.has_extra_keys());
    REQUIRE(result.extra_keys().size() == 1);
}

TEST_CASE("hybrid parse: detects missing fields", "[validation][hybrid][parse]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name", validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age")
        .field(&HybridValidatedDog::breed, "breed");

    auto result = dog_type.parse(json{{"name", "Rex"}});

    REQUIRE(result.valid());
    REQUIRE(result.has_missing_fields());
    REQUIRE(result.missing_fields().size() == 2);
}

TEST_CASE("hybrid parse: reports validation errors", "[validation][hybrid][parse]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name", validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age", validators(positive{}));

    auto result = dog_type.parse(json{{"name", ""}, {"age", -1}});

    REQUIRE(!result.valid());
    REQUIRE(result.validation_errors().size() == 2);
    REQUIRE(result->name == "");
    REQUIRE(result->age == -1);
}

// ═════════════════════════════════════════════════════════════════════════
// parse_options — configurable strictness
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic parse_options: reject_extra_keys throws", "[validation][dynamic][parse_options]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name")
        .field<int>("age");

    REQUIRE_THROWS_AS(
        dog_type.parse(json{{"name", "Rex"}, {"age", 3}, {"color", "brown"}},
            {.reject_extra_keys = true}),
        def_type::parse_error);
}

TEST_CASE("dynamic parse_options: reject_extra_keys does not throw when no extras", "[validation][dynamic][parse_options]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name")
        .field<int>("age");

    REQUIRE_NOTHROW(
        dog_type.parse(json{{"name", "Rex"}, {"age", 3}},
            {.reject_extra_keys = true}));
}

TEST_CASE("dynamic parse_options: require_all_fields throws on missing", "[validation][dynamic][parse_options]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name")
        .field<int>("age")
        .field<std::string>("breed");

    REQUIRE_THROWS_AS(
        dog_type.parse(json{{"name", "Rex"}},
            {.require_all_fields = true}),
        def_type::parse_error);
}

TEST_CASE("dynamic parse_options: require_valid throws on validation errors", "[validation][dynamic][parse_options]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}));

    REQUIRE_THROWS_AS(
        dog_type.parse(json{{"name", ""}},
            {.require_valid = true}),
        def_type::parse_error);
}

TEST_CASE("dynamic parse_options: strict sets all three", "[validation][dynamic][parse_options]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}))
        .field<int>("age");

    // Extra key + missing field + validation error
    REQUIRE_THROWS_AS(
        dog_type.parse(json{{"name", ""}, {"unknown", true}},
            {.strict = true}),
        def_type::parse_error);
}

TEST_CASE("dynamic parse_options: parse_error carries structured data", "[validation][dynamic][parse_options]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}))
        .field<int>("age");

    REQUIRE_THROWS_MATCHES(
        dog_type.parse(json{{"name", ""}, {"unknown", true}}, {.strict = true}),
        def_type::parse_error,
        Catch::Matchers::Predicate<def_type::parse_error>(
            [](const auto& error) {
                return error.extra_keys().size() == 1
                    && error.missing_fields().size() == 1
                    && error.validation_errors().size() == 1;
            },
            "has 1 extra key, 1 missing field, 1 validation error"));
}

TEST_CASE("dynamic parse_options: default options don't throw", "[validation][dynamic][parse_options]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string(""),
            validators(not_empty{}));

    // Default parse_options — no throwing, even with validation errors
    REQUIRE_NOTHROW(dog_type.parse(json{{"name", ""}, {"extra", 1}}));
}

// ═════════════════════════════════════════════════════════════════════════
// Type mismatch graceful handling — parse() defaults on bad types
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic parse: type mismatch defaults the field", "[validation][dynamic][parse][mismatch]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name", std::string("default_name"))
        .field<int>("age", 42);

    // "age" is a string in JSON but int in schema
    auto result = dog_type.parse(json{{"name", "Rex"}, {"age", "not a number"}});

    REQUIRE(result->get<std::string>("name") == "Rex");
    REQUIRE(result->get<int>("age") == 42);  // kept default
}

TEST_CASE("dynamic parse: type mismatch does not throw", "[validation][dynamic][parse][mismatch]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name")
        .field<int>("age");

    REQUIRE_NOTHROW(dog_type.parse(json{{"name", 999}, {"age", "nope"}}));
}

TEST_CASE("typed parse: type mismatch defaults the field", "[validation][typed][parse][mismatch]") {
    auto result = type_def<ParseableDog>{}.parse(json{
        {"name", "Rex"}, {"age", "not a number"}, {"breed", "Husky"}
    });

    REQUIRE(result->name.value == "Rex");
    REQUIRE(result->age.value == 0);  // default
    REQUIRE(result->breed.value == "Husky");
}

TEST_CASE("hybrid parse: type mismatch defaults the field", "[validation][hybrid][parse][mismatch]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name")
        .field(&HybridValidatedDog::age, "age")
        .field(&HybridValidatedDog::breed, "breed");

    auto result = dog_type.parse(json{
        {"name", "Rex"}, {"age", "not a number"}, {"breed", "Husky"}
    });

    REQUIRE(result->name == "Rex");
    REQUIRE(result->age == 0);  // default
    REQUIRE(result->breed == "Husky");
}

// ═════════════════════════════════════════════════════════════════════════
// Nested validation on typed path
// ═════════════════════════════════════════════════════════════════════════

struct ValidatedAddress {
    field<std::string> street {
        .value = "",
        .validators = validators(not_empty{})
    };
    field<std::string> zip {
        .value = "",
        .validators = validators(not_empty{})
    };
};

struct ValidatedPerson {
    field<std::string> name {
        .value = "",
        .validators = validators(not_empty{})
    };
    field<ValidatedAddress> address;
};

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<ValidatedAddress>() {
    return def_type::field_info<ValidatedAddress>("street", "zip");
}

template <>
constexpr auto def_type::struct_info<ValidatedPerson>() {
    return def_type::field_info<ValidatedPerson>("name", "address");
}
#endif

TEST_CASE("typed: nested validation — inner fields fail with dotted paths", "[validation][typed][nested]") {
    ValidatedPerson person;
    person.name = "Alice";
    // address.street and address.zip are both "" — should fail

    type_def<ValidatedPerson> person_type;
    auto result = person_type.validate(person);

    REQUIRE(!result);
    REQUIRE(result.error_count() == 2);
    REQUIRE(result.errors()[0].path == "address.street");
    REQUIRE(result.errors()[1].path == "address.zip");
}

TEST_CASE("typed: nested validation — all valid", "[validation][typed][nested]") {
    ValidatedPerson person;
    person.name = "Alice";
    person.address.value.street = "123 Main St";
    person.address.value.zip = "97201";

    type_def<ValidatedPerson> person_type;
    REQUIRE(person_type.valid(person));
}

TEST_CASE("typed: nested validation — outer and inner both fail", "[validation][typed][nested]") {
    ValidatedPerson person;
    // name is "" and address fields are ""

    type_def<ValidatedPerson> person_type;
    auto result = person_type.validate(person);

    REQUIRE(result.error_count() == 3);
    REQUIRE(result.errors()[0].path == "name");
    REQUIRE(result.errors()[1].path == "address.street");
    REQUIRE(result.errors()[2].path == "address.zip");
}

TEST_CASE("typed parse: nested field<> structs populate correctly", "[validation][typed][parse][nested]") {
    auto result = type_def<ValidatedPerson>{}.parse(json{
        {"name", "Alice"},
        {"address", {{"street", "123 Main"}, {"zip", "97201"}}}
    });

    REQUIRE(result->name.value == "Alice");
    REQUIRE(result->address.value.street.value == "123 Main");
    REQUIRE(result->address.value.zip.value == "97201");
    REQUIRE(result.valid());
}

TEST_CASE("typed parse: nested struct missing inner keys", "[validation][typed][parse][nested]") {
    auto result = type_def<ValidatedPerson>{}.parse(json{
        {"name", "Alice"},
        {"address", {{"street", "123 Main"}}}
        // zip missing from address
    });

    REQUIRE(result->name.value == "Alice");
    REQUIRE(result->address.value.street.value == "123 Main");
    REQUIRE(result->address.value.zip.value == "");  // default
}

TEST_CASE("typed parse: nested struct entirely missing", "[validation][typed][parse][nested]") {
    auto result = type_def<ValidatedPerson>{}.parse(json{
        {"name", "Alice"}
        // address entirely missing
    });

    REQUIRE(result->name.value == "Alice");
    REQUIRE(result->address.value.street.value == "");
    REQUIRE(result->address.value.zip.value == "");
}

TEST_CASE("typed parse: nested validation errors have dotted paths", "[validation][typed][parse][nested]") {
    auto result = type_def<ValidatedPerson>{}.parse(json{
        {"name", "Alice"},
        {"address", {{"street", ""}, {"zip", ""}}}
    });

    // name is valid, but address.street and address.zip fail not_empty
    REQUIRE(!result.valid());
    REQUIRE(result.validation_errors().size() == 2);
    REQUIRE(result.validation_errors()[0].path == "address.street");
    REQUIRE(result.validation_errors()[1].path == "address.zip");
}

TEST_CASE("typed parse: extra keys inside nested object", "[validation][typed][parse][nested]") {
    auto result = type_def<ValidatedPerson>{}.parse(json{
        {"name", "Alice"},
        {"address", {{"street", "123 Main"}, {"zip", "97201"}, {"country", "US"}}}
    });

    // "country" is an extra key inside the nested address
    REQUIRE(result->address.value.street.value == "123 Main");
}

TEST_CASE("typed parse: type mismatch in nested field", "[validation][typed][parse][nested]") {
    auto result = type_def<ValidatedPerson>{}.parse(json{
        {"name", "Alice"},
        {"address", {{"street", 42}, {"zip", "97201"}}}
        // street is int instead of string
    });

    REQUIRE(result->address.value.street.value == "");  // default on mismatch
    REQUIRE(result->address.value.zip.value == "97201");
}

// ── 3-level deep typed parse ────────────────────────────────────────────

struct ValidatedLeaf {
    field<std::string> tag {
        .value = "",
        .validators = validators(not_empty{})
    };
};

struct ValidatedMiddle {
    field<std::string> label {.value = "mid"};
    field<ValidatedLeaf> leaf;
};

struct ValidatedRoot {
    field<std::string> name {.value = "root"};
    field<ValidatedMiddle> middle;
};

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<ValidatedLeaf>() {
    return def_type::field_info<ValidatedLeaf>("tag");
}

template <>
constexpr auto def_type::struct_info<ValidatedMiddle>() {
    return def_type::field_info<ValidatedMiddle>("label", "leaf");
}

template <>
constexpr auto def_type::struct_info<ValidatedRoot>() {
    return def_type::field_info<ValidatedRoot>("name", "middle");
}
#endif

TEST_CASE("typed parse: 3-level deep nested populate", "[validation][typed][parse][nested]") {
    auto result = type_def<ValidatedRoot>{}.parse(json{
        {"name", "top"},
        {"middle", {{"label", "mid"}, {"leaf", {{"tag", "deep"}}}}}
    });

    REQUIRE(result->name.value == "top");
    REQUIRE(result->middle.value.label.value == "mid");
    REQUIRE(result->middle.value.leaf.value.tag.value == "deep");
    REQUIRE(result.valid());
}

TEST_CASE("typed parse: 3-level deep validation error has full dotted path", "[validation][typed][parse][nested]") {
    auto result = type_def<ValidatedRoot>{}.parse(json{
        {"name", "top"},
        {"middle", {{"label", "mid"}, {"leaf", {{"tag", ""}}}}}
    });

    REQUIRE(!result.valid());
    REQUIRE(result.validation_errors().size() == 1);
    REQUIRE(result.validation_errors()[0].path == "middle.leaf.tag");
}

TEST_CASE("typed parse: 3-level deep entirely missing nested", "[validation][typed][parse][nested]") {
    auto result = type_def<ValidatedRoot>{}.parse(json{
        {"name", "top"}
        // middle entirely missing
    });

    REQUIRE(result->name.value == "top");
    REQUIRE(result->middle.value.label.value == "mid");  // default
    REQUIRE(result->middle.value.leaf.value.tag.value == "");  // default
}

// ═════════════════════════════════════════════════════════════════════════
// parse_options on typed path
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed parse_options: reject_extra_keys throws", "[validation][typed][parse_options]") {
    REQUIRE_THROWS_AS(
        type_def<ParseableDog>{}.parse(
            json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}, {"color", "brown"}},
            {.reject_extra_keys = true}),
        def_type::parse_error);
}

TEST_CASE("typed parse_options: reject_extra_keys does not throw when no extras", "[validation][typed][parse_options]") {
    REQUIRE_NOTHROW(
        type_def<ParseableDog>{}.parse(
            json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}},
            {.reject_extra_keys = true}));
}

TEST_CASE("typed parse_options: require_all_fields throws on missing", "[validation][typed][parse_options]") {
    REQUIRE_THROWS_AS(
        type_def<ParseableDog>{}.parse(
            json{{"name", "Rex"}},
            {.require_all_fields = true}),
        def_type::parse_error);
}

TEST_CASE("typed parse_options: require_all_fields does not throw when all present", "[validation][typed][parse_options]") {
    REQUIRE_NOTHROW(
        type_def<ParseableDog>{}.parse(
            json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}},
            {.require_all_fields = true}));
}

TEST_CASE("typed parse_options: require_valid throws on validation errors", "[validation][typed][parse_options]") {
    REQUIRE_THROWS_AS(
        type_def<ParseableDog>{}.parse(
            json{{"name", ""}, {"age", -5}, {"breed", "Husky"}},
            {.require_valid = true}),
        def_type::parse_error);
}

TEST_CASE("typed parse_options: require_valid does not throw when valid", "[validation][typed][parse_options]") {
    REQUIRE_NOTHROW(
        type_def<ParseableDog>{}.parse(
            json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}},
            {.require_valid = true}));
}

TEST_CASE("typed parse_options: strict sets all three", "[validation][typed][parse_options]") {
    REQUIRE_THROWS_AS(
        type_def<ParseableDog>{}.parse(
            json{{"name", ""}, {"unknown", true}},
            {.strict = true}),
        def_type::parse_error);
}

TEST_CASE("typed parse_options: strict does not throw on clean input", "[validation][typed][parse_options]") {
    REQUIRE_NOTHROW(
        type_def<ParseableDog>{}.parse(
            json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}},
            {.strict = true}));
}

TEST_CASE("typed parse_options: parse_error carries structured data", "[validation][typed][parse_options]") {
    REQUIRE_THROWS_MATCHES(
        type_def<ParseableDog>{}.parse(
            json{{"name", ""}, {"unknown", true}}, {.strict = true}),
        def_type::parse_error,
        Catch::Matchers::Predicate<def_type::parse_error>(
            [](const auto& error) {
                return error.extra_keys().size() == 1
                    && error.extra_keys()[0] == "unknown"
                    && error.missing_fields().size() == 2
                    && error.validation_errors().size() >= 1;
            },
            "has 1 extra key 'unknown', 2 missing fields, >=1 validation error"));
}

// ═════════════════════════════════════════════════════════════════════════
// parse_options on hybrid path
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("hybrid parse_options: reject_extra_keys throws", "[validation][hybrid][parse_options]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name")
        .field(&HybridValidatedDog::age, "age");

    REQUIRE_THROWS_AS(
        dog_type.parse(json{{"name", "Rex"}, {"age", 3}, {"unknown", true}},
            {.reject_extra_keys = true}),
        def_type::parse_error);
}

TEST_CASE("hybrid parse_options: reject_extra_keys does not throw when no extras", "[validation][hybrid][parse_options]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name")
        .field(&HybridValidatedDog::age, "age");

    REQUIRE_NOTHROW(
        dog_type.parse(json{{"name", "Rex"}, {"age", 3}},
            {.reject_extra_keys = true}));
}

TEST_CASE("hybrid parse_options: require_all_fields throws on missing", "[validation][hybrid][parse_options]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name")
        .field(&HybridValidatedDog::age, "age")
        .field(&HybridValidatedDog::breed, "breed");

    REQUIRE_THROWS_AS(
        dog_type.parse(json{{"name", "Rex"}},
            {.require_all_fields = true}),
        def_type::parse_error);
}

TEST_CASE("hybrid parse_options: require_valid throws on validation errors", "[validation][hybrid][parse_options]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name", validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age");

    REQUIRE_THROWS_AS(
        dog_type.parse(json{{"name", ""}, {"age", 3}},
            {.require_valid = true}),
        def_type::parse_error);
}

TEST_CASE("hybrid parse_options: strict throws on any issue", "[validation][hybrid][parse_options]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name", validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age");

    REQUIRE_THROWS_AS(
        dog_type.parse(json{{"name", ""}, {"unknown", 1}},
            {.strict = true}),
        def_type::parse_error);
}

TEST_CASE("hybrid parse_options: strict does not throw on clean input", "[validation][hybrid][parse_options]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name", validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age");

    REQUIRE_NOTHROW(
        dog_type.parse(json{{"name", "Rex"}, {"age", 3}},
            {.strict = true}));
}

TEST_CASE("hybrid parse_options: parse_error carries structured data", "[validation][hybrid][parse_options]") {
    auto dog_type = type_def<HybridValidatedDog>()
        .field(&HybridValidatedDog::name, "name", validators(not_empty{}))
        .field(&HybridValidatedDog::age, "age");

    REQUIRE_THROWS_MATCHES(
        dog_type.parse(json{{"name", ""}, {"unknown", true}}, {.strict = true}),
        def_type::parse_error,
        Catch::Matchers::Predicate<def_type::parse_error>(
            [](const auto& error) {
                return error.extra_keys().size() == 1
                    && error.missing_fields().size() == 1
                    && error.validation_errors().size() == 1;
            },
            "has 1 extra key, 1 missing field, 1 validation error"));
}

// ═════════════════════════════════════════════════════════════════════════
// Dynamic builder: validators without default value
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic: validators without default — compiles and validates", "[validation][dynamic]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name",
            validators(not_empty{}))
        .field<int>("age",
            validators(positive{}));

    auto dog = dog_type.create();
    REQUIRE(!dog.valid());

    dog.set("name", std::string("Rex"));
    dog.set("age", 3);
    REQUIRE(dog.valid());
}

TEST_CASE("dynamic: validators without default — validate returns errors", "[validation][dynamic]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name",
            validators(not_empty{}));

    auto dog = dog_type.create();
    auto result = dog.validate();
    REQUIRE(result.error_count() == 1);
    REQUIRE(result.errors()[0].path == "name");
    REQUIRE(result.errors()[0].constraint == "not_empty");
}

TEST_CASE("dynamic: validators without default — parse works", "[validation][dynamic][parse]") {
    auto dog_type = type_def("Dog")
        .field<std::string>("name",
            validators(not_empty{}));

    auto result = dog_type.parse(json{{"name", "Rex"}});
    REQUIRE(result.valid());
    REQUIRE(result->get<std::string>("name") == "Rex");
}

// ═════════════════════════════════════════════════════════════════════════
// Typed path: validators + metas mixed on same field<T>
// ═════════════════════════════════════════════════════════════════════════

struct typed_validation_help {
    const char* summary = "";
};

struct typed_validation_cli {
    struct { char short_flag = '\0'; } cli;
};

struct TypedMixedDog {
    field<std::string, with<typed_validation_help>> name {
        .with = {{.summary = "Dog's name"}},
        .value = "",
        .validators = validators(not_empty{})
    };
    field<int, with<typed_validation_cli, typed_validation_help>> age {
        .with = {{.cli = {.short_flag = 'a'}}, {.summary = "Age in years"}},
        .value = 0,
        .validators = validators(positive{})
    };
    field<std::string> breed;
};

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<TypedMixedDog>() {
    return def_type::field_info<TypedMixedDog>("name", "age", "breed");
}
#endif

TEST_CASE("typed: validators + metas on same field — validators work", "[validation][typed]") {
    TypedMixedDog dog;
    type_def<TypedMixedDog> dog_type;

    REQUIRE(!dog_type.valid(dog));

    dog.name = "Rex";
    dog.age = 3;
    REQUIRE(dog_type.valid(dog));
}

TEST_CASE("typed: validators + metas on same field — validate collects errors", "[validation][typed]") {
    TypedMixedDog dog;
    type_def<TypedMixedDog> dog_type;

    auto result = dog_type.validate(dog);
    REQUIRE(result.error_count() == 2);
    REQUIRE(result.errors()[0].path == "name");
    REQUIRE(result.errors()[1].path == "age");
}

TEST_CASE("typed: validators + metas on same field — metas accessible", "[validation][typed]") {
    type_def<TypedMixedDog> dog_type;

    // Use field() query API which is type-erased and safe
    REQUIRE(dog_type.field("name").has_meta<typed_validation_help>());
    REQUIRE(std::string_view{dog_type.field("name").meta<typed_validation_help>().summary} == "Dog's name");
    REQUIRE(dog_type.field("age").has_meta<typed_validation_cli>());
    REQUIRE(dog_type.field("age").meta<typed_validation_cli>().cli.short_flag == 'a');
    REQUIRE(dog_type.field("age").has_meta<typed_validation_help>());
    REQUIRE(std::string_view{dog_type.field("age").meta<typed_validation_help>().summary} == "Age in years");
}

TEST_CASE("typed: validators + metas on same field — parse works", "[validation][typed][parse]") {
    auto result = type_def<TypedMixedDog>{}.parse(
        json{{"name", ""}, {"age", -1}, {"breed", "Husky"}});

    REQUIRE(!result.valid());
    REQUIRE(result.validation_errors().size() == 2);
    REQUIRE(result->name.value == "");
    REQUIRE(result->age.value == -1);
}
