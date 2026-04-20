#include "test_model_types.hpp"

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field iterates fields with name and value
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field iterates field<> members with name and value", "[type_def][typed][for_each_field]") {
    SimpleArgs args;
    args.name = "Alice";
    args.age = 30;
    args.active = true;

    std::vector<std::string> collected_names;
    type_def<SimpleArgs>{}.for_each_field(args, [&](std::string_view name, auto& value) {
        collected_names.emplace_back(name);
    });

    REQUIRE(collected_names.size() == 3);
    REQUIRE(collected_names[0] == "name");
    REQUIRE(collected_names[1] == "age");
    REQUIRE(collected_names[2] == "active");
}

TEST_CASE("hybrid: for_each_field() iterates all non-meta members with typed refs", "[type_def][hybrid][for_each_field]") {
    type_def<PlainDog> t;

    PlainDog rex{"Rex", 3, "Husky"};

    std::vector<std::string> names;
    t.for_each_field(rex, [&](std::string_view name, auto& value) {
        names.emplace_back(name);
    });

    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "age");
    REQUIRE(names[2] == "breed");
}

TEST_CASE("type_instance: for_each_field() iterates all fields", "[type_instance][for_each_field]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 100);
    auto obj = t.create();
    obj.set("title", std::string("Party"));

    std::vector<std::string> names;
    obj.for_each_field([&](std::string_view name, field_value) {
        names.emplace_back(name);
    });

    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "title");
    REQUIRE(names[1] == "count");
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field provides typed value access
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field provides access to field values", "[type_def][typed][for_each_field]") {
    SimpleArgs args;
    args.name = "Bob";
    args.age = 25;
    args.active = false;

    std::string found_name;
    int found_age = 0;

    type_def<SimpleArgs>{}.for_each_field(args, [&](std::string_view name, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, std::string>) {
            if (name == "name") found_name = value;
        } else if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, int>) {
            if (name == "age") found_age = value;
        }
    });

    REQUIRE(found_name == "Bob");
    REQUIRE(found_age == 25);
}

TEST_CASE("hybrid: for_each_field() provides real typed values", "[type_def][hybrid][for_each_field]") {
    type_def<PlainDog> t;

    PlainDog rex{"Rex", 3, "Husky"};

    std::string found_name;
    int found_age = 0;
    t.for_each_field(rex, [&](std::string_view name, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, std::string>) {
            if (name == "name") found_name = value;
        } else if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, int>) {
            if (name == "age") found_age = value;
        }
    });

    REQUIRE(found_name == "Rex");
    REQUIRE(found_age == 3);
}

TEST_CASE("type_instance: for_each_field() provides typed access via field_value", "[type_instance][for_each_field]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count");
    auto obj = t.create();
    obj.set("title", std::string("Party"));
    obj.set("count", 42);

    std::string found_title;
    int found_count = 0;
    obj.for_each_field([&](std::string_view name, field_value value) {
        if (name == "title")
            found_title = value.as<std::string>();
        if (name == "count")
            found_count = value.as<int>();
    });

    REQUIRE(found_title == "Party");
    REQUIRE(found_count == 42);
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field provides mutable access
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field provides mutable value access", "[type_def][typed][for_each_field]") {
    SimpleArgs args;
    args.name = "Original";

    type_def<SimpleArgs>{}.for_each_field(args, [](std::string_view name, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, std::string>) {
            value = "Changed";
        }
    });

    REQUIRE(args.name.value == "Changed");
}

TEST_CASE("hybrid: for_each_field() provides mutable access", "[type_def][hybrid][for_each_field]") {
    type_def<PlainDog> t;

    PlainDog rex{"Rex", 3, "Husky"};

    t.for_each_field(rex, [](std::string_view name, auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, std::string>) {
            if (name == "name") value = "Buddy";
        }
    });

    REQUIRE(rex.name == "Buddy");
}

