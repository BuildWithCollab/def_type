#include "test_json_types.hpp"

// ═════════════════════════════════════════════════════════════════════════
// Typed to_json — explicit serialization tests
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed to_json: primitive fields", "[to_json][typed]") {
    SimpleArgs args;
    args.name = "Alice";
    args.age = 30;
    args.active = true;

    auto j = to_json(args);

    REQUIRE(j.is_object());
    REQUIRE(j["name"] == "Alice");
    REQUIRE(j["age"] == 30);
    REQUIRE(j["active"] == true);
}

TEST_CASE("dynamic to_json: primitive fields", "[to_json][dynamic]") {
    auto t = type_def("SimpleArgs_toP")
        .field<std::string>("name")
        .field<int>("age")
        .field<bool>("active");
    auto obj = t.create();
    obj.set("name", std::string("Alice"));
    obj.set("age", 30);
    obj.set("active", true);

    auto j = obj.to_json();

    REQUIRE(j.is_object());
    REQUIRE(j["name"] == "Alice");
    REQUIRE(j["age"] == 30);
    REQUIRE(j["active"] == true);
}

TEST_CASE("typed to_json: default-constructed struct", "[to_json][typed]") {
    SimpleArgs args;
    auto j = to_json(args);

    REQUIRE(j["name"] == "");
    REQUIRE(j["age"] == 0);
    REQUIRE(j["active"] == false);
}

TEST_CASE("dynamic to_json: default-constructed fields", "[to_json][dynamic]") {
    auto t = type_def("SimpleArgs_toDef")
        .field<std::string>("name")
        .field<int>("age")
        .field<bool>("active");
    auto obj = t.create();

    auto j = obj.to_json();

    REQUIRE(j["name"] == "");
    REQUIRE(j["age"] == 0);
    REQUIRE(j["active"] == false);
}

TEST_CASE("typed to_json: struct with defaults", "[to_json][typed]") {
    WithDefaults w;
    auto j = to_json(w);

    REQUIRE(j["city"] == "Portland");
    REQUIRE(j["days"] == 7);
}

TEST_CASE("dynamic to_json: struct with defaults", "[to_json][dynamic]") {
    auto t = type_def("WithDefaults_to")
        .field<std::string>("city", std::string("Portland"))
        .field<int>("days", 7);
    auto obj = t.create();

    auto j = obj.to_json();

    REQUIRE(j["city"] == "Portland");
    REQUIRE(j["days"] == 7);
}

TEST_CASE("typed to_json: nested struct", "[to_json][typed][nested]") {
    Person p;
    p.name = "Bob";
    p.address.value.street = "123 Main St";
    p.address.value.zip = "97201";

    auto j = to_json(p);

    REQUIRE(j["name"] == "Bob");
    REQUIRE(j["address"].is_object());
    REQUIRE(j["address"]["street"] == "123 Main St");
    REQUIRE(j["address"]["zip"] == "97201");
}

TEST_CASE("dynamic to_json: nested struct", "[to_json][dynamic][nested]") {
    auto t = type_def("Person_to")
        .field<std::string>("name")
        .field<Address>("address");
    auto obj = t.create();
    obj.set("name", std::string("Bob"));
    Address addr; addr.street = "123 Main St"; addr.zip = "97201";
    obj.set("address", addr);

    auto j = obj.to_json();

    REQUIRE(j["name"] == "Bob");
    REQUIRE(j["address"].is_object());
    REQUIRE(j["address"]["street"] == "123 Main St");
    REQUIRE(j["address"]["zip"] == "97201");
}

TEST_CASE("typed to_json: vector of strings", "[to_json][typed][vector]") {
    TaggedItem item;
    item.title = "Post";
    item.tags.value = {"c++", "modules", "reflection"};

    auto j = to_json(item);

    REQUIRE(j["tags"].is_array());
    REQUIRE(j["tags"].size() == 3);
    REQUIRE(j["tags"][0] == "c++");
    REQUIRE(j["tags"][1] == "modules");
    REQUIRE(j["tags"][2] == "reflection");
}

TEST_CASE("dynamic to_json: vector of strings", "[to_json][dynamic][vector]") {
    auto t = type_def("TaggedItem_to")
        .field<std::string>("title")
        .field<std::vector<std::string>>("tags");
    auto obj = t.create();
    obj.set("title", std::string("Post"));
    obj.set("tags", std::vector<std::string>{"c++", "modules", "reflection"});

    auto j = obj.to_json();

    REQUIRE(j["tags"].is_array());
    REQUIRE(j["tags"].size() == 3);
    REQUIRE(j["tags"][0] == "c++");
    REQUIRE(j["tags"][1] == "modules");
    REQUIRE(j["tags"][2] == "reflection");
}

TEST_CASE("typed to_json: empty vector", "[to_json][typed][vector]") {
    TaggedItem item;
    item.title = "Empty";

    auto j = to_json(item);

    REQUIRE(j["tags"].is_array());
    REQUIRE(j["tags"].empty());
}

TEST_CASE("dynamic to_json: empty vector", "[to_json][dynamic][vector]") {
    auto t = type_def("TaggedItem_toEV")
        .field<std::string>("title")
        .field<std::vector<std::string>>("tags");
    auto obj = t.create();
    obj.set("title", std::string("Empty"));

    auto j = obj.to_json();

    REQUIRE(j["tags"].is_array());
    REQUIRE(j["tags"].empty());
}

