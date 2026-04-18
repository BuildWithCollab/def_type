#include "test_model_types.hpp"

// ═════════════════════════════════════════════════════════════════════════
// field<type_def<>> — nesting a runtime-defined type inside a struct
// ═════════════════════════════════════════════════════════════════════════

// ── Test types ──────────────────────────────────────────────────────────

inline auto field_typedef_details = type_def("Details")
    .field<std::string>("note")
    .field<int>("priority", 5);

struct FieldTypeDefContainer {
    field<std::string>  name;
    field<type_def<>>   details {field_typedef_details};
};

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<FieldTypeDefContainer>() {
    return def_type::field_info<FieldTypeDefContainer>("name", "details");
}
#endif

// ── Construction and access ─────────────────────────────────────────────

TEST_CASE("field<type_def<>>: struct is constructible", "[field][type_def][dynamic]") {
    FieldTypeDefContainer container;
    REQUIRE(container.name.value.empty());
    REQUIRE(container.details.value.get<int>("priority") == 5);
}

TEST_CASE("field<type_def<>>: access nested fields via operator->", "[field][type_def][dynamic]") {
    FieldTypeDefContainer container;
    container.name = "parent";
    container.details->set("note", std::string("hello"));
    container.details->set("priority", 10);

    REQUIRE(container.name.value == "parent");
    REQUIRE(container.details->get<std::string>("note") == "hello");
    REQUIRE(container.details->get<int>("priority") == 10);
}

TEST_CASE("field<type_def<>>: defaults from nested type_def", "[field][type_def][dynamic]") {
    FieldTypeDefContainer container;
    REQUIRE(container.details->get<std::string>("note") == "");
    REQUIRE(container.details->get<int>("priority") == 5);
}

TEST_CASE("field<type_def<>>: multiple instances are independent", "[field][type_def][dynamic]") {
    FieldTypeDefContainer a;
    FieldTypeDefContainer b;

    a.details->set("note", std::string("A"));
    b.details->set("note", std::string("B"));

    REQUIRE(a.details->get<std::string>("note") == "A");
    REQUIRE(b.details->get<std::string>("note") == "B");
}

// ── Serialization ───────────────────────────────────────────────────────

TEST_CASE("field<type_def<>>: nested instance produces valid JSON string", "[field][type_def][dynamic][to_json]") {
    FieldTypeDefContainer container;
    container.details->set("note", std::string("hello"));
    container.details->set("priority", 10);

    auto json_string = container.details.value.to_json_string();

    REQUIRE(json_string.find("\"note\":\"hello\"") != std::string::npos);
    REQUIRE(json_string.find("\"priority\":10") != std::string::npos);
}

// ── with<> extensions ───────────────────────────────────────────────────

TEST_CASE("field<type_def<>>: single with<> extension", "[field][type_def][dynamic][with]") {
    struct WithContainer {
        field<std::string> name;
        field<type_def<>, with<cli_meta>> details {
            with<cli_meta>{{.cli = {.short_flag = 'd'}}},
            field_typedef_details
        };
    };

    WithContainer container;
    REQUIRE(container.details.with.cli.short_flag == 'd');
    REQUIRE(container.details.value.get<int>("priority") == 5);
    container.details->set("note", std::string("with works"));
    REQUIRE(container.details->get<std::string>("note") == "with works");
}

TEST_CASE("field<type_def<>>: multiple with<> extensions", "[field][type_def][dynamic][with]") {
    struct MultiWithContainer {
        field<std::string> name;
        field<type_def<>, with<cli_meta, render_meta>> details {
            with<cli_meta, render_meta>{
                {.cli = {.short_flag = 'd'}},
                {.render = {.style = "bold", .width = 10}}
            },
            field_typedef_details
        };
    };

    MultiWithContainer container;
    REQUIRE(container.details.with.cli.short_flag == 'd');
    REQUIRE(std::string_view{container.details.with.render.style} == "bold");
    REQUIRE(container.details.with.render.width == 10);
    REQUIRE(container.details.value.get<int>("priority") == 5);
}

// ── Deep nesting ────────────────────────────────────────────────────────

TEST_CASE("field<type_def<>>: 3-level deep nesting in structs", "[field][type_def][dynamic][nesting]") {
    auto leaf_type = type_def("Leaf")
        .field<std::string>("value", std::string("deep"));

    auto middle_type = type_def("Middle")
        .field<std::string>("label", std::string("mid"))
        .field("leaf", leaf_type);

    struct MiddleHolder {
        field<std::string>  name;
        field<type_def<>>   middle;

        MiddleHolder(const type_def<>& middle_typedef) : middle(middle_typedef) {}
    };

    MiddleHolder holder(middle_type);
    holder.name = "top";

    REQUIRE(holder.middle->get<std::string>("label") == "mid");
    auto leaf = holder.middle->get<type_instance>("leaf");
    REQUIRE(leaf.get<std::string>("value") == "deep");
}