TEST_CASE("type_instance: for_each_field() provides mutable access via field_value", "[type_instance][for_each_field]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 0);
    auto obj = t.create();
    obj.set("title", std::string("Before"));
    obj.set("count", 10);

    obj.for_each_field([&](std::string_view name, field_value value) {
        if (name == "title")
            value.as<std::string>() = "After";
        if (name == "count")
            value.as<int>() = 99;
    });

    REQUIRE(obj.get<std::string>("title") == "After");
    REQUIRE(obj.get<int>("count") == 99);
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field skips meta members, iterates plain members
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field skips meta<> members", "[type_def][typed][for_each_field]") {
    Dog rex;
    rex.name = "Rex";
    rex.age = 3;
    rex.breed = "Husky";

    std::vector<std::string> names;
    type_def<Dog>{}.for_each_field(rex, [&](std::string_view name, auto&) {
        names.emplace_back(name);
    });

    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "age");
    REQUIRE(names[2] == "breed");
}

TEST_CASE("hybrid: for_each_field(instance) skips meta members", "[type_def][hybrid][for_each_field]") {
    type_def<MetaDog> t;
    MetaDog rex;
    rex.name = "Rex";
    rex.age = 3;

    std::vector<std::string> names;
    t.for_each_field(rex, [&](std::string_view name, auto&) {
        names.emplace_back(name);
    });

    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "age");
    for (auto& n : names) {
        REQUIRE(n != "help");
    }
}

TEST_CASE("typed: for_each_field iterates all non-meta members including plain", "[type_def][typed][for_each_field]") {
    MixedStruct ms;
    ms.label = "hello";
    ms.counter = 999;
    ms.score = 42;

    std::vector<std::string> names;
    type_def<MixedStruct>{}.for_each_field(ms, [&](std::string_view name, auto&) {
        names.emplace_back(name);
    });

    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "label");
    REQUIRE(names[1] == "counter");
    REQUIRE(names[2] == "score");
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field on empty/meta-only
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field on meta-only struct calls nothing", "[type_def][typed][for_each_field]") {
    MetaOnly mo;

    int count = 0;
    type_def<MetaOnly>{}.for_each_field(mo, [&](std::string_view, auto&) {
        ++count;
    });

    REQUIRE(count == 0);
}

TEST_CASE("hybrid: for_each_field iterates all members of plain struct", "[type_def][hybrid][for_each_field]") {
    type_def<PlainDog> t;

    PlainDog rex{"Rex", 3, "Husky"};

    int count = 0;
    t.for_each_field(rex, [&](std::string_view, auto&) { ++count; });
    REQUIRE(count == 3);
}

TEST_CASE("type_instance: for_each_field() on empty type_def", "[type_instance][for_each_field]") {
    auto t = type_def("Empty");
    auto obj = t.create();

    int count = 0;
    obj.for_each_field([&](std::string_view, field_value) { ++count; });
    REQUIRE(count == 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field with const instance
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field with const instance", "[type_def][typed][for_each_field]") {
    SimpleArgs args;
    args.name = "Const";
    args.age = 99;
    args.active = true;
    const SimpleArgs& cargs = args;

    std::vector<std::string> names;
    type_def<SimpleArgs>{}.for_each_field(cargs, [&](std::string_view name, const auto&) {
        names.emplace_back(name);
    });

    REQUIRE(names.size() == 3);
}

TEST_CASE("hybrid: for_each_field() with const instance", "[type_def][hybrid][for_each_field]") {
    type_def<PlainDog> t;

    const PlainDog rex{"Rex", 3, "Husky"};

    std::string found_name;
    t.for_each_field(rex, [&](std::string_view name, const auto& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>, std::string>) {
            if (name == "name") found_name = value;
        }
    });

    REQUIRE(found_name == "Rex");
}