TEST_CASE("typed to_json: vector of structs", "[to_json][typed][vector]") {
    Team team;
    team.team_name = "Pirates";
    SimpleArgs a1; a1.name = "Alice"; a1.age = 30; a1.active = true;
    SimpleArgs a2; a2.name = "Bob"; a2.age = 25; a2.active = false;
    team.members.value = {a1, a2};

    auto j = to_json(team);

    REQUIRE(j["members"].is_array());
    REQUIRE(j["members"].size() == 2);
    REQUIRE(j["members"][0]["name"] == "Alice");
    REQUIRE(j["members"][0]["age"] == 30);
    REQUIRE(j["members"][1]["name"] == "Bob");
}

TEST_CASE("dynamic to_json: vector of structs", "[to_json][dynamic][vector]") {
    auto t = type_def("Team_to")
        .field<std::string>("team_name")
        .field<std::vector<SimpleArgs>>("members");
    auto obj = t.create();
    obj.set("team_name", std::string("Pirates"));
    SimpleArgs a1; a1.name = "Alice"; a1.age = 30; a1.active = true;
    SimpleArgs a2; a2.name = "Bob"; a2.age = 25; a2.active = false;
    obj.set("members", std::vector<SimpleArgs>{a1, a2});

    auto j = obj.to_json();

    REQUIRE(j["members"].is_array());
    REQUIRE(j["members"].size() == 2);
    REQUIRE(j["members"][0]["name"] == "Alice");
    REQUIRE(j["members"][0]["age"] == 30);
    REQUIRE(j["members"][1]["name"] == "Bob");
}

TEST_CASE("typed to_json: optional present", "[to_json][typed][optional]") {
    MaybeNickname m;
    m.name = "Alice";
    m.nickname.value = "Ali";

    auto j = to_json(m);

    REQUIRE(j["nickname"] == "Ali");
}

TEST_CASE("dynamic to_json: optional present", "[to_json][dynamic][optional]") {
    auto t = type_def("MaybeNickname_toP")
        .field<std::string>("name")
        .field<std::optional<std::string>>("nickname");
    auto obj = t.create();
    obj.set("name", std::string("Alice"));
    obj.set("nickname", std::optional<std::string>("Ali"));

    auto j = obj.to_json();

    REQUIRE(j["nickname"] == "Ali");
}

TEST_CASE("typed to_json: optional absent", "[to_json][typed][optional]") {
    MaybeNickname m;
    m.name = "Bob";

    auto j = to_json(m);

    REQUIRE(j["nickname"].is_null());
}

TEST_CASE("dynamic to_json: optional absent", "[to_json][dynamic][optional]") {
    auto t = type_def("MaybeNickname_toA")
        .field<std::string>("name")
        .field<std::optional<std::string>>("nickname");
    auto obj = t.create();
    obj.set("name", std::string("Bob"));

    auto j = obj.to_json();

    REQUIRE(j["nickname"].is_null());
}

TEST_CASE("typed to_json: optional nested struct present", "[to_json][typed][optional][nested]") {
    Outer o;
    o.name = "test";
    o.extra.value = Inner{};
    o.extra.value->x = 42;

    auto j = to_json(o);

    REQUIRE(j["extra"].is_object());
    REQUIRE(j["extra"]["x"] == 42);
}

TEST_CASE("dynamic to_json: optional nested struct present", "[to_json][dynamic][optional][nested]") {
    auto t = type_def("Outer_toP")
        .field<std::string>("name")
        .field<std::optional<Inner>>("extra");
    auto obj = t.create();
    obj.set("name", std::string("test"));
    Inner inner; inner.x = 42;
    obj.set("extra", std::optional<Inner>(inner));

    auto j = obj.to_json();

    REQUIRE(j["extra"].is_object());
    REQUIRE(j["extra"]["x"] == 42);
}

TEST_CASE("typed to_json: optional nested struct absent", "[to_json][typed][optional][nested]") {
    Outer o;
    o.name = "test";

    auto j = to_json(o);

    REQUIRE(j["extra"].is_null());
}

TEST_CASE("dynamic to_json: optional nested struct absent", "[to_json][dynamic][optional][nested]") {
    auto t = type_def("Outer_toA")
        .field<std::string>("name")
        .field<std::optional<Inner>>("extra");
    auto obj = t.create();
    obj.set("name", std::string("test"));

    auto j = obj.to_json();

    REQUIRE(j["extra"].is_null());
}

TEST_CASE("typed to_json: std::map<string, int>", "[to_json][typed][map]") {
    Config c;
    c.name = "app";
    c.settings.value = {{"timeout", 30}, {"retries", 3}};

    auto j = to_json(c);

    REQUIRE(j["settings"].is_object());
    REQUIRE(j["settings"]["timeout"] == 30);
    REQUIRE(j["settings"]["retries"] == 3);
}

TEST_CASE("dynamic to_json: std::map<string, int>", "[to_json][dynamic][map]") {
    auto t = type_def("Config_to")
        .field<std::string>("name")
        .field<std::map<std::string, int>>("settings");
    auto obj = t.create();
    obj.set("name", std::string("app"));
    obj.set("settings", std::map<std::string, int>{{"timeout", 30}, {"retries", 3}});

    auto j = obj.to_json();

    REQUIRE(j["settings"].is_object());
    REQUIRE(j["settings"]["timeout"] == 30);
    REQUIRE(j["settings"]["retries"] == 3);
}

