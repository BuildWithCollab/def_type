#include "test_model_types.hpp"

// ═════════════════════════════════════════════════════════════════════════
// Field with single meta
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field detects field-level metas", "[type_def][typed][field_meta]") {
    bool found_verbose_cli = false;
    bool found_limit_cli = false;
    bool found_limit_render = false;

    type_def<CliArgs>{}.for_each_field([&](auto descriptor) {
        if (descriptor.name() == "verbose") {
            found_verbose_cli = descriptor.template has_meta<cli_meta>();
        }
        if (descriptor.name() == "limit") {
            found_limit_cli = descriptor.template has_meta<cli_meta>();
            found_limit_render = descriptor.template has_meta<render_meta>();
        }
    });

    REQUIRE(found_verbose_cli);
    REQUIRE(found_limit_cli);
    REQUIRE(found_limit_render);
}

TEST_CASE("hybrid: for_each_field detects field-level metas", "[type_def][hybrid][field_meta]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name",
            with<help_info>({.summary = "Dog's name"}))
        .field(&PlainDog::age, "age",
            with<render_meta>({.render = {.style = "bold", .width = 5}}))
        .field(&PlainDog::breed, "breed");

    bool name_has_help = false;
    bool age_has_render = false;
    bool breed_has_help = false;
    bool breed_has_render = false;

    t.for_each_field([&](auto descriptor) {
        if (descriptor.name() == "name")
            name_has_help = descriptor.template has_meta<help_info>();
        if (descriptor.name() == "age")
            age_has_render = descriptor.template has_meta<render_meta>();
        if (descriptor.name() == "breed") {
            breed_has_help = descriptor.template has_meta<help_info>();
            breed_has_render = descriptor.template has_meta<render_meta>();
        }
    });

    REQUIRE(name_has_help);
    REQUIRE(age_has_render);
    REQUIRE(!breed_has_help);
    REQUIRE(!breed_has_render);
}

TEST_CASE("dynamic: for_each_field detects field-level metas", "[type_def][dynamic][field_meta]") {
    auto t = type_def("CLI")
        .field<std::string>("query")
        .field<bool>("verbose", false,
            with<cli_meta>({.cli = {.short_flag = 'v'}}))
        .field<int>("limit", 10,
            with<cli_meta>({.cli = {.short_flag = 'l'}}),
            with<render_meta>({.render = {.style = "bold", .width = 10}}));

    bool query_has_cli = true;
    bool verbose_has_cli = false;
    bool limit_has_cli = false;
    bool limit_has_render = false;

    t.for_each_field([&](auto descriptor) {
        if (descriptor.name() == "query")
            query_has_cli = descriptor.template has_meta<cli_meta>();
        if (descriptor.name() == "verbose")
            verbose_has_cli = descriptor.template has_meta<cli_meta>();
        if (descriptor.name() == "limit") {
            limit_has_cli = descriptor.template has_meta<cli_meta>();
            limit_has_render = descriptor.template has_meta<render_meta>();
        }
    });

    REQUIRE(!query_has_cli);
    REQUIRE(verbose_has_cli);
    REQUIRE(limit_has_cli);
    REQUIRE(limit_has_render);
}

TEST_CASE("typed: field() with single meta via field_def", "[type_def][typed][field_meta]") {
    auto fv = type_def<CliArgs>{}.field("verbose");
    REQUIRE(fv.has_meta<cli_meta>());
    REQUIRE(fv.meta<cli_meta>().cli.short_flag == 'v');
}

TEST_CASE("typed: field() with multiple metas via field_def", "[type_def][typed][field_meta]") {
    auto fv = type_def<CliArgs>{}.field("limit");
    REQUIRE(fv.has_meta<cli_meta>());
    REQUIRE(fv.has_meta<render_meta>());
    REQUIRE(fv.meta<cli_meta>().cli.short_flag == 'l');
    REQUIRE(std::string_view{fv.meta<render_meta>().render.style} == "bold");
    REQUIRE(fv.meta<render_meta>().render.width == 10);
}

TEST_CASE("typed: field() without meta via field_def", "[type_def][typed][field_meta]") {
    auto fv = type_def<CliArgs>{}.field("query");
    REQUIRE(!fv.has_meta<cli_meta>());
    REQUIRE(!fv.has_meta<render_meta>());
}

TEST_CASE("hybrid: field with meta", "[type_def][hybrid][field_meta]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name",
            with<help_info>({.summary = "Dog's name"}));

    REQUIRE(t.field("name").has_meta<help_info>());
    REQUIRE(std::string_view{t.field("name").meta<help_info>().summary} == "Dog's name");
}

TEST_CASE("dynamic: field with single meta", "[type_def][dynamic][field_meta]") {
    auto t = type_def("CLI")
        .field<bool>("verbose", false,
            with<cli_meta>({.cli = {.short_flag = 'v'}}));

    REQUIRE(t.field("verbose").has_meta<cli_meta>());
    REQUIRE(t.field("verbose").meta<cli_meta>().cli.short_flag == 'v');
}