TEST_CASE("type_instance: for_each_field() with const type_instance", "[type_instance][for_each_field]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 42);
    const auto obj = t.create();

    std::vector<std::string> names;
    obj.for_each_field([&](std::string_view name, const_field_value) {
        names.emplace_back(name);
    });

    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "title");
    REQUIRE(names[1] == "count");
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field count matches field_count
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("hybrid: for_each_field(instance) count matches field_count()", "[type_def][hybrid][for_each_field]") {
    type_def<PlainDog> t;

    PlainDog rex{"Rex", 3, "Husky"};

    int visited = 0;
    t.for_each_field(rex, [&](std::string_view, auto&) { ++visited; });
    REQUIRE(visited == static_cast<int>(t.field_count()));
}

TEST_CASE("typed: for_each_field(instance) count matches field_count()", "[type_def][typed][for_each_field]") {
    SimpleArgs args;
    args.name = "Alice";
    args.age = 30;
    args.active = true;

    int visited = 0;
    type_def<SimpleArgs>{}.for_each_field(args, [&](std::string_view, auto&) { ++visited; });
    REQUIRE(visited == static_cast<int>(type_def<SimpleArgs>{}.field_count()));
}

TEST_CASE("dynamic: for_each_field(instance) count matches field_count()", "[type_def][dynamic][for_each_field]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 0)
        .field<bool>("active", true);
    auto obj = t.create();

    int visited = 0;
    obj.for_each_field([&](std::string_view, field_value) { ++visited; });
    REQUIRE(obj.type().field_count() == 3);
    REQUIRE(visited == static_cast<int>(obj.type().field_count()));
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field iterates field descriptors
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field iterates field descriptors", "[type_def][typed][for_each_field]") {
    std::vector<std::string> names;
    std::vector<std::size_t> indices;

    type_def<Dog>{}.for_each_field([&](auto descriptor) {
        names.emplace_back(descriptor.name());
        if constexpr (requires { descriptor.index(); }) {
            indices.push_back(descriptor.index());
        }
    });

    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "age");
    REQUIRE(names[2] == "breed");
    // Indices are raw struct member indices (metas occupy 0 and 1)
    REQUIRE(indices[0] == 2);
    REQUIRE(indices[1] == 3);
    REQUIRE(indices[2] == 4);
}

TEST_CASE("hybrid: for_each_field() iterates all non-meta members", "[type_def][hybrid][for_each_field]") {
    type_def<PlainDog> t;

    std::vector<std::string> names;
    t.for_each_field([&](auto descriptor) {
        names.emplace_back(descriptor.name());
    });

    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "age");
    REQUIRE(names[2] == "breed");
}

TEST_CASE("dynamic: for_each_field()", "[type_def][dynamic][for_each_field]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 100);

    std::vector<std::string> names;
    bool found_default = false;
    t.for_each_field([&](field_def fd) {
        names.emplace_back(fd.name());
        if (fd.name() == "count" && fd.default_value<int>() == 100)
            found_default = true;
    });

    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "title");
    REQUIRE(names[1] == "count");
    REQUIRE(found_default);
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field skips meta members
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field skips meta members but includes plain", "[type_def][typed][for_each_field]") {
    std::vector<std::string> names;
    type_def<MixedStruct>{}.for_each_field([&](auto descriptor) {
        names.emplace_back(descriptor.name());
    });

    REQUIRE(names.size() == 3);
    REQUIRE(names[0] == "label");
    REQUIRE(names[1] == "counter");
    REQUIRE(names[2] == "score");
}

TEST_CASE("hybrid: for_each_field skips meta members", "[type_def][hybrid][for_each_field]") {
    type_def<MetaDog> t;

    std::vector<std::string> names;
    t.for_each_field([&](auto descriptor) {
        names.emplace_back(descriptor.name());
    });

    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "name");
    REQUIRE(names[1] == "age");
    // "help" (meta member) must not appear
    for (auto& n : names) {
        REQUIRE(n != "help");
    }
}