TEST_CASE("typed to_json: map of structs", "[to_json][typed][map]") {
    ServiceMap sm;
    Endpoint api; api.url = "https://api.example.com"; api.port = 443;
    sm.services.value = {{"api", api}};

    auto j = to_json(sm);

    REQUIRE(j["services"]["api"].is_object());
    REQUIRE(j["services"]["api"]["url"] == "https://api.example.com");
    REQUIRE(j["services"]["api"]["port"] == 443);
}

TEST_CASE("dynamic to_json: map of structs", "[to_json][dynamic][map]") {
    auto t = type_def("ServiceMap_to")
        .field<std::map<std::string, Endpoint>>("services");
    auto obj = t.create();
    Endpoint api; api.url = "https://api.example.com"; api.port = 443;
    obj.set("services", std::map<std::string, Endpoint>{{"api", api}});

    auto j = obj.to_json();

    REQUIRE(j["services"]["api"].is_object());
    REQUIRE(j["services"]["api"]["url"] == "https://api.example.com");
    REQUIRE(j["services"]["api"]["port"] == 443);
}

TEST_CASE("typed to_json: empty map", "[to_json][typed][map]") {
    EmptyConfig ec;
    auto j = to_json(ec);

    REQUIRE(j["settings"].is_object());
    REQUIRE(j["settings"].empty());
}

TEST_CASE("dynamic to_json: empty map", "[to_json][dynamic][map]") {
    auto t = type_def("EmptyConfig_to")
        .field<std::map<std::string, int>>("settings");
    auto obj = t.create();

    auto j = obj.to_json();

    REQUIRE(j["settings"].is_object());
    REQUIRE(j["settings"].empty());
}

TEST_CASE("typed to_json: std::set<string>", "[to_json][typed][set]") {
    TagSet ts;
    ts.tags.value = {"alpha", "beta", "gamma"};

    auto j = to_json(ts);

    REQUIRE(j["tags"].is_array());
    REQUIRE(j["tags"].size() == 3);
}

TEST_CASE("dynamic to_json: std::set<string>", "[to_json][dynamic][set]") {
    auto t = type_def("TagSet_to")
        .field<std::set<std::string>>("tags");
    auto obj = t.create();
    obj.set("tags", std::set<std::string>{"alpha", "beta", "gamma"});

    auto j = obj.to_json();

    REQUIRE(j["tags"].is_array());
    REQUIRE(j["tags"].size() == 3);
}

TEST_CASE("typed to_json: std::unordered_set<int>", "[to_json][typed][set]") {
    IdSet is;
    is.ids.value = {10, 20, 30};

    auto j = to_json(is);

    REQUIRE(j["ids"].is_array());
    REQUIRE(j["ids"].size() == 3);
}

TEST_CASE("dynamic to_json: std::unordered_set<int>", "[to_json][dynamic][set]") {
    auto t = type_def("IdSet_to")
        .field<std::unordered_set<int>>("ids");
    auto obj = t.create();
    obj.set("ids", std::unordered_set<int>{10, 20, 30});

    auto j = obj.to_json();

    REQUIRE(j["ids"].is_array());
    REQUIRE(j["ids"].size() == 3);
}

TEST_CASE("typed to_json: ankerl::unordered_dense::map", "[to_json][typed][dense]") {
    DenseMapStruct dm;
    dm.scores.value = {{"alice", 100}, {"bob", 85}};

    auto j = to_json(dm);

    REQUIRE(j["scores"].is_object());
    REQUIRE(j["scores"]["alice"] == 100);
    REQUIRE(j["scores"]["bob"] == 85);
}

TEST_CASE("dynamic to_json: ankerl::unordered_dense::map", "[to_json][dynamic][dense]") {
    auto t = type_def("DenseMapStruct_to")
        .field<ankerl::unordered_dense::map<std::string, int>>("scores");
    auto obj = t.create();
    obj.set("scores", ankerl::unordered_dense::map<std::string, int>{{"alice", 100}, {"bob", 85}});

    auto j = obj.to_json();

    REQUIRE(j["scores"].is_object());
    REQUIRE(j["scores"]["alice"] == 100);
    REQUIRE(j["scores"]["bob"] == 85);
}

TEST_CASE("typed to_json: ankerl::unordered_dense::set", "[to_json][typed][dense]") {
    DenseSetStruct ds;
    ds.names.value = {"x", "y", "z"};

    auto j = to_json(ds);

    REQUIRE(j["names"].is_array());
    REQUIRE(j["names"].size() == 3);
}

TEST_CASE("dynamic to_json: ankerl::unordered_dense::set", "[to_json][dynamic][dense]") {
    auto t = type_def("DenseSetStruct_to")
        .field<ankerl::unordered_dense::set<std::string>>("names");
    auto obj = t.create();
    obj.set("names", ankerl::unordered_dense::set<std::string>{"x", "y", "z"});

    auto j = obj.to_json();

    REQUIRE(j["names"].is_array());
    REQUIRE(j["names"].size() == 3);
}