// ═════════════════════════════════════════════════════════════════════════
// Field with multiple metas
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("hybrid: field with multiple metas", "[type_def][hybrid][field_meta]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::age, "age",
            with<help_info>({.summary = "Age in years"}),
            with<render_meta>({.render = {.style = "bold", .width = 5}}));

    REQUIRE(t.field("age").has_meta<help_info>());
    REQUIRE(t.field("age").has_meta<render_meta>());
    REQUIRE(std::string_view{t.field("age").meta<help_info>().summary} == "Age in years");
    REQUIRE(std::string_view{t.field("age").meta<render_meta>().render.style} == "bold");
    REQUIRE(t.field("age").meta<render_meta>().render.width == 5);
}

TEST_CASE("dynamic: field with multiple metas", "[type_def][dynamic][field_meta]") {
    auto t = type_def("CLI")
        .field<bool>("verbose", false,
            with<cli_meta>({.cli = {.short_flag = 'v'}}),
            with<render_meta>({.render = {.style = "dim", .width = 5}}));

    REQUIRE(t.field("verbose").has_meta<cli_meta>());
    REQUIRE(t.field("verbose").has_meta<render_meta>());
    REQUIRE(t.field("verbose").meta<cli_meta>().cli.short_flag == 'v');
    REQUIRE(std::string_view{t.field("verbose").meta<render_meta>().render.style} == "dim");
    REQUIRE(t.field("verbose").meta<render_meta>().render.width == 5);
}

// ═════════════════════════════════════════════════════════════════════════
// Field without meta
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field — query has no meta", "[type_def][typed][field_meta]") {
    bool query_has_cli = true;

    type_def<CliArgs>{}.for_each_field([&](auto descriptor) {
        if (descriptor.name() == "query") {
            query_has_cli = descriptor.template has_meta<cli_meta>();
        }
    });

    REQUIRE(!query_has_cli);
}

TEST_CASE("hybrid: field without meta", "[type_def][hybrid][field_meta]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");

    REQUIRE(!t.field("name").has_meta<help_info>());
}

TEST_CASE("dynamic: field without meta returns false", "[type_def][dynamic][field_meta]") {
    auto t = type_def("Event")
        .field<std::string>("title");

    REQUIRE(!t.field("title").has_meta<cli_meta>());
}

// ═════════════════════════════════════════════════════════════════════════
// meta_count and metas on fields
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: field has_meta and meta via for_each_field", "[type_def][typed][field_meta]") {
    bool found_limit = false;
    bool limit_has_cli = false;
    bool limit_has_render = false;
    char limit_short_flag = '\0';

    type_def<CliArgs>{}.for_each_field([&](auto descriptor) {
        if constexpr (requires { descriptor.index(); }) {
            if (descriptor.name() == "limit") {
                found_limit = true;
                limit_has_cli = descriptor.template has_meta<cli_meta>();
                limit_has_render = descriptor.template has_meta<render_meta>();
                if constexpr (descriptor.template has_meta<cli_meta>()) {
                    limit_short_flag = descriptor.template meta<cli_meta>().cli.short_flag;
                }
            }
        }
    });

    REQUIRE(found_limit);
    REQUIRE(limit_has_cli);
    REQUIRE(limit_has_render);
    REQUIRE(limit_short_flag == 'l');
}

TEST_CASE("typed: field meta_count via field_def", "[type_def][typed][field_meta]") {
    auto limit_fv = type_def<CliArgs>{}.field("limit");
    REQUIRE(limit_fv.meta_count<cli_meta>() == 1);
    REQUIRE(limit_fv.meta_count<render_meta>() == 1);

    auto query_fv = type_def<CliArgs>{}.field("query");
    REQUIRE(query_fv.meta_count<cli_meta>() == 0);

    auto verbose_fv = type_def<CliArgs>{}.field("verbose");
    REQUIRE(verbose_fv.meta_count<cli_meta>() == 1);
    REQUIRE(verbose_fv.meta_count<render_meta>() == 0);
}

TEST_CASE("typed: field metas<M>() via field_def", "[type_def][typed][field_meta]") {
    type_def<CliArgs> t;
    auto cli_metas = t.field("limit").metas<cli_meta>();
    REQUIRE(cli_metas.size() == 1);
    REQUIRE(cli_metas[0].cli.short_flag == 'l');

    auto render_metas = t.field("limit").metas<render_meta>();
    REQUIRE(render_metas.size() == 1);
    REQUIRE(std::string_view{render_metas[0].render.style} == "bold");

    auto empty = t.field("query").metas<cli_meta>();
    REQUIRE(empty.empty());
}

TEST_CASE("hybrid: field meta_count and metas", "[type_def][hybrid][field_meta]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name",
            with<tag_info>({.value = "a"}),
            with<tag_info>({.value = "b"}));

    REQUIRE(t.field("name").meta_count<tag_info>() == 2);
    auto tags = t.field("name").metas<tag_info>();
    REQUIRE(tags.size() == 2);
    REQUIRE(std::string_view{tags[0].value} == "a");
    REQUIRE(std::string_view{tags[1].value} == "b");
}