TEST_CASE("dynamic: for_each_field skips meta members", "[type_def][dynamic][for_each_field]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/events"})
        .field<std::string>("title");

    std::vector<std::string> names;
    t.for_each_field([&](field_def fd) {
        names.emplace_back(fd.name());
    });

    REQUIRE(names.size() == 1);
    REQUIRE(names[0] == "title");
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field on empty
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field on meta-only struct yields nothing", "[type_def][typed][for_each_field]") {
    int count = 0;
    type_def<MetaOnly>{}.for_each_field([&](auto) {
        ++count;
    });
    REQUIRE(count == 0);
}

TEST_CASE("hybrid: for_each_field() on plain struct yields all members", "[type_def][hybrid][for_each_field]") {
    type_def<PlainDog> t;
    int count = 0;
    t.for_each_field([&](auto) { ++count; });
    REQUIRE(count == 3);
}

TEST_CASE("dynamic: for_each_field() empty", "[type_def][dynamic][for_each_field]") {
    auto t = type_def("Empty");
    int count = 0;
    t.for_each_field([&](field_def) { ++count; });
    REQUIRE(count == 0);
}

TEST_CASE("type_instance: for_each_field() via type()", "[type_instance][for_each_field]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 0);
    auto obj = t.create();

    std::vector<std::string> names;
    obj.type().for_each_field([&](field_def fd) {
        names.emplace_back(fd.name());
    });

    REQUIRE(names.size() == 2);
    REQUIRE(names[0] == "title");
    REQUIRE(names[1] == "count");
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field can query field metas
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_field can query field metas", "[type_def][typed][for_each_field]") {
    bool verbose_has_cli = false;
    bool query_has_cli = true;

    type_def<CliArgs>{}.for_each_field([&](auto descriptor) {
        if constexpr (requires { descriptor.index(); }) {
            if (descriptor.name() == "verbose") {
                verbose_has_cli = descriptor.template has_meta<cli_meta>();
            }
            if (descriptor.name() == "query") {
                query_has_cli = descriptor.template has_meta<cli_meta>();
            }
        }
    });

    REQUIRE(verbose_has_cli);
    REQUIRE(!query_has_cli);
}

TEST_CASE("hybrid: for_each_field() can query field metas on auto-discovered members", "[type_def][hybrid][for_each_field]") {
    type_def<PlainDog> t;

    bool name_has_help = true;
    bool age_has_help = true;

    t.for_each_field([&](auto descriptor) {
        if (descriptor.name() == "name")
            name_has_help = descriptor.template has_meta<help_info>();
        if (descriptor.name() == "age")
            age_has_help = descriptor.template has_meta<help_info>();
    });

    // PlainDog has no field-level metas
    REQUIRE(!name_has_help);
    REQUIRE(!age_has_help);
}

TEST_CASE("dynamic: for_each_field() can query field metas", "[type_def][dynamic][for_each_field]") {
    auto t = type_def("CLI")
        .field<std::string>("query")
        .field<bool>("verbose", false,
            with<cli_meta>({.cli = {.short_flag = 'v'}}));

    bool query_has_cli = true;
    bool verbose_has_cli = false;
    char verbose_flag = '\0';

    t.for_each_field([&](field_def fd) {
        if (fd.name() == "query")
            query_has_cli = fd.has_meta<cli_meta>();
        if (fd.name() == "verbose") {
            verbose_has_cli = fd.has_meta<cli_meta>();
            if (verbose_has_cli)
                verbose_flag = fd.meta<cli_meta>().cli.short_flag;
        }
    });

    REQUIRE(!query_has_cli);
    REQUIRE(verbose_has_cli);
    REQUIRE(verbose_flag == 'v');
}