TEST_CASE("typed to_json: double field", "[to_json][typed][numeric]") {
    Measurement m;
    m.unit = "celsius";
    m.value = 72.5;

    auto j = to_json(m);

    REQUIRE(j["value"] == 72.5);
}

TEST_CASE("dynamic to_json: double field", "[to_json][dynamic][numeric]") {
    auto t = type_def("Measurement_to")
        .field<std::string>("unit")
        .field<double>("value");
    auto obj = t.create();
    obj.set("unit", std::string("celsius"));
    obj.set("value", 72.5);

    auto j = obj.to_json();

    REQUIRE(j["value"] == 72.5);
}

TEST_CASE("typed to_json: float field", "[to_json][typed][numeric]") {
    FloatStruct fs;
    fs.weight = 3.14f;

    auto j = to_json(fs);

    REQUIRE(j["weight"].get<float>() == Catch::Approx(3.14f));
}

TEST_CASE("dynamic to_json: float field", "[to_json][dynamic][numeric]") {
    auto t = type_def("FloatStruct_to")
        .field<float>("weight");
    auto obj = t.create();
    obj.set("weight", 3.14f);

    auto j = obj.to_json();

    REQUIRE(j["weight"].get<float>() == Catch::Approx(3.14f));
}

TEST_CASE("typed to_json: int64 and uint64", "[to_json][typed][numeric]") {
    BigNumbers bn;
    bn.signed_big = 9'000'000'000LL;
    bn.unsigned_big = 18'000'000'000ULL;

    auto j = to_json(bn);

    REQUIRE(j["signed_big"] == 9000000000LL);
    REQUIRE(j["unsigned_big"] == 18000000000ULL);
}