TEST_CASE("dynamic: field meta_count and metas", "[type_def][dynamic][field_meta]") {
    auto t = type_def("Event")
        .field<std::string>("title", std::string(""),
            with<tag_info>({.value = "a"}),
            with<tag_info>({.value = "b"}));

    REQUIRE(t.field("title").meta_count<tag_info>() == 2);
    auto tags = t.field("title").metas<tag_info>();
    REQUIRE(tags.size() == 2);
    REQUIRE(std::string_view{tags[0].value} == "a");
    REQUIRE(std::string_view{tags[1].value} == "b");
}

TEST_CASE("hybrid: field meta_count for absent meta", "[type_def][hybrid][field_meta]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name",
            with<help_info>({.summary = "x"}));

    REQUIRE(t.field("name").meta_count<cli_meta>() == 0);
}

TEST_CASE("dynamic: field meta_count for absent meta", "[type_def][dynamic][field_meta]") {
    auto t = type_def("CLI")
        .field<bool>("verbose", false,
            with<cli_meta>({.cli = {.short_flag = 'v'}}));

    REQUIRE(t.field("verbose").meta_count<render_meta>() == 0);
}

TEST_CASE("type_instance: field-level meta_count and metas via type()", "[type_instance][field_meta]") {
    auto t = type_def("CLI")
        .field<bool>("verbose", false,
            with<cli_meta>({.cli = {.short_flag = 'v'}}))
        .field<std::string>("query");
    auto obj = t.create();

    REQUIRE(obj.type().field("verbose").meta_count<cli_meta>() == 1);
    REQUIRE(obj.type().field("verbose").metas<cli_meta>()[0].cli.short_flag == 'v');
    REQUIRE(obj.type().field("query").meta_count<cli_meta>() == 0);
}

TEST_CASE("type_instance: field-level meta via type().field()", "[type_instance][field_meta]") {
    auto t = type_def("CLI")
        .field<bool>("verbose", false,
            with<cli_meta>({.cli = {.short_flag = 'v'}}))
        .field<std::string>("query");

    auto obj = t.create();

    REQUIRE(obj.type().field("verbose").has_meta<cli_meta>() == true);
    REQUIRE(obj.type().field("verbose").meta<cli_meta>().cli.short_flag == 'v');
    REQUIRE(obj.type().field("query").has_meta<cli_meta>() == false);
}

// ═════════════════════════════════════════════════════════════════════════
// for_each_field reads meta values
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field reads meta values via meta<M>()", "[type_def][typed][field_meta]") {
    char verbose_flag = '\0';

    type_def<CliArgs>{}.for_each_field([&](auto descriptor) {
        if constexpr (requires { descriptor.index(); }) {
            if (descriptor.name() == "verbose") {
                if constexpr (descriptor.template has_meta<cli_meta>()) {
                    verbose_flag = descriptor.template meta<cli_meta>().cli.short_flag;
                }
            }
        }
    });

    REQUIRE(verbose_flag == 'v');
}

TEST_CASE("hybrid: for_each_field reads meta values", "[type_def][hybrid][field_meta]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name",
            with<help_info>({.summary = "Dog's name"}));

    std::string summary;
    t.for_each_field([&](auto descriptor) {
        if (descriptor.name() == "name" && descriptor.template has_meta<help_info>())
            summary = descriptor.template meta<help_info>().summary;
    });

    REQUIRE(summary == "Dog's name");
}

TEST_CASE("dynamic: for_each_field reads meta values", "[type_def][dynamic][field_meta]") {
    auto t = type_def("CLI")
        .field<bool>("verbose", false,
            with<cli_meta>({.cli = {.short_flag = 'v'}}));

    char short_flag = '\0';
    t.for_each_field([&](auto descriptor) {
        if (descriptor.name() == "verbose" && descriptor.template has_meta<cli_meta>())
            short_flag = descriptor.template meta<cli_meta>().cli.short_flag;
    });

    REQUIRE(short_flag == 'v');
}

// ═════════════════════════════════════════════════════════════════════════
// field_def meta() throws for absent meta
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic: field_def meta() throws for absent meta", "[type_def][dynamic][field_meta][throw]") {
    auto t = type_def("Event")
        .field<std::string>("title");

    REQUIRE_THROWS_AS(t.field("title").meta<cli_meta>(), std::logic_error);
}

TEST_CASE("typed: field_def meta() throws for absent meta", "[type_def][typed][field_meta][throw]") {
    REQUIRE_THROWS_AS(type_def<CliArgs>{}.field("query").meta<cli_meta>(), std::logic_error);
}

TEST_CASE("hybrid: field_def meta() throws for absent meta", "[type_def][hybrid][field_meta][throw]") {
    auto t = type_def<PlainDog>()
        .field(&PlainDog::name, "name");

    REQUIRE_THROWS_AS(t.field("name").meta<cli_meta>(), std::logic_error);
}

TEST_CASE("type_instance: field_def meta() throws for absent meta", "[type_instance][field_meta][throw]") {
    auto t = type_def("Event")
        .field<std::string>("title");
    auto obj = t.create();

    REQUIRE_THROWS_AS(obj.type().field("title").meta<cli_meta>(), std::logic_error);
}