TEST_CASE("type_instance: for_each_field() can query field metas via type()", "[type_instance][for_each_field]") {
    auto t = type_def("CLI")
        .field<std::string>("query")
        .field<bool>("verbose", false,
            with<cli_meta>({.cli = {.short_flag = 'v'}}));
    auto obj = t.create();

    bool verbose_has_cli = false;
    bool query_has_cli = true;

    obj.type().for_each_field([&](field_def fd) {
        if (fd.name() == "verbose")
            verbose_has_cli = fd.has_meta<cli_meta>();
        if (fd.name() == "query")
            query_has_cli = fd.has_meta<cli_meta>();
    });

    REQUIRE(verbose_has_cli);
    REQUIRE(!query_has_cli);
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_field count matches
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("hybrid: for_each_field() count matches field_count()", "[type_def][hybrid][for_each_field]") {
    type_def<PlainDog> t;

    int visited = 0;
    t.for_each_field([&](auto) { ++visited; });
    REQUIRE(visited == static_cast<int>(t.field_count()));
}

TEST_CASE("typed: for_each_field count matches field_count()", "[type_def][typed][for_each_field]") {
    int visited = 0;
    type_def<SimpleArgs>{}.for_each_field([&](auto) { ++visited; });
    REQUIRE(visited == static_cast<int>(type_def<SimpleArgs>{}.field_count()));
}

TEST_CASE("dynamic: for_each_field count matches field_count()", "[type_def][dynamic][for_each_field]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 0);

    int visited = 0;
    t.for_each_field([&](field_def) { ++visited; });
    REQUIRE(visited == static_cast<int>(t.field_count()));
}

// ═══════════════════════════════════════════════════════════════════════════
// for_each_meta
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("typed: for_each_meta iterates meta values (schema-only)", "[type_def][typed][for_each_meta]") {
    int meta_count = 0;
    type_def<Dog>{}.for_each_meta([&](auto& meta_value) {
        ++meta_count;
    });
    REQUIRE(meta_count == 2);
}

TEST_CASE("typed: for_each_meta provides correct values", "[type_def][typed][for_each_meta]") {
    bool found_endpoint = false;
    bool found_help = false;

    type_def<Dog>{}.for_each_meta([&](auto& meta_value) {
        using M = std::remove_cvref_t<decltype(meta_value)>;
        if constexpr (std::is_same_v<M, endpoint_info>) {
            REQUIRE(std::string_view{meta_value.path} == "/dogs");
            found_endpoint = true;
        } else if constexpr (std::is_same_v<M, help_info>) {
            REQUIRE(std::string_view{meta_value.summary} == "A good boy");
            found_help = true;
        }
    });

    REQUIRE(found_endpoint);
    REQUIRE(found_help);
}

TEST_CASE("typed: for_each_meta on struct with no metas", "[type_def][typed][for_each_meta]") {
    int count = 0;
    type_def<SimpleArgs>{}.for_each_meta([&](auto&) {
        ++count;
    });
    REQUIRE(count == 0);
}

TEST_CASE("typed: for_each_meta with multiple metas of same type", "[type_def][typed][for_each_meta]") {
    std::vector<std::string> tag_values;
    type_def<MultiTagged>{}.for_each_meta([&](auto& meta_value) {
        using M = std::remove_cvref_t<decltype(meta_value)>;
        if constexpr (std::is_same_v<M, tag_info>) {
            tag_values.emplace_back(meta_value.value);
        }
    });

    REQUIRE(tag_values.size() == 3);
    REQUIRE(tag_values[0] == "pet");
    REQUIRE(tag_values[1] == "animal");
    REQUIRE(tag_values[2] == "good-boy");
}

TEST_CASE("typed: for_each_meta with instance reads from that instance", "[type_def][typed][for_each_meta]") {
    Dog rex;
    bool found = false;
    type_def<Dog>{}.for_each_meta(rex, [&](auto& meta_value) {
        using M = std::remove_cvref_t<decltype(meta_value)>;
        if constexpr (std::is_same_v<M, endpoint_info>) {
            REQUIRE(std::string_view{meta_value.path} == "/dogs");
            found = true;
        }
    });
    REQUIRE(found);
}

TEST_CASE("hybrid: for_each_meta with instance reads from that instance", "[type_def][hybrid][for_each_meta]") {
    type_def<MetaDog> t;
    MetaDog rex;

    bool found = false;
    int count = 0;
    t.for_each_meta(rex, [&](auto& meta_value) {
        ++count;
        using M = std::remove_cvref_t<decltype(meta_value)>;
        if constexpr (std::is_same_v<M, help_info>) {
            REQUIRE(std::string_view{meta_value.summary} == "A dog");
            found = true;
        }
    });

    REQUIRE(count == 1);
    REQUIRE(found);
}

TEST_CASE("hybrid: for_each_meta() on plain struct yields nothing", "[type_def][hybrid][for_each_meta]") {
    type_def<PlainDog> t;
    int count = 0;
    t.for_each_meta([&](auto&) { ++count; });
    REQUIRE(count == 0);
}

TEST_CASE("hybrid: for_each_meta() on struct with metas", "[type_def][hybrid][for_each_meta]") {
    type_def<MetaDog> t;
    int count = 0;
    t.for_each_meta([&](auto&) { ++count; });
    REQUIRE(count == 1);
}

TEST_CASE("hybrid: for_each_meta iterates meta values", "[type_def][hybrid][for_each_meta]") {
    type_def<MetaDog> t;

    int count = 0;
    bool found_help = false;

    t.for_each_meta([&](auto& meta_value) {
        ++count;
        using M = std::remove_cvref_t<decltype(meta_value)>;
        if constexpr (std::is_same_v<M, help_info>) {
            REQUIRE(std::string_view{meta_value.summary} == "A dog");
            found_help = true;
        }
    });

    REQUIRE(count == 1);
    REQUIRE(found_help);
}

TEST_CASE("dynamic: for_each_meta iterates meta values", "[type_def][dynamic][for_each_meta]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/events", .method = "POST"})
        .meta<help_info>({.summary = "An event"})
        .field<int>("x");

    int count = 0;
    bool found_endpoint = false;
    bool found_help = false;

    t.for_each_meta([&](metadata m) {
        ++count;
        if (m.is<endpoint_info>()) {
            REQUIRE(std::string_view{m.as<endpoint_info>().path} == "/events");
            REQUIRE(std::string_view{m.as<endpoint_info>().method} == "POST");
            found_endpoint = true;
        }
        if (m.is<help_info>()) {
            REQUIRE(std::string_view{m.as<help_info>().summary} == "An event");
            found_help = true;
        }
    });

    REQUIRE(count == 2);
    REQUIRE(found_endpoint);
    REQUIRE(found_help);
}