TEST_CASE("dynamic to_json: int64 and uint64", "[to_json][dynamic][numeric]") {
    auto t = type_def("BigNumbers_to")
        .field<int64_t>("signed_big")
        .field<uint64_t>("unsigned_big");
    auto obj = t.create();
    obj.set("signed_big", int64_t(9'000'000'000LL));
    obj.set("unsigned_big", uint64_t(18'000'000'000ULL));

    auto j = obj.to_json();

    REQUIRE(j["signed_big"] == 9000000000LL);
    REQUIRE(j["unsigned_big"] == 18000000000ULL);
}

TEST_CASE("typed to_json: mixed struct skips non-field members", "[to_json][typed][mixed]") {
    MixedStruct ms;
    ms.visible = "hello";
    ms.internal_counter = 999;
    ms.score = 42;

    auto j = to_json(ms);

    REQUIRE(j.size() == 2);
    REQUIRE(j["visible"] == "hello");
    REQUIRE(j["score"] == 42);
    REQUIRE(!j.contains("internal_counter"));
}

TEST_CASE("typed to_json: extensions untouched in output", "[to_json][typed][with]") {
    CliArgs args;
    args.query = "hello";
    args.verbose = true;

    auto j = to_json(args);

    REQUIRE(j["query"] == "hello");
    REQUIRE(j["verbose"] == true);
    REQUIRE(!j.contains("posix"));
    REQUIRE(!j.contains("with"));
}

// ═════════════════════════════════════════════════════════════════════════
// Typed — nested collections
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed to_json: vector of vectors", "[to_json][typed][nested_collections]") {
    VecOfVecs vv;
    vv.matrix.value = {{1, 2, 3}, {4, 5}, {6}};

    auto j = to_json(vv);

    REQUIRE(j["matrix"].is_array());
    REQUIRE(j["matrix"].size() == 3);
    REQUIRE(j["matrix"][0] == json::array({1, 2, 3}));
    REQUIRE(j["matrix"][1] == json::array({4, 5}));
    REQUIRE(j["matrix"][2] == json::array({6}));
}

TEST_CASE("dynamic to_json: vector of vectors", "[to_json][dynamic][nested_collections]") {
    auto t = type_def("VecOfVecs_to")
        .field<std::vector<std::vector<int>>>("matrix");
    auto obj = t.create();
    obj.set("matrix", std::vector<std::vector<int>>{{1, 2, 3}, {4, 5}, {6}});

    auto j = obj.to_json();

    REQUIRE(j["matrix"].is_array());
    REQUIRE(j["matrix"].size() == 3);
    REQUIRE(j["matrix"][0] == json::array({1, 2, 3}));
    REQUIRE(j["matrix"][1] == json::array({4, 5}));
    REQUIRE(j["matrix"][2] == json::array({6}));
}

TEST_CASE("typed to_json: map of vectors", "[to_json][typed][nested_collections]") {
    MapOfVecs mv;
    mv.grouped.value = {{"fruits", {"apple", "banana"}}, {"colors", {"red"}}};

    auto j = to_json(mv);

    REQUIRE(j["grouped"]["fruits"] == json::array({"apple", "banana"}));
    REQUIRE(j["grouped"]["colors"] == json::array({"red"}));
}

TEST_CASE("dynamic to_json: map of vectors", "[to_json][dynamic][nested_collections]") {
    auto t = type_def("MapOfVecs_to")
        .field<std::map<std::string, std::vector<std::string>>>("grouped");
    auto obj = t.create();
    obj.set("grouped", std::map<std::string, std::vector<std::string>>{{"fruits", {"apple", "banana"}}, {"colors", {"red"}}});

    auto j = obj.to_json();

    REQUIRE(j["grouped"]["fruits"] == json::array({"apple", "banana"}));
    REQUIRE(j["grouped"]["colors"] == json::array({"red"}));
}

TEST_CASE("typed to_json: vector of maps", "[to_json][typed][nested_collections]") {
    VecOfMaps vm;
    vm.records.value = {{{"x", 1}, {"y", 2}}, {{"z", 3}}};

    auto j = to_json(vm);

    REQUIRE(j["records"].is_array());
    REQUIRE(j["records"].size() == 2);
    REQUIRE(j["records"][0]["x"] == 1);
    REQUIRE(j["records"][0]["y"] == 2);
    REQUIRE(j["records"][1]["z"] == 3);
}

TEST_CASE("dynamic to_json: vector of maps", "[to_json][dynamic][nested_collections]") {
    auto t = type_def("VecOfMaps_to")
        .field<std::vector<std::map<std::string, int>>>("records");
    auto obj = t.create();
    obj.set("records", std::vector<std::map<std::string, int>>{{{"x", 1}, {"y", 2}}, {{"z", 3}}});

    auto j = obj.to_json();

    REQUIRE(j["records"].is_array());
    REQUIRE(j["records"].size() == 2);
    REQUIRE(j["records"][0]["x"] == 1);
    REQUIRE(j["records"][0]["y"] == 2);
    REQUIRE(j["records"][1]["z"] == 3);
}

// ═════════════════════════════════════════════════════════════════════════
// Typed — optional + collection combos
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed to_json: optional vector present", "[to_json][typed][optional_collection]") {
    OptionalVec ov;
    ov.maybe_tags.value = std::vector<std::string>{"a", "b"};

    auto j = to_json(ov);

    REQUIRE(j["maybe_tags"].is_array());
    REQUIRE(j["maybe_tags"].size() == 2);
    REQUIRE(j["maybe_tags"][0] == "a");
}

TEST_CASE("dynamic to_json: optional vector present", "[to_json][dynamic][optional_collection]") {
    auto t = type_def("OptionalVec_toP")
        .field<std::optional<std::vector<std::string>>>("maybe_tags");
    auto obj = t.create();
    obj.set("maybe_tags", std::optional<std::vector<std::string>>(std::vector<std::string>{"a", "b"}));

    auto j = obj.to_json();

    REQUIRE(j["maybe_tags"].is_array());
    REQUIRE(j["maybe_tags"].size() == 2);
    REQUIRE(j["maybe_tags"][0] == "a");
}

TEST_CASE("typed to_json: optional vector absent", "[to_json][typed][optional_collection]") {
    OptionalVec ov;

    auto j = to_json(ov);

    REQUIRE(j["maybe_tags"].is_null());
}

TEST_CASE("dynamic to_json: optional vector absent", "[to_json][dynamic][optional_collection]") {
    auto t = type_def("OptionalVec_toA")
        .field<std::optional<std::vector<std::string>>>("maybe_tags");
    auto obj = t.create();

    auto j = obj.to_json();

    REQUIRE(j["maybe_tags"].is_null());
}

TEST_CASE("typed to_json: vector of optionals", "[to_json][typed][optional_collection]") {
    VecOfOptionals vo;
    vo.scores.value = {1, std::nullopt, 3, std::nullopt, 5};

    auto j = to_json(vo);

    REQUIRE(j["scores"].is_array());
    REQUIRE(j["scores"].size() == 5);
    REQUIRE(j["scores"][0] == 1);
    REQUIRE(j["scores"][1].is_null());
    REQUIRE(j["scores"][2] == 3);
    REQUIRE(j["scores"][3].is_null());
    REQUIRE(j["scores"][4] == 5);
}

TEST_CASE("dynamic to_json: vector of optionals", "[to_json][dynamic][optional_collection]") {
    auto t = type_def("VecOfOptionals_to")
        .field<std::vector<std::optional<int>>>("scores");
    auto obj = t.create();
    obj.set("scores", std::vector<std::optional<int>>{1, std::nullopt, 3, std::nullopt, 5});

    auto j = obj.to_json();

    REQUIRE(j["scores"].is_array());
    REQUIRE(j["scores"].size() == 5);
    REQUIRE(j["scores"][0] == 1);
    REQUIRE(j["scores"][1].is_null());
    REQUIRE(j["scores"][2] == 3);
    REQUIRE(j["scores"][3].is_null());
    REQUIRE(j["scores"][4] == 5);
}

// ═════════════════════════════════════════════════════════════════════════
// Typed — deep nesting (3 levels + collections)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed to_json: 3-level deep nested struct", "[to_json][typed][deep_nesting]") {
    DeepOuter obj;
    obj.name = "root";
    obj.middle.value.label = "mid";
    obj.middle.value.inner.value.value = "leaf";
    DeepInner item_a; item_a.value = "a";
    DeepInner item_b; item_b.value = "b";
    obj.items.value = {item_a, item_b};

    auto j = to_json(obj);

    REQUIRE(j["name"] == "root");
    REQUIRE(j["middle"]["label"] == "mid");
    REQUIRE(j["middle"]["inner"]["value"] == "leaf");
    REQUIRE(j["items"].is_array());
    REQUIRE(j["items"].size() == 2);
    REQUIRE(j["items"][0]["value"] == "a");
    REQUIRE(j["items"][1]["value"] == "b");
}

TEST_CASE("dynamic to_json: 3-level deep nested struct", "[to_json][dynamic][deep_nesting]") {
    auto t = type_def("DeepOuter_to")
        .field<std::string>("name")
        .field<DeepMiddle>("middle")
        .field<std::vector<DeepInner>>("items");
    auto obj = t.create();
    obj.set("name", std::string("root"));
    DeepMiddle mid; mid.label = "mid"; mid.inner.value.value = "leaf";
    obj.set("middle", mid);
    DeepInner item_a; item_a.value = "a";
    DeepInner item_b; item_b.value = "b";
    obj.set("items", std::vector<DeepInner>{item_a, item_b});

    auto j = obj.to_json();

    REQUIRE(j["name"] == "root");
    REQUIRE(j["middle"]["label"] == "mid");
    REQUIRE(j["middle"]["inner"]["value"] == "leaf");
    REQUIRE(j["items"].is_array());
    REQUIRE(j["items"].size() == 2);
    REQUIRE(j["items"][0]["value"] == "a");
    REQUIRE(j["items"][1]["value"] == "b");
}

TEST_CASE("typed to_json: vector of persons with nested addresses", "[to_json][typed][deep_nesting]") {
    PersonList pl;
    pl.org = "Collab";
    Person p1; p1.name = "Alice"; p1.address.value.street = "1st St"; p1.address.value.zip = "10001";
    Person p2; p2.name = "Bob"; p2.address.value.street = "2nd Ave"; p2.address.value.zip = "20002";
    pl.people.value = {p1, p2};

    auto j = to_json(pl);

    REQUIRE(j["org"] == "Collab");
    REQUIRE(j["people"].size() == 2);
    REQUIRE(j["people"][0]["name"] == "Alice");
    REQUIRE(j["people"][0]["address"]["street"] == "1st St");
    REQUIRE(j["people"][1]["address"]["zip"] == "20002");
}

TEST_CASE("dynamic to_json: vector of persons with nested addresses", "[to_json][dynamic][deep_nesting]") {
    auto t = type_def("PersonList_to")
        .field<std::string>("org")
        .field<std::vector<Person>>("people");
    auto obj = t.create();
    obj.set("org", std::string("Collab"));
    Person p1; p1.name = "Alice"; p1.address.value.street = "1st St"; p1.address.value.zip = "10001";
    Person p2; p2.name = "Bob"; p2.address.value.street = "2nd Ave"; p2.address.value.zip = "20002";
    obj.set("people", std::vector<Person>{p1, p2});

    auto j = obj.to_json();

    REQUIRE(j["org"] == "Collab");
    REQUIRE(j["people"].size() == 2);
    REQUIRE(j["people"][0]["name"] == "Alice");
    REQUIRE(j["people"][0]["address"]["street"] == "1st St");
    REQUIRE(j["people"][1]["address"]["zip"] == "20002");
}

// ═════════════════════════════════════════════════════════════════════════
// Typed — dense containers with struct values
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed to_json: ankerl dense map of structs", "[to_json][typed][dense]") {
    DenseMapOfStructs dm;
    Address home; home.street = "123 Main"; home.zip = "97201";
    Address work; work.street = "456 Corp Dr"; work.zip = "97202";
    dm.locations.value = {{"home", home}, {"work", work}};

    auto j = to_json(dm);

    REQUIRE(j["locations"]["home"]["street"] == "123 Main");
    REQUIRE(j["locations"]["work"]["zip"] == "97202");
}

TEST_CASE("dynamic to_json: ankerl dense map of structs", "[to_json][dynamic][dense]") {
    auto t = type_def("DenseMapOfStructs_to")
        .field<ankerl::unordered_dense::map<std::string, Address>>("locations");
    auto obj = t.create();
    Address home; home.street = "123 Main"; home.zip = "97201";
    Address work; work.street = "456 Corp Dr"; work.zip = "97202";
    obj.set("locations", ankerl::unordered_dense::map<std::string, Address>{{"home", home}, {"work", work}});

    auto j = obj.to_json();

    REQUIRE(j["locations"]["home"]["street"] == "123 Main");
    REQUIRE(j["locations"]["work"]["zip"] == "97202");
}

// ═════════════════════════════════════════════════════════════════════════
// Typed — enums to_json
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed to_json: enum field serializes as string", "[to_json][typed][enum]") {
    EnumStruct es;
    es.method = HttpMethod::POST;
    es.name = "create-dog";

    auto j = to_json(es);

    REQUIRE(j["method"] == "POST");
    REQUIRE(j["name"] == "create-dog");
}

TEST_CASE("dynamic to_json: enum field serializes as string", "[to_json][dynamic][enum]") {
    auto t = type_def("EnumStruct_to")
        .field<HttpMethod>("method")
        .field<std::string>("name");
    auto obj = t.create();
    obj.set("method", HttpMethod::POST);
    obj.set("name", std::string("create-dog"));

    auto j = obj.to_json();

    REQUIRE(j["method"] == "POST");
    REQUIRE(j["name"] == "create-dog");
}

TEST_CASE("typed to_json: enum default serializes as string", "[to_json][typed][enum]") {
    EnumStruct es;
    es.name = "default";

    auto j = to_json(es);

    REQUIRE(j["method"] == "GET");
}