TEST_CASE("dynamic: for_each_meta on no-meta type_def", "[type_def][dynamic][for_each_meta]") {
    auto t = type_def("Simple")
        .field<int>("x");
    int count = 0;
    t.for_each_meta([&](metadata) { ++count; });
    REQUIRE(count == 0);
}

TEST_CASE("dynamic: for_each_meta with multiple metas of same type", "[type_def][dynamic][for_each_meta]") {
    auto t = type_def("Tagged")
        .meta<tag_info>({.value = "a"})
        .meta<tag_info>({.value = "b"})
        .meta<tag_info>({.value = "c"});

    std::vector<std::string> values;
    t.for_each_meta([&](metadata m) {
        if (m.is<tag_info>())
            values.emplace_back(m.as<tag_info>().value);
    });

    REQUIRE(values.size() == 3);
    REQUIRE(values[0] == "a");
    REQUIRE(values[1] == "b");
    REQUIRE(values[2] == "c");
}

TEST_CASE("dynamic: for_each_meta metadata.is() returns false for wrong type", "[type_def][dynamic][for_each_meta]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/e"});

    t.for_each_meta([&](metadata m) {
        REQUIRE(m.is<endpoint_info>());
        REQUIRE(!m.is<help_info>());
        REQUIRE(!m.is<tag_info>());
    });
}

TEST_CASE("dynamic: for_each_meta metadata.as() throws for wrong type", "[type_def][dynamic][for_each_meta][throw]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/e"});

    t.for_each_meta([&](metadata m) {
        REQUIRE_THROWS_AS(m.as<help_info>(), std::logic_error);
    });
}