TEST_CASE("dynamic to_json: enum default serializes as string", "[to_json][dynamic][enum]") {
    auto t = type_def("EnumStruct_toDef")
        .field<HttpMethod>("method")
        .field<std::string>("name");
    auto obj = t.create();
    obj.set("name", std::string("default"));

    auto j = obj.to_json();

    REQUIRE(j["method"] == "GET");
}

TEST_CASE("typed to_json: multiple enum fields serialize as strings", "[to_json][typed][enum]") {
    MultiEnumStruct ms;
    ms.color = Color::blue;
    ms.method = HttpMethod::PUT;
    ms.label = "fancy";

    auto j = to_json(ms);

    REQUIRE(j["color"] == "blue");
    REQUIRE(j["method"] == "PUT");
    REQUIRE(j["label"] == "fancy");
}

TEST_CASE("dynamic to_json: multiple enum fields serialize as strings", "[to_json][dynamic][enum]") {
    auto t = type_def("MultiEnumStruct_to")
        .field<Color>("color")
        .field<HttpMethod>("method")
        .field<std::string>("label");
    auto obj = t.create();
    obj.set("color", Color::blue);
    obj.set("method", HttpMethod::PUT);
    obj.set("label", std::string("fancy"));

    auto j = obj.to_json();

    REQUIRE(j["color"] == "blue");
    REQUIRE(j["method"] == "PUT");
    REQUIRE(j["label"] == "fancy");
}

TEST_CASE("typed to_json: vector of enums", "[to_json][typed][enum][collection]") {
    VecOfEnums ve;
    ve.methods.value = {HttpMethod::GET, HttpMethod::POST, HttpMethod::PUT};

    auto j = to_json(ve);

    REQUIRE(j["methods"].is_array());
    REQUIRE(j["methods"].size() == 3);
    REQUIRE(j["methods"][0] == "GET");
    REQUIRE(j["methods"][1] == "POST");
    REQUIRE(j["methods"][2] == "PUT");
}

TEST_CASE("dynamic to_json: vector of enums", "[to_json][dynamic][enum][collection]") {
    auto t = type_def("VecOfEnums_to")
        .field<std::vector<HttpMethod>>("methods");
    auto obj = t.create();
    obj.set("methods", std::vector<HttpMethod>{HttpMethod::GET, HttpMethod::POST, HttpMethod::PUT});

    auto j = obj.to_json();

    REQUIRE(j["methods"].is_array());
    REQUIRE(j["methods"].size() == 3);
    REQUIRE(j["methods"][0] == "GET");
    REQUIRE(j["methods"][1] == "POST");
    REQUIRE(j["methods"][2] == "PUT");
}

TEST_CASE("typed to_json: map of enums", "[to_json][typed][enum][collection]") {
    MapOfEnums me;
    me.palette.value = {{"sky", Color::blue}, {"grass", Color::green}};

    auto j = to_json(me);

    REQUIRE(j["palette"]["sky"] == "blue");
    REQUIRE(j["palette"]["grass"] == "green");
}

TEST_CASE("dynamic to_json: map of enums", "[to_json][dynamic][enum][collection]") {
    auto t = type_def("MapOfEnums_to")
        .field<std::map<std::string, Color>>("palette");
    auto obj = t.create();
    obj.set("palette", std::map<std::string, Color>{{"sky", Color::blue}, {"grass", Color::green}});

    auto j = obj.to_json();

    REQUIRE(j["palette"]["sky"] == "blue");
    REQUIRE(j["palette"]["grass"] == "green");
}

// ═════════════════════════════════════════════════════════════════════════
// Typed/hybrid to_json
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed/hybrid to_json: struct with field<> serializes correctly", "[to_json][hybrid]") {
    SimpleArgs args;
    args.name = "Alice";
    args.age = 30;
    args.active = true;

    auto j = to_json(args);

    REQUIRE(j["name"] == "Alice");
    REQUIRE(j["age"] == 30);
    REQUIRE(j["active"] == true);
}

// ═════════════════════════════════════════════════════════════════════════
// Dynamic to_json — type_instance serialization (existing tests)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic to_json: basic primitive fields", "[to_json][dynamic]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count")
        .field<bool>("active");
    auto obj = t.create();

    obj.set("title", std::string("Party"));
    obj.set("count", 42);
    obj.set("active", true);

    auto j = obj.to_json();

    REQUIRE(j["title"] == "Party");
    REQUIRE(j["count"] == 42);
    REQUIRE(j["active"] == true);
}

TEST_CASE("dynamic to_json: preserves default values", "[to_json][dynamic]") {
    auto t = type_def("Event_defV")
        .field<std::string>("title", std::string("Untitled"))
        .field<int>("count", 100);
    auto obj = t.create();

    auto j = obj.to_json();

    REQUIRE(j["title"] == "Untitled");
    REQUIRE(j["count"] == 100);
}

TEST_CASE("dynamic to_json: all primitive types", "[to_json][dynamic]") {
    auto t = type_def("Types")
        .field<std::string>("s", std::string("hello"))
        .field<bool>("b", true)
        .field<int>("i", 42)
        .field<int64_t>("i64", int64_t(9000000000LL))
        .field<uint64_t>("u64", uint64_t(18000000000ULL))
        .field<double>("d", 3.14)
        .field<float>("f", 2.5f);
    auto obj = t.create();

    auto j = obj.to_json();

    REQUIRE(j["s"] == "hello");
    REQUIRE(j["b"] == true);
    REQUIRE(j["i"] == 42);
    REQUIRE(j["i64"] == 9000000000LL);
    REQUIRE(j["u64"] == 18000000000ULL);
    REQUIRE(j["d"] == 3.14);
    REQUIRE(j["f"].get<float>() == Catch::Approx(2.5f));
}

TEST_CASE("dynamic to_json: empty type_def produces empty object", "[to_json][dynamic]") {
    auto t = type_def("Empty");
    auto obj = t.create();

    auto j = obj.to_json();

    REQUIRE(j.is_object());
    REQUIRE(j.empty());
}

TEST_CASE("dynamic to_json: fields with no defaults get default-constructed values", "[to_json][dynamic]") {
    auto t = type_def("Event_noDef")
        .field<std::string>("title")
        .field<int>("count");
    auto obj = t.create();

    auto j = obj.to_json();

    REQUIRE(j["title"] == "");
    REQUIRE(j["count"] == 0);
}

// ═════════════════════════════════════════════════════════════════════════
// Dynamic to_json_string
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic to_json_string: compact output", "[to_json][dynamic][string]") {
    auto t = type_def("Event_strC")
        .field<std::string>("title", std::string("Party"));
    auto obj = t.create();

    auto s = obj.to_json_string();

    auto j = json::parse(s);
    REQUIRE(j["title"] == "Party");
    REQUIRE(s.find('\n') == std::string::npos);
}

TEST_CASE("dynamic to_json_string: pretty output", "[to_json][dynamic][string]") {
    auto t = type_def("Event_strP")
        .field<std::string>("title", std::string("Party"));
    auto obj = t.create();

    auto s = obj.to_json_string(2);

    auto j = json::parse(s);
    REQUIRE(j["title"] == "Party");
    REQUIRE(s.find('\n') != std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════
// Dynamic to_json with metas — metas don't appear in JSON
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic to_json: metas are not serialized", "[to_json][dynamic][meta]") {
    struct endpoint_info { const char* path = ""; };

    auto t = type_def("Event_meta")
        .meta<endpoint_info>({.path = "/events"})
        .field<std::string>("title", std::string("Party"));
    auto obj = t.create();

    auto j = obj.to_json();

    REQUIRE(j.size() == 1);
    REQUIRE(j["title"] == "Party");
    REQUIRE(!j.contains("endpoint"));
    REQUIRE(!j.contains("path"));
}

TEST_CASE("dynamic to_json: set after create reflects updated value", "[to_json][dynamic]") {
    auto t = type_def("Event_upd")
        .field<std::string>("title", std::string("Original"));
    auto obj = t.create();

    obj.set("title", std::string("Updated"));

    REQUIRE(obj.to_json()["title"] == "Updated");
}

TEST_CASE("dynamic to_json_string: round-trip parse matches to_json", "[to_json][dynamic][string]") {
    auto t = type_def("Event_strRT")
        .field<std::string>("title", std::string("Party"))
        .field<int>("count", 42);
    auto obj = t.create();

    auto from_string = json::parse(obj.to_json_string());
    auto from_method = obj.to_json();

    REQUIRE(from_string == from_method);
}

TEST_CASE("dynamic to_json_string: empty type produces empty object string", "[to_json][dynamic][string]") {
    auto t = type_def("Empty_str");
    auto obj = t.create();

    REQUIRE(obj.to_json_string() == "{}");
}

// ═════════════════════════════════════════════════════════════════════════
// Hybrid to_json
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("hybrid to_json: struct with metas — only fields serialized", "[to_json][hybrid][meta]") {
    HJsonDog d;
    d.name = "Rex";
    d.age = 3;
    d.breed = "Husky";

    auto j = to_json(d);

    REQUIRE(j.size() == 3);
    REQUIRE(j["name"] == "Rex");
    REQUIRE(j["age"] == 3);
    REQUIRE(j["breed"] == "Husky");
    REQUIRE(!j.contains("endpoint"));
    REQUIRE(!j.contains("help"));
}

TEST_CASE("hybrid to_json: mixed struct skips meta and plain members", "[to_json][hybrid][meta]") {
    HJsonMixed ms;
    ms.label = "hello";
    ms.counter = 999;
    ms.score = 42;

    auto j = to_json(ms);

    REQUIRE(j.size() == 2);
    REQUIRE(j["label"] == "hello");
    REQUIRE(j["score"] == 42);
    REQUIRE(!j.contains("counter"));
    REQUIRE(!j.contains("help"));
}

TEST_CASE("hybrid to_json_string: compact output", "[to_json][hybrid][string]") {
    HJsonDog d;
    d.name = "Rex";
    d.age = 3;
    d.breed = "Husky";

    auto s = to_json_string(d);

    auto j = json::parse(s);
    REQUIRE(j["name"] == "Rex");
    REQUIRE(s.find('\n') == std::string::npos);
}

TEST_CASE("hybrid to_json_string: pretty output", "[to_json][hybrid][string]") {
    HJsonDog d;
    d.name = "Rex";
    d.age = 3;
    d.breed = "Husky";

    auto s = to_json_string(d, 2);

    auto j = json::parse(s);
    REQUIRE(j["name"] == "Rex");
    REQUIRE(s.find('\n') != std::string::npos);
}