TEST_CASE("dynamic: for_each_meta metadata.try_as() returns nullptr for wrong type", "[type_def][dynamic][for_each_meta]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/e"});

    t.for_each_meta([&](metadata m) {
        REQUIRE(m.try_as<endpoint_info>() != nullptr);
        REQUIRE(m.try_as<help_info>() == nullptr);
    });
}

TEST_CASE("type_instance: for_each_meta via type()", "[type_instance][for_each_meta]") {
    auto t = type_def("Event")
        .meta<endpoint_info>({.path = "/events"})
        .meta<help_info>({.summary = "An event"})
        .field<int>("x");
    auto obj = t.create();

    int count = 0;
    bool found_endpoint = false;

    obj.type().for_each_meta([&](metadata m) {
        ++count;
        if (m.is<endpoint_info>()) {
            REQUIRE(std::string_view{m.as<endpoint_info>().path} == "/events");
            found_endpoint = true;
        }
    });

    REQUIRE(count == 2);
    REQUIRE(found_endpoint);
}

TEST_CASE("type_instance: for_each_meta on no-meta type", "[type_instance][for_each_meta]") {
    auto t = type_def("Simple")
        .field<int>("x");
    auto obj = t.create();

    int count = 0;
    obj.type().for_each_meta([&](metadata) { ++count; });
    REQUIRE(count == 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// dynamic full integration
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic: full integration", "[type_def][dynamic][integration]") {
    auto event_t = type_def("Event")
        .meta<endpoint_info>({.path = "/events"})
        .meta<help_info>({.summary = "An event"})
        .field<std::string>("title")
        .field<int>("attendees", 100,
            with<render_meta>({.render = {.style = "bold", .width = 10}}))
        .field<bool>("verbose", false,
            with<cli_meta>({.cli = {.short_flag = 'v'}}),
            with<render_meta>({.render = {.style = "dim"}}));

    // Schema
    REQUIRE(event_t.name() == "Event");
    REQUIRE(event_t.field_count() == 3);
    auto names = event_t.field_names();
    REQUIRE(names[0] == "title");
    REQUIRE(names[1] == "attendees");
    REQUIRE(names[2] == "verbose");

    // Type-level metas
    REQUIRE(event_t.has_meta<endpoint_info>());
    REQUIRE(std::string_view{event_t.meta<endpoint_info>().path} == "/events");
    REQUIRE(event_t.has_meta<help_info>());
    REQUIRE(std::string_view{event_t.meta<help_info>().summary} == "An event");
    REQUIRE(!event_t.has_meta<tag_info>());

    // Field queries
    REQUIRE(event_t.has_field("title"));
    REQUIRE(event_t.has_field("attendees"));
    REQUIRE(event_t.has_field("verbose"));
    REQUIRE(!event_t.has_field("nope"));

    REQUIRE(event_t.field("attendees").default_value<int>() == 100);
    REQUIRE(event_t.field("verbose").default_value<bool>() == false);

    // Field-level metas
    REQUIRE(!event_t.field("title").has_meta<cli_meta>());
    REQUIRE(!event_t.field("title").has_meta<render_meta>());

    REQUIRE(event_t.field("attendees").has_meta<render_meta>());
    REQUIRE(!event_t.field("attendees").has_meta<cli_meta>());
    REQUIRE(std::string_view{
        event_t.field("attendees").meta<render_meta>().render.style} == "bold");
    REQUIRE(event_t.field("attendees").meta<render_meta>().render.width == 10);

    REQUIRE(event_t.field("verbose").has_meta<cli_meta>());
    REQUIRE(event_t.field("verbose").has_meta<render_meta>());
    REQUIRE(event_t.field("verbose").meta<cli_meta>().cli.short_flag == 'v');
    REQUIRE(std::string_view{
        event_t.field("verbose").meta<render_meta>().render.style} == "dim");
}
