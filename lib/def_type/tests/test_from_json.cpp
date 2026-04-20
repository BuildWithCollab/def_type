#include "test_json_types.hpp"

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — primitives
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: primitive fields", "[from_json][typed]") {
    auto j = json{{"name", "Alice"}, {"age", 30}, {"active", true}};
    auto args = from_json<SimpleArgs>(j);

    REQUIRE(args.name.value == "Alice");
    REQUIRE(args.age.value == 30);
    REQUIRE(args.active.value == true);
}

TEST_CASE("dynamic from_json: primitive fields", "[from_json][dynamic]") {
    auto t = type_def("SimpleArgs_fP")
        .field<std::string>("name")
        .field<int>("age")
        .field<bool>("active");

    auto j = json{{"name", "Alice"}, {"age", 30}, {"active", true}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "Alice");
    REQUIRE(obj.get<int>("age") == 30);
    REQUIRE(obj.get<bool>("active") == true);
}

TEST_CASE("typed from_json: preserves struct defaults for missing keys", "[from_json][typed]") {
    auto j = json::object();
    auto w = from_json<WithDefaults>(j);

    REQUIRE(w.city.value == "Portland");
    REQUIRE(w.days.value == 7);
}

TEST_CASE("dynamic from_json: preserves defaults for missing keys", "[from_json][dynamic]") {
    auto t = type_def("WithDefaults_fDef")
        .field<std::string>("city", std::string("Portland"))
        .field<int>("days", 7);

    auto obj = t.create(json::object());

    REQUIRE(obj.get<std::string>("city") == "Portland");
    REQUIRE(obj.get<int>("days") == 7);
}

TEST_CASE("typed from_json: partial overlay — some keys present, some missing", "[from_json][typed]") {
    auto j = json{{"city", "Seattle"}};
    auto w = from_json<WithDefaults>(j);

    REQUIRE(w.city.value == "Seattle");
    REQUIRE(w.days.value == 7);
}

TEST_CASE("dynamic from_json: partial overlay — some keys present, some missing", "[from_json][dynamic]") {
    auto t = type_def("WithDefaults_fPart")
        .field<std::string>("city", std::string("Portland"))
        .field<int>("days", 7);

    auto j = json{{"city", "Seattle"}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("city") == "Seattle");
    REQUIRE(obj.get<int>("days") == 7);
}

TEST_CASE("typed from_json: extra keys are silently ignored", "[from_json][typed]") {
    auto j = json{{"name", "Bob"}, {"age", 25}, {"active", false},
                   {"unknown_field", "ignored"}, {"another", 999}};
    auto args = from_json<SimpleArgs>(j);

    REQUIRE(args.name.value == "Bob");
    REQUIRE(args.age.value == 25);
    REQUIRE(args.active.value == false);
}

TEST_CASE("dynamic from_json: extra keys are silently ignored", "[from_json][dynamic]") {
    auto t = type_def("SimpleArgs_fExtra")
        .field<std::string>("name")
        .field<int>("age")
        .field<bool>("active");

    auto j = json{{"name", "Bob"}, {"age", 25}, {"active", false},
                   {"unknown_field", "ignored"}, {"another", 999}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "Bob");
    REQUIRE(obj.get<int>("age") == 25);
    REQUIRE(obj.get<bool>("active") == false);
}

TEST_CASE("typed from_json: empty JSON object gives all defaults", "[from_json][typed]") {
    auto args = from_json<SimpleArgs>(json::object());

    REQUIRE(args.name.value.empty());
    REQUIRE(args.age.value == 0);
    REQUIRE(args.active.value == false);
}

TEST_CASE("dynamic from_json: empty JSON object gives all defaults", "[from_json][dynamic]") {
    auto t = type_def("SimpleArgs_fEmpty")
        .field<std::string>("name")
        .field<int>("age")
        .field<bool>("active");

    auto obj = t.create(json::object());

    REQUIRE(obj.get<std::string>("name").empty());
    REQUIRE(obj.get<int>("age") == 0);
    REQUIRE(obj.get<bool>("active") == false);
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — nested structs
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: nested reflected struct", "[from_json][typed][nested]") {
    auto j = json{
        {"name", "Bob"},
        {"address", {{"street", "123 Main St"}, {"zip", "97201"}}}
    };
    auto p = from_json<Person>(j);

    REQUIRE(p.name.value == "Bob");
    REQUIRE(p.address.value.street.value == "123 Main St");
    REQUIRE(p.address.value.zip.value == "97201");
}

TEST_CASE("dynamic from_json: nested reflected struct", "[from_json][dynamic][nested]") {
    auto t = type_def("Person_fN")
        .field<std::string>("name")
        .field<Address>("address");

    auto j = json{
        {"name", "Bob"},
        {"address", {{"street", "123 Main St"}, {"zip", "97201"}}}
    };
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "Bob");
    auto addr = obj.get<Address>("address");
    REQUIRE(addr.street.value == "123 Main St");
    REQUIRE(addr.zip.value == "97201");
}

TEST_CASE("typed from_json: nested struct with missing inner keys", "[from_json][typed][nested]") {
    auto j = json{
        {"name", "Bob"},
        {"address", {{"street", "123 Main St"}}}
    };
    auto p = from_json<Person>(j);

    REQUIRE(p.address.value.street.value == "123 Main St");
    REQUIRE(p.address.value.zip.value.empty());
}

TEST_CASE("dynamic from_json: nested struct with missing inner keys", "[from_json][dynamic][nested]") {
    auto t = type_def("Person_fNMissing")
        .field<std::string>("name")
        .field<Address>("address");

    auto j = json{
        {"name", "Bob"},
        {"address", {{"street", "123 Main St"}}}
    };
    auto obj = t.create(j);

    auto addr = obj.get<Address>("address");
    REQUIRE(addr.street.value == "123 Main St");
    REQUIRE(addr.zip.value.empty());
}

TEST_CASE("typed from_json: missing nested object entirely", "[from_json][typed][nested]") {
    auto j = json{{"name", "Bob"}};
    auto p = from_json<Person>(j);

    REQUIRE(p.name.value == "Bob");
    REQUIRE(p.address.value.street.value.empty());
    REQUIRE(p.address.value.zip.value.empty());
}

TEST_CASE("dynamic from_json: missing nested object entirely", "[from_json][dynamic][nested]") {
    auto t = type_def("Person_fNMissObj")
        .field<std::string>("name")
        .field<Address>("address");

    auto j = json{{"name", "Bob"}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "Bob");
    auto addr = obj.get<Address>("address");
    REQUIRE(addr.street.value.empty());
    REQUIRE(addr.zip.value.empty());
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — vectors
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: vector of strings", "[from_json][typed][vector]") {
    auto j = json{{"title", "My Post"}, {"tags", {"c++", "modules", "reflection"}}};
    auto item = from_json<TaggedItem>(j);

    REQUIRE(item.title.value == "My Post");
    REQUIRE(item.tags.value.size() == 3);
    REQUIRE(item.tags.value[0] == "c++");
    REQUIRE(item.tags.value[1] == "modules");
    REQUIRE(item.tags.value[2] == "reflection");
}

TEST_CASE("dynamic from_json: vector of strings", "[from_json][dynamic][vector]") {
    auto t = type_def("TaggedItem_f")
        .field<std::string>("title")
        .field<std::vector<std::string>>("tags");

    auto j = json{{"title", "My Post"}, {"tags", {"c++", "modules", "reflection"}}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("title") == "My Post");
    auto tags = obj.get<std::vector<std::string>>("tags");
    REQUIRE(tags.size() == 3);
    REQUIRE(tags[0] == "c++");
    REQUIRE(tags[1] == "modules");
    REQUIRE(tags[2] == "reflection");
}

TEST_CASE("typed from_json: empty vector", "[from_json][typed][vector]") {
    auto j = json{{"title", "Empty"}, {"tags", json::array()}};
    auto item = from_json<TaggedItem>(j);

    REQUIRE(item.tags.value.empty());
}

TEST_CASE("dynamic from_json: empty vector", "[from_json][dynamic][vector]") {
    auto t = type_def("TaggedItem_fEV")
        .field<std::string>("title")
        .field<std::vector<std::string>>("tags");

    auto j = json{{"title", "Empty"}, {"tags", json::array()}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::vector<std::string>>("tags").empty());
}

TEST_CASE("typed from_json: vector of nested reflected structs", "[from_json][typed][vector]") {
    auto j = json{
        {"team_name", "Pirates"},
        {"members", {
            {{"name", "Alice"}, {"age", 30}, {"active", true}},
            {{"name", "Bob"}, {"age", 25}, {"active", false}}
        }}
    };
    auto t = from_json<Team>(j);

    REQUIRE(t.team_name.value == "Pirates");
    REQUIRE(t.members.value.size() == 2);
    REQUIRE(t.members.value[0].name.value == "Alice");
    REQUIRE(t.members.value[0].age.value == 30);
    REQUIRE(t.members.value[0].active.value == true);
    REQUIRE(t.members.value[1].name.value == "Bob");
    REQUIRE(t.members.value[1].age.value == 25);
    REQUIRE(t.members.value[1].active.value == false);
}

TEST_CASE("dynamic from_json: vector of nested reflected structs", "[from_json][dynamic][vector]") {
    auto td = type_def("Team_f")
        .field<std::string>("team_name")
        .field<std::vector<SimpleArgs>>("members");

    auto j = json{
        {"team_name", "Pirates"},
        {"members", {
            {{"name", "Alice"}, {"age", 30}, {"active", true}},
            {{"name", "Bob"}, {"age", 25}, {"active", false}}
        }}
    };
    auto obj = td.create(j);

    REQUIRE(obj.get<std::string>("team_name") == "Pirates");
    auto members = obj.get<std::vector<SimpleArgs>>("members");
    REQUIRE(members.size() == 2);
    REQUIRE(members[0].name.value == "Alice");
    REQUIRE(members[0].age.value == 30);
    REQUIRE(members[0].active.value == true);
    REQUIRE(members[1].name.value == "Bob");
    REQUIRE(members[1].age.value == 25);
    REQUIRE(members[1].active.value == false);
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — optionals
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: optional present", "[from_json][typed][optional]") {
    auto j = json{{"name", "Alice"}, {"nickname", "Ali"}};
    auto m = from_json<MaybeNickname>(j);

    REQUIRE(m.name.value == "Alice");
    REQUIRE(m.nickname.value.has_value());
    REQUIRE(*m.nickname.value == "Ali");
}

TEST_CASE("dynamic from_json: optional present", "[from_json][dynamic][optional]") {
    auto t = type_def("MaybeNickname_fP")
        .field<std::string>("name")
        .field<std::optional<std::string>>("nickname");

    auto j = json{{"name", "Alice"}, {"nickname", "Ali"}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "Alice");
    auto nn = obj.get<std::optional<std::string>>("nickname");
    REQUIRE(nn.has_value());
    REQUIRE(*nn == "Ali");
}

TEST_CASE("typed from_json: optional null", "[from_json][typed][optional]") {
    auto j = json{{"name", "Bob"}, {"nickname", nullptr}};
    auto m = from_json<MaybeNickname>(j);

    REQUIRE(m.name.value == "Bob");
    REQUIRE(!m.nickname.value.has_value());
}

TEST_CASE("dynamic from_json: optional null", "[from_json][dynamic][optional]") {
    auto t = type_def("MaybeNickname_fNull")
        .field<std::string>("name")
        .field<std::optional<std::string>>("nickname");

    auto j = json{{"name", "Bob"}, {"nickname", nullptr}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "Bob");
    auto nn = obj.get<std::optional<std::string>>("nickname");
    REQUIRE(!nn.has_value());
}

TEST_CASE("typed from_json: optional missing key", "[from_json][typed][optional]") {
    auto j = json{{"name", "Bob"}};
    auto m = from_json<MaybeNickname>(j);

    REQUIRE(m.name.value == "Bob");
    REQUIRE(!m.nickname.value.has_value());
}

TEST_CASE("dynamic from_json: optional missing key", "[from_json][dynamic][optional]") {
    auto t = type_def("MaybeNickname_fMiss")
        .field<std::string>("name")
        .field<std::optional<std::string>>("nickname");

    auto j = json{{"name", "Bob"}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "Bob");
    auto nn = obj.get<std::optional<std::string>>("nickname");
    REQUIRE(!nn.has_value());
}

TEST_CASE("typed from_json: optional nested struct — present", "[from_json][typed][optional][nested]") {
    auto j = json{{"name", "test"}, {"extra", {{"x", 42}}}};
    auto o = from_json<Outer>(j);

    REQUIRE(o.name.value == "test");
    REQUIRE(o.extra.value.has_value());
    REQUIRE(o.extra.value->x.value == 42);
}

TEST_CASE("dynamic from_json: optional nested struct — present", "[from_json][dynamic][optional][nested]") {
    auto t = type_def("Outer_fP")
        .field<std::string>("name")
        .field<std::optional<Inner>>("extra");

    auto j = json{{"name", "test"}, {"extra", {{"x", 42}}}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "test");
    auto extra = obj.get<std::optional<Inner>>("extra");
    REQUIRE(extra.has_value());
    REQUIRE(extra->x.value == 42);
}

TEST_CASE("typed from_json: optional nested struct — null", "[from_json][typed][optional][nested]") {
    auto j = json{{"name", "test"}, {"extra", nullptr}};
    auto o = from_json<Outer>(j);

    REQUIRE(o.name.value == "test");
    REQUIRE(!o.extra.value.has_value());
}

TEST_CASE("dynamic from_json: optional nested struct — null", "[from_json][dynamic][optional][nested]") {
    auto t = type_def("Outer_fNull")
        .field<std::string>("name")
        .field<std::optional<Inner>>("extra");

    auto j = json{{"name", "test"}, {"extra", nullptr}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "test");
    auto extra = obj.get<std::optional<Inner>>("extra");
    REQUIRE(!extra.has_value());
}

TEST_CASE("typed from_json: optional nested struct — missing", "[from_json][typed][optional][nested]") {
    auto j = json{{"name", "test"}};
    auto o = from_json<Outer>(j);

    REQUIRE(!o.extra.value.has_value());
}

TEST_CASE("dynamic from_json: optional nested struct — missing", "[from_json][dynamic][optional][nested]") {
    auto t = type_def("Outer_fMiss")
        .field<std::string>("name")
        .field<std::optional<Inner>>("extra");

    auto j = json{{"name", "test"}};
    auto obj = t.create(j);

    auto extra = obj.get<std::optional<Inner>>("extra");
    REQUIRE(!extra.has_value());
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — maps
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: std::map<string, int>", "[from_json][typed][map]") {
    auto j = json{{"name", "my-app"}, {"settings", {{"timeout", 30}, {"retries", 3}}}};
    auto c = from_json<Config>(j);

    REQUIRE(c.name.value == "my-app");
    REQUIRE(c.settings.value.at("timeout") == 30);
    REQUIRE(c.settings.value.at("retries") == 3);
}

TEST_CASE("dynamic from_json: std::map<string, int>", "[from_json][dynamic][map]") {
    auto t = type_def("Config_f")
        .field<std::string>("name")
        .field<std::map<std::string, int>>("settings");

    auto j = json{{"name", "my-app"}, {"settings", {{"timeout", 30}, {"retries", 3}}}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "my-app");
    auto settings = obj.get<std::map<std::string, int>>("settings");
    REQUIRE(settings.at("timeout") == 30);
    REQUIRE(settings.at("retries") == 3);
}

TEST_CASE("typed from_json: std::unordered_map<string, string>", "[from_json][typed][map]") {
    auto j = json{{"tags", {{"env", "prod"}, {"region", "us-west"}}}};
    auto l = from_json<Labels>(j);

    REQUIRE(l.tags.value.at("env") == "prod");
    REQUIRE(l.tags.value.at("region") == "us-west");
}

TEST_CASE("dynamic from_json: std::unordered_map<string, string>", "[from_json][dynamic][map]") {
    auto t = type_def("Labels_f")
        .field<std::unordered_map<std::string, std::string>>("tags");

    auto j = json{{"tags", {{"env", "prod"}, {"region", "us-west"}}}};
    auto obj = t.create(j);

    auto tags = obj.get<std::unordered_map<std::string, std::string>>("tags");
    REQUIRE(tags.at("env") == "prod");
    REQUIRE(tags.at("region") == "us-west");
}

TEST_CASE("typed from_json: map of reflected structs", "[from_json][typed][map]") {
    auto j = json{{"services", {
        {"api", {{"url", "https://api.example.com"}, {"port", 443}}},
        {"db",  {{"url", "localhost"}, {"port", 5432}}}
    }}};
    auto sm = from_json<ServiceMap>(j);

    REQUIRE(sm.services.value.at("api").url.value == "https://api.example.com");
    REQUIRE(sm.services.value.at("api").port.value == 443);
    REQUIRE(sm.services.value.at("db").url.value == "localhost");
    REQUIRE(sm.services.value.at("db").port.value == 5432);
}

TEST_CASE("dynamic from_json: map of reflected structs", "[from_json][dynamic][map]") {
    auto t = type_def("ServiceMap_f")
        .field<std::map<std::string, Endpoint>>("services");

    auto j = json{{"services", {
        {"api", {{"url", "https://api.example.com"}, {"port", 443}}},
        {"db",  {{"url", "localhost"}, {"port", 5432}}}
    }}};
    auto obj = t.create(j);

    auto services = obj.get<std::map<std::string, Endpoint>>("services");
    REQUIRE(services.at("api").url.value == "https://api.example.com");
    REQUIRE(services.at("api").port.value == 443);
    REQUIRE(services.at("db").url.value == "localhost");
    REQUIRE(services.at("db").port.value == 5432);
}

TEST_CASE("typed from_json: empty map", "[from_json][typed][map]") {
    auto j = json{{"settings", json::object()}};
    auto ec = from_json<EmptyConfig>(j);

    REQUIRE(ec.settings.value.empty());
}

TEST_CASE("dynamic from_json: empty map", "[from_json][dynamic][map]") {
    auto t = type_def("EmptyConfig_f")
        .field<std::map<std::string, int>>("settings");

    auto j = json{{"settings", json::object()}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::map<std::string, int>>("settings").empty());
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — sets
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: std::set<string>", "[from_json][typed][set]") {
    auto j = json{{"tags", {"alpha", "beta", "gamma"}}};
    auto ts = from_json<TagSet>(j);

    REQUIRE(ts.tags.value.size() == 3);
    REQUIRE(ts.tags.value.count("alpha") == 1);
    REQUIRE(ts.tags.value.count("beta") == 1);
    REQUIRE(ts.tags.value.count("gamma") == 1);
}

TEST_CASE("dynamic from_json: std::set<string>", "[from_json][dynamic][set]") {
    auto t = type_def("TagSet_f")
        .field<std::set<std::string>>("tags");

    auto j = json{{"tags", {"alpha", "beta", "gamma"}}};
    auto obj = t.create(j);

    auto tags = obj.get<std::set<std::string>>("tags");
    REQUIRE(tags.size() == 3);
    REQUIRE(tags.count("alpha") == 1);
    REQUIRE(tags.count("beta") == 1);
    REQUIRE(tags.count("gamma") == 1);
}

TEST_CASE("typed from_json: std::unordered_set<int>", "[from_json][typed][set]") {
    auto j = json{{"ids", {10, 20, 30}}};
    auto is = from_json<IdSet>(j);

    REQUIRE(is.ids.value.size() == 3);
    REQUIRE(is.ids.value.count(10) == 1);
    REQUIRE(is.ids.value.count(20) == 1);
    REQUIRE(is.ids.value.count(30) == 1);
}

TEST_CASE("dynamic from_json: std::unordered_set<int>", "[from_json][dynamic][set]") {
    auto t = type_def("IdSet_f")
        .field<std::unordered_set<int>>("ids");

    auto j = json{{"ids", {10, 20, 30}}};
    auto obj = t.create(j);

    auto ids = obj.get<std::unordered_set<int>>("ids");
    REQUIRE(ids.size() == 3);
    REQUIRE(ids.count(10) == 1);
    REQUIRE(ids.count(20) == 1);
    REQUIRE(ids.count(30) == 1);
}

TEST_CASE("typed from_json: empty set", "[from_json][typed][set]") {
    auto j = json{{"tags", json::array()}};
    auto ts = from_json<TagSet>(j);

    REQUIRE(ts.tags.value.empty());
}

TEST_CASE("dynamic from_json: empty set", "[from_json][dynamic][set]") {
    auto t = type_def("TagSet_fE")
        .field<std::set<std::string>>("tags");

    auto j = json{{"tags", json::array()}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::set<std::string>>("tags").empty());
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — ankerl::unordered_dense
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: ankerl::unordered_dense::map", "[from_json][typed][dense]") {
    auto j = json{{"scores", {{"alice", 100}, {"bob", 85}}}};
    auto dm = from_json<DenseMapStruct>(j);

    REQUIRE(dm.scores.value.at("alice") == 100);
    REQUIRE(dm.scores.value.at("bob") == 85);
}

TEST_CASE("dynamic from_json: ankerl::unordered_dense::map", "[from_json][dynamic][dense]") {
    auto t = type_def("DenseMapStruct_f")
        .field<ankerl::unordered_dense::map<std::string, int>>("scores");

    auto j = json{{"scores", {{"alice", 100}, {"bob", 85}}}};
    auto obj = t.create(j);

    auto scores = obj.get<ankerl::unordered_dense::map<std::string, int>>("scores");
    REQUIRE(scores.at("alice") == 100);
    REQUIRE(scores.at("bob") == 85);
}

TEST_CASE("typed from_json: ankerl::unordered_dense::set", "[from_json][typed][dense]") {
    auto j = json{{"names", {"x", "y", "z"}}};
    auto ds = from_json<DenseSetStruct>(j);

    REQUIRE(ds.names.value.size() == 3);
    REQUIRE(ds.names.value.count("x") == 1);
}

TEST_CASE("dynamic from_json: ankerl::unordered_dense::set", "[from_json][dynamic][dense]") {
    auto t = type_def("DenseSetStruct_f")
        .field<ankerl::unordered_dense::set<std::string>>("names");

    auto j = json{{"names", {"x", "y", "z"}}};
    auto obj = t.create(j);

    auto names = obj.get<ankerl::unordered_dense::set<std::string>>("names");
    REQUIRE(names.size() == 3);
    REQUIRE(names.count("x") == 1);
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — numeric types
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: double field", "[from_json][typed][numeric]") {
    auto j = json{{"unit", "celsius"}, {"value", 72.5}};
    auto m = from_json<Measurement>(j);

    REQUIRE(m.unit.value == "celsius");
    REQUIRE(m.value.value == 72.5);
}

TEST_CASE("dynamic from_json: double field", "[from_json][dynamic][numeric]") {
    auto t = type_def("Measurement_f")
        .field<std::string>("unit")
        .field<double>("value");

    auto j = json{{"unit", "celsius"}, {"value", 72.5}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("unit") == "celsius");
    REQUIRE(obj.get<double>("value") == 72.5);
}

TEST_CASE("typed from_json: float field", "[from_json][typed][numeric]") {
    auto j = json{{"weight", 3.14}};
    auto fs = from_json<FloatStruct>(j);

    REQUIRE(fs.weight.value == Catch::Approx(3.14f));
}

TEST_CASE("dynamic from_json: float field", "[from_json][dynamic][numeric]") {
    auto t = type_def("FloatStruct_f")
        .field<float>("weight");

    auto j = json{{"weight", 3.14}};
    auto obj = t.create(j);

    REQUIRE(obj.get<float>("weight") == Catch::Approx(3.14f));
}

TEST_CASE("typed from_json: int64 and uint64", "[from_json][typed][numeric]") {
    auto j = json{{"signed_big", 9000000000LL}, {"unsigned_big", 18000000000ULL}};
    auto bn = from_json<BigNumbers>(j);

    REQUIRE(bn.signed_big.value == 9'000'000'000LL);
    REQUIRE(bn.unsigned_big.value == 18'000'000'000ULL);
}

TEST_CASE("dynamic from_json: int64 and uint64", "[from_json][dynamic][numeric]") {
    auto t = type_def("BigNumbers_f")
        .field<int64_t>("signed_big")
        .field<uint64_t>("unsigned_big");

    auto j = json{{"signed_big", 9000000000LL}, {"unsigned_big", 18000000000ULL}};
    auto obj = t.create(j);

    REQUIRE(obj.get<int64_t>("signed_big") == 9'000'000'000LL);
    REQUIRE(obj.get<uint64_t>("unsigned_big") == 18'000'000'000ULL);
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — extensions (with<>) are untouched
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: extensions untouched — only value deserialized", "[from_json][typed][with]") {
    auto j = json{{"query", "hello world"}, {"verbose", true}};
    auto args = from_json<CliArgs>(j);

    REQUIRE(args.query.value == "hello world");
    REQUIRE(args.verbose.value == true);
    REQUIRE(args.query.with.posix.short_flag == 'q');
    REQUIRE(args.query.with.posix.from_stdin == true);
    REQUIRE(args.verbose.with.posix.short_flag == 'v');
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — mixed struct (all non-meta members deserialized)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: deserializes all non-meta members", "[from_json][typed][mixed]") {
    auto j = json{{"visible", "hello"}, {"score", 42}, {"internal_counter", 999}};
    auto ms = from_json<MixedStruct>(j);

    REQUIRE(ms.visible.value == "hello");
    REQUIRE(ms.score.value == 42);
    REQUIRE(ms.internal_counter == 999);
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — from string
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: from JSON string", "[from_json][typed][string]") {
    auto args = from_json<SimpleArgs>(std::string(R"({"name":"Alice","age":30,"active":true})"));

    REQUIRE(args.name.value == "Alice");
    REQUIRE(args.age.value == 30);
    REQUIRE(args.active.value == true);
}

TEST_CASE("typed from_json: from pretty JSON string", "[from_json][typed][string]") {
    std::string pretty = R"({
        "name": "Bob",
        "age": 25,
        "active": false
    })";
    auto args = from_json<SimpleArgs>(pretty);

    REQUIRE(args.name.value == "Bob");
    REQUIRE(args.age.value == 25);
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — round-trip (to_json -> from_json)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: round-trip simple struct", "[from_json][typed][roundtrip]") {
    SimpleArgs original;
    original.name = "Alice";
    original.age = 30;
    original.active = true;

    auto j = to_json(original);
    auto restored = from_json<SimpleArgs>(j);

    REQUIRE(restored.name.value == original.name.value);
    REQUIRE(restored.age.value == original.age.value);
    REQUIRE(restored.active.value == original.active.value);
}

TEST_CASE("dynamic from_json: round-trip simple struct", "[from_json][dynamic][roundtrip]") {
    auto t = type_def("SimpleArgs_fRT")
        .field<std::string>("name")
        .field<int>("age")
        .field<bool>("active");
    auto obj = t.create();
    obj.set("name", std::string("Alice"));
    obj.set("age", 30);
    obj.set("active", true);

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    REQUIRE(obj2.get<std::string>("name") == "Alice");
    REQUIRE(obj2.get<int>("age") == 30);
    REQUIRE(obj2.get<bool>("active") == true);
}

TEST_CASE("typed from_json: round-trip nested struct", "[from_json][typed][roundtrip]") {
    Person original;
    original.name = "Bob";
    original.address.value.street = "123 Main";
    original.address.value.zip = "97201";

    auto j = to_json(original);
    auto restored = from_json<Person>(j);

    REQUIRE(restored.name.value == "Bob");
    REQUIRE(restored.address.value.street.value == "123 Main");
    REQUIRE(restored.address.value.zip.value == "97201");
}

TEST_CASE("dynamic from_json: round-trip nested struct", "[from_json][dynamic][roundtrip]") {
    auto t = type_def("Person_fRT")
        .field<std::string>("name")
        .field<Address>("address");
    auto obj = t.create();
    obj.set("name", std::string("Bob"));
    Address addr; addr.street = "123 Main"; addr.zip = "97201";
    obj.set("address", addr);

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    REQUIRE(obj2.get<std::string>("name") == "Bob");
    auto a = obj2.get<Address>("address");
    REQUIRE(a.street.value == "123 Main");
    REQUIRE(a.zip.value == "97201");
}

TEST_CASE("typed from_json: round-trip vector of structs", "[from_json][typed][roundtrip]") {
    Team original;
    original.team_name = "Pirates";
    SimpleArgs a1; a1.name = "Alice"; a1.age = 30; a1.active = true;
    SimpleArgs a2; a2.name = "Bob"; a2.age = 25; a2.active = false;
    original.members.value = {a1, a2};

    auto j = to_json(original);
    auto restored = from_json<Team>(j);

    REQUIRE(restored.team_name.value == "Pirates");
    REQUIRE(restored.members.value.size() == 2);
    REQUIRE(restored.members.value[0].name.value == "Alice");
    REQUIRE(restored.members.value[1].name.value == "Bob");
}

TEST_CASE("dynamic from_json: round-trip vector of structs", "[from_json][dynamic][roundtrip]") {
    auto t = type_def("Team_fRT")
        .field<std::string>("team_name")
        .field<std::vector<SimpleArgs>>("members");
    auto obj = t.create();
    obj.set("team_name", std::string("Pirates"));
    SimpleArgs a1; a1.name = "Alice"; a1.age = 30; a1.active = true;
    SimpleArgs a2; a2.name = "Bob"; a2.age = 25; a2.active = false;
    obj.set("members", std::vector<SimpleArgs>{a1, a2});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    REQUIRE(obj2.get<std::string>("team_name") == "Pirates");
    auto members = obj2.get<std::vector<SimpleArgs>>("members");
    REQUIRE(members.size() == 2);
    REQUIRE(members[0].name.value == "Alice");
    REQUIRE(members[1].name.value == "Bob");
}

TEST_CASE("typed from_json: round-trip map of structs", "[from_json][typed][roundtrip]") {
    ServiceMap original;
    Endpoint api; api.url = "https://api.example.com"; api.port = 443;
    Endpoint db; db.url = "localhost"; db.port = 5432;
    original.services.value = {{"api", api}, {"db", db}};

    auto j = to_json(original);
    auto restored = from_json<ServiceMap>(j);

    REQUIRE(restored.services.value.at("api").url.value == "https://api.example.com");
    REQUIRE(restored.services.value.at("api").port.value == 443);
    REQUIRE(restored.services.value.at("db").port.value == 5432);
}

TEST_CASE("dynamic from_json: round-trip map of structs", "[from_json][dynamic][roundtrip]") {
    auto t = type_def("ServiceMap_fRT")
        .field<std::map<std::string, Endpoint>>("services");
    auto obj = t.create();
    Endpoint api; api.url = "https://api.example.com"; api.port = 443;
    Endpoint db; db.url = "localhost"; db.port = 5432;
    obj.set("services", std::map<std::string, Endpoint>{{"api", api}, {"db", db}});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto services = obj2.get<std::map<std::string, Endpoint>>("services");
    REQUIRE(services.at("api").url.value == "https://api.example.com");
    REQUIRE(services.at("api").port.value == 443);
    REQUIRE(services.at("db").port.value == 5432);
}

TEST_CASE("typed from_json: round-trip optional present", "[from_json][typed][roundtrip]") {
    MaybeNickname original;
    original.name = "Alice";
    original.nickname.value = "Ali";

    auto j = to_json(original);
    auto restored = from_json<MaybeNickname>(j);

    REQUIRE(restored.nickname.value.has_value());
    REQUIRE(*restored.nickname.value == "Ali");
}

TEST_CASE("dynamic from_json: round-trip optional present", "[from_json][dynamic][roundtrip]") {
    auto t = type_def("MaybeNickname_fRTP")
        .field<std::string>("name")
        .field<std::optional<std::string>>("nickname");
    auto obj = t.create();
    obj.set("name", std::string("Alice"));
    obj.set("nickname", std::optional<std::string>("Ali"));

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto nn = obj2.get<std::optional<std::string>>("nickname");
    REQUIRE(nn.has_value());
    REQUIRE(*nn == "Ali");
}

TEST_CASE("typed from_json: round-trip optional absent", "[from_json][typed][roundtrip]") {
    MaybeNickname original;
    original.name = "Bob";

    auto j = to_json(original);
    auto restored = from_json<MaybeNickname>(j);

    REQUIRE(!restored.nickname.value.has_value());
}

TEST_CASE("dynamic from_json: round-trip optional absent", "[from_json][dynamic][roundtrip]") {
    auto t = type_def("MaybeNickname_fRTA")
        .field<std::string>("name")
        .field<std::optional<std::string>>("nickname");
    auto obj = t.create();
    obj.set("name", std::string("Bob"));

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto nn = obj2.get<std::optional<std::string>>("nickname");
    REQUIRE(!nn.has_value());
}

// ═════════════════════════════════════════════════════════════════════════
// Typed from_json — error cases
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: throws on non-object JSON", "[from_json][typed][throw]") {
    REQUIRE_THROWS_AS(from_json<SimpleArgs>(json(42)), std::logic_error);
    REQUIRE_THROWS_AS(from_json<SimpleArgs>(json("string")), std::logic_error);
    REQUIRE_THROWS_AS(from_json<SimpleArgs>(json::array()), std::logic_error);
    REQUIRE_THROWS_AS(from_json<SimpleArgs>(json(true)), std::logic_error);
}

TEST_CASE("dynamic from_json: throws on non-object JSON", "[from_json][dynamic][throw]") {
    auto t = type_def("SimpleArgs_fThrow")
        .field<std::string>("name")
        .field<int>("age")
        .field<bool>("active");

    REQUIRE_THROWS_AS(t.create(json(42)), std::logic_error);
    REQUIRE_THROWS_AS(t.create(json("string")), std::logic_error);
    REQUIRE_THROWS_AS(t.create(json::array()), std::logic_error);
    REQUIRE_THROWS_AS(t.create(json(true)), std::logic_error);
}

TEST_CASE("typed from_json: throws on type mismatch — string field gets int", "[from_json][typed][throw]") {
    auto j = json{{"name", 42}, {"age", 30}, {"active", true}};
    REQUIRE_THROWS_AS(from_json<SimpleArgs>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: throws on type mismatch — string field gets int", "[from_json][dynamic][throw]") {
    auto t = type_def("SimpleArgs_fTM1")
        .field<std::string>("name")
        .field<int>("age")
        .field<bool>("active");

    auto j = json{{"name", 42}, {"age", 30}, {"active", true}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

TEST_CASE("typed from_json: throws on type mismatch — int field gets string", "[from_json][typed][throw]") {
    auto j = json{{"name", "Alice"}, {"age", "thirty"}, {"active", true}};
    REQUIRE_THROWS_AS(from_json<SimpleArgs>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: throws on type mismatch — int field gets string", "[from_json][dynamic][throw]") {
    auto t = type_def("SimpleArgs_fTM2")
        .field<std::string>("name")
        .field<int>("age")
        .field<bool>("active");

    auto j = json{{"name", "Alice"}, {"age", "thirty"}, {"active", true}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

TEST_CASE("typed from_json: throws on type mismatch — bool field gets int", "[from_json][typed][throw]") {
    auto j = json{{"name", "Alice"}, {"age", 30}, {"active", 1}};
    REQUIRE_THROWS_AS(from_json<SimpleArgs>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: throws on type mismatch — bool field gets int", "[from_json][dynamic][throw]") {
    auto t = type_def("SimpleArgs_fTM3")
        .field<std::string>("name")
        .field<int>("age")
        .field<bool>("active");

    auto j = json{{"name", "Alice"}, {"age", 30}, {"active", 1}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

TEST_CASE("typed from_json: throws on type mismatch — vector field gets object", "[from_json][typed][throw]") {
    auto j = json{{"title", "test"}, {"tags", {{"not", "an array"}}}};
    REQUIRE_THROWS_AS(from_json<TaggedItem>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: throws on type mismatch — vector field gets object", "[from_json][dynamic][throw]") {
    auto t = type_def("TaggedItem_fTM")
        .field<std::string>("title")
        .field<std::vector<std::string>>("tags");

    auto j = json{{"title", "test"}, {"tags", {{"not", "an array"}}}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

TEST_CASE("typed from_json: throws on type mismatch — nested struct gets array", "[from_json][typed][throw]") {
    auto j = json{{"name", "Bob"}, {"address", json::array({1, 2, 3})}};
    REQUIRE_THROWS_AS(from_json<Person>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: throws on type mismatch — nested struct gets array", "[from_json][dynamic][throw]") {
    auto t = type_def("Person_fTM")
        .field<std::string>("name")
        .field<Address>("address");

    auto j = json{{"name", "Bob"}, {"address", json::array({1, 2, 3})}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

TEST_CASE("typed from_json: throws on invalid JSON string", "[from_json][typed][throw]") {
    REQUIRE_THROWS(from_json<SimpleArgs>(std::string("not valid json")));
}

TEST_CASE("typed from_json: throws on type mismatch — map field gets array", "[from_json][typed][throw]") {
    auto j = json{{"settings", json::array({1, 2, 3})}};
    REQUIRE_THROWS_AS(from_json<EmptyConfig>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: throws on type mismatch — map field gets array", "[from_json][dynamic][throw]") {
    auto t = type_def("EmptyConfig_fTM")
        .field<std::map<std::string, int>>("settings");

    auto j = json{{"settings", json::array({1, 2, 3})}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// Nested collections — from_json + round-trip
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: vector of vectors", "[from_json][typed][nested_collections]") {
    auto j = json{{"matrix", {{1, 2, 3}, {4, 5}, {6}}}};
    auto vv = from_json<VecOfVecs>(j);

    REQUIRE(vv.matrix.value.size() == 3);
    REQUIRE(vv.matrix.value[0] == std::vector<int>{1, 2, 3});
    REQUIRE(vv.matrix.value[1] == std::vector<int>{4, 5});
    REQUIRE(vv.matrix.value[2] == std::vector<int>{6});
}

TEST_CASE("dynamic from_json: vector of vectors", "[from_json][dynamic][nested_collections]") {
    auto t = type_def("VecOfVecs_f")
        .field<std::vector<std::vector<int>>>("matrix");

    auto j = json{{"matrix", {{1, 2, 3}, {4, 5}, {6}}}};
    auto obj = t.create(j);

    auto matrix = obj.get<std::vector<std::vector<int>>>("matrix");
    REQUIRE(matrix.size() == 3);
    REQUIRE(matrix[0] == std::vector<int>{1, 2, 3});
    REQUIRE(matrix[1] == std::vector<int>{4, 5});
    REQUIRE(matrix[2] == std::vector<int>{6});
}

TEST_CASE("typed round-trip: vector of vectors", "[json][typed][nested_collections][roundtrip]") {
    VecOfVecs original;
    original.matrix.value = {{1, 2}, {3, 4, 5}, {}};

    auto restored = from_json<VecOfVecs>(to_json(original));

    REQUIRE(restored.matrix.value.size() == 3);
    REQUIRE(restored.matrix.value[0] == std::vector<int>{1, 2});
    REQUIRE(restored.matrix.value[2].empty());
}

TEST_CASE("dynamic round-trip: vector of vectors", "[json][dynamic][nested_collections][roundtrip]") {
    auto t = type_def("VecOfVecs_fRT")
        .field<std::vector<std::vector<int>>>("matrix");
    auto obj = t.create();
    obj.set("matrix", std::vector<std::vector<int>>{{1, 2}, {3, 4, 5}, {}});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto matrix = obj2.get<std::vector<std::vector<int>>>("matrix");
    REQUIRE(matrix.size() == 3);
    REQUIRE(matrix[0] == std::vector<int>{1, 2});
    REQUIRE(matrix[2].empty());
}

TEST_CASE("typed from_json: map of vectors", "[from_json][typed][nested_collections]") {
    auto j = json{{"grouped", {{"fruits", {"apple", "banana"}}, {"colors", {"red"}}}}};
    auto mv = from_json<MapOfVecs>(j);

    REQUIRE(mv.grouped.value.at("fruits").size() == 2);
    REQUIRE(mv.grouped.value.at("fruits")[0] == "apple");
    REQUIRE(mv.grouped.value.at("colors").size() == 1);
}

TEST_CASE("dynamic from_json: map of vectors", "[from_json][dynamic][nested_collections]") {
    auto t = type_def("MapOfVecs_f")
        .field<std::map<std::string, std::vector<std::string>>>("grouped");

    auto j = json{{"grouped", {{"fruits", {"apple", "banana"}}, {"colors", {"red"}}}}};
    auto obj = t.create(j);

    auto grouped = obj.get<std::map<std::string, std::vector<std::string>>>("grouped");
    REQUIRE(grouped.at("fruits").size() == 2);
    REQUIRE(grouped.at("fruits")[0] == "apple");
    REQUIRE(grouped.at("colors").size() == 1);
}

TEST_CASE("typed round-trip: map of vectors", "[json][typed][nested_collections][roundtrip]") {
    MapOfVecs original;
    original.grouped.value = {{"a", {"x", "y"}}, {"b", {}}};

    auto restored = from_json<MapOfVecs>(to_json(original));

    REQUIRE(restored.grouped.value.at("a") == std::vector<std::string>{"x", "y"});
    REQUIRE(restored.grouped.value.at("b").empty());
}

TEST_CASE("dynamic round-trip: map of vectors", "[json][dynamic][nested_collections][roundtrip]") {
    auto t = type_def("MapOfVecs_fRT")
        .field<std::map<std::string, std::vector<std::string>>>("grouped");
    auto obj = t.create();
    obj.set("grouped", std::map<std::string, std::vector<std::string>>{{"a", {"x", "y"}}, {"b", {}}});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto grouped = obj2.get<std::map<std::string, std::vector<std::string>>>("grouped");
    REQUIRE(grouped.at("a") == std::vector<std::string>{"x", "y"});
    REQUIRE(grouped.at("b").empty());
}

TEST_CASE("typed from_json: vector of maps", "[from_json][typed][nested_collections]") {
    auto j = json{{"records", {{{"x", 1}, {"y", 2}}, {{"z", 3}}}}};
    auto vm = from_json<VecOfMaps>(j);

    REQUIRE(vm.records.value.size() == 2);
    REQUIRE(vm.records.value[0].at("x") == 1);
    REQUIRE(vm.records.value[1].at("z") == 3);
}

TEST_CASE("dynamic from_json: vector of maps", "[from_json][dynamic][nested_collections]") {
    auto t = type_def("VecOfMaps_f")
        .field<std::vector<std::map<std::string, int>>>("records");

    auto j = json{{"records", {{{"x", 1}, {"y", 2}}, {{"z", 3}}}}};
    auto obj = t.create(j);

    auto records = obj.get<std::vector<std::map<std::string, int>>>("records");
    REQUIRE(records.size() == 2);
    REQUIRE(records[0].at("x") == 1);
    REQUIRE(records[1].at("z") == 3);
}

TEST_CASE("typed round-trip: vector of maps", "[json][typed][nested_collections][roundtrip]") {
    VecOfMaps original;
    original.records.value = {{{"a", 1}}, {{"b", 2}, {"c", 3}}};

    auto restored = from_json<VecOfMaps>(to_json(original));

    REQUIRE(restored.records.value.size() == 2);
    REQUIRE(restored.records.value[0].at("a") == 1);
    REQUIRE(restored.records.value[1].size() == 2);
}

TEST_CASE("dynamic round-trip: vector of maps", "[json][dynamic][nested_collections][roundtrip]") {
    auto t = type_def("VecOfMaps_fRT")
        .field<std::vector<std::map<std::string, int>>>("records");
    auto obj = t.create();
    obj.set("records", std::vector<std::map<std::string, int>>{{{"a", 1}}, {{"b", 2}, {"c", 3}}});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto records = obj2.get<std::vector<std::map<std::string, int>>>("records");
    REQUIRE(records.size() == 2);
    REQUIRE(records[0].at("a") == 1);
    REQUIRE(records[1].size() == 2);
}

// ═════════════════════════════════════════════════════════════════════════
// Optional + collection combos — from_json + round-trip
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: optional vector present", "[from_json][typed][optional_collection]") {
    auto j = json{{"maybe_tags", {"a", "b", "c"}}};
    auto ov = from_json<OptionalVec>(j);

    REQUIRE(ov.maybe_tags.value.has_value());
    REQUIRE(ov.maybe_tags.value->size() == 3);
    REQUIRE((*ov.maybe_tags.value)[0] == "a");
}

TEST_CASE("dynamic from_json: optional vector present", "[from_json][dynamic][optional_collection]") {
    auto t = type_def("OptionalVec_fP")
        .field<std::optional<std::vector<std::string>>>("maybe_tags");

    auto j = json{{"maybe_tags", {"a", "b", "c"}}};
    auto obj = t.create(j);

    auto mt = obj.get<std::optional<std::vector<std::string>>>("maybe_tags");
    REQUIRE(mt.has_value());
    REQUIRE(mt->size() == 3);
    REQUIRE((*mt)[0] == "a");
}

TEST_CASE("typed from_json: optional vector null", "[from_json][typed][optional_collection]") {
    auto j = json{{"maybe_tags", nullptr}};
    auto ov = from_json<OptionalVec>(j);

    REQUIRE(!ov.maybe_tags.value.has_value());
}

TEST_CASE("dynamic from_json: optional vector null", "[from_json][dynamic][optional_collection]") {
    auto t = type_def("OptionalVec_fNull")
        .field<std::optional<std::vector<std::string>>>("maybe_tags");

    auto j = json{{"maybe_tags", nullptr}};
    auto obj = t.create(j);

    auto mt = obj.get<std::optional<std::vector<std::string>>>("maybe_tags");
    REQUIRE(!mt.has_value());
}

TEST_CASE("typed from_json: optional vector missing key", "[from_json][typed][optional_collection]") {
    auto j = json::object();
    auto ov = from_json<OptionalVec>(j);

    REQUIRE(!ov.maybe_tags.value.has_value());
}

TEST_CASE("dynamic from_json: optional vector missing key", "[from_json][dynamic][optional_collection]") {
    auto t = type_def("OptionalVec_fMiss")
        .field<std::optional<std::vector<std::string>>>("maybe_tags");

    auto j = json::object();
    auto obj = t.create(j);

    auto mt = obj.get<std::optional<std::vector<std::string>>>("maybe_tags");
    REQUIRE(!mt.has_value());
}

TEST_CASE("typed round-trip: optional vector present", "[json][typed][optional_collection][roundtrip]") {
    OptionalVec original;
    original.maybe_tags.value = std::vector<std::string>{"x", "y"};

    auto restored = from_json<OptionalVec>(to_json(original));

    REQUIRE(restored.maybe_tags.value.has_value());
    REQUIRE(*restored.maybe_tags.value == std::vector<std::string>{"x", "y"});
}

TEST_CASE("dynamic round-trip: optional vector present", "[json][dynamic][optional_collection][roundtrip]") {
    auto t = type_def("OptionalVec_fRTP")
        .field<std::optional<std::vector<std::string>>>("maybe_tags");
    auto obj = t.create();
    obj.set("maybe_tags", std::optional<std::vector<std::string>>(std::vector<std::string>{"x", "y"}));

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto mt = obj2.get<std::optional<std::vector<std::string>>>("maybe_tags");
    REQUIRE(mt.has_value());
    REQUIRE(*mt == std::vector<std::string>{"x", "y"});
}

TEST_CASE("typed round-trip: optional vector absent", "[json][typed][optional_collection][roundtrip]") {
    OptionalVec original;

    auto restored = from_json<OptionalVec>(to_json(original));

    REQUIRE(!restored.maybe_tags.value.has_value());
}

TEST_CASE("dynamic round-trip: optional vector absent", "[json][dynamic][optional_collection][roundtrip]") {
    auto t = type_def("OptionalVec_fRTA")
        .field<std::optional<std::vector<std::string>>>("maybe_tags");
    auto obj = t.create();

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto mt = obj2.get<std::optional<std::vector<std::string>>>("maybe_tags");
    REQUIRE(!mt.has_value());
}

TEST_CASE("typed from_json: vector of optionals with nulls", "[from_json][typed][optional_collection]") {
    auto j = json{{"scores", {1, nullptr, 3, nullptr, 5}}};
    auto vo = from_json<VecOfOptionals>(j);

    REQUIRE(vo.scores.value.size() == 5);
    REQUIRE(vo.scores.value[0].has_value());
    REQUIRE(*vo.scores.value[0] == 1);
    REQUIRE(!vo.scores.value[1].has_value());
    REQUIRE(*vo.scores.value[2] == 3);
    REQUIRE(!vo.scores.value[3].has_value());
    REQUIRE(*vo.scores.value[4] == 5);
}

TEST_CASE("dynamic from_json: vector of optionals with nulls", "[from_json][dynamic][optional_collection]") {
    auto t = type_def("VecOfOptionals_f")
        .field<std::vector<std::optional<int>>>("scores");

    auto j = json{{"scores", {1, nullptr, 3, nullptr, 5}}};
    auto obj = t.create(j);

    auto scores = obj.get<std::vector<std::optional<int>>>("scores");
    REQUIRE(scores.size() == 5);
    REQUIRE(scores[0].has_value());
    REQUIRE(*scores[0] == 1);
    REQUIRE(!scores[1].has_value());
    REQUIRE(*scores[2] == 3);
    REQUIRE(!scores[3].has_value());
    REQUIRE(*scores[4] == 5);
}

TEST_CASE("typed round-trip: vector of optionals", "[json][typed][optional_collection][roundtrip]") {
    VecOfOptionals original;
    original.scores.value = {10, std::nullopt, 30};

    auto restored = from_json<VecOfOptionals>(to_json(original));

    REQUIRE(restored.scores.value.size() == 3);
    REQUIRE(*restored.scores.value[0] == 10);
    REQUIRE(!restored.scores.value[1].has_value());
    REQUIRE(*restored.scores.value[2] == 30);
}

TEST_CASE("dynamic round-trip: vector of optionals", "[json][dynamic][optional_collection][roundtrip]") {
    auto t = type_def("VecOfOptionals_fRT")
        .field<std::vector<std::optional<int>>>("scores");
    auto obj = t.create();
    obj.set("scores", std::vector<std::optional<int>>{10, std::nullopt, 30});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto scores = obj2.get<std::vector<std::optional<int>>>("scores");
    REQUIRE(scores.size() == 3);
    REQUIRE(*scores[0] == 10);
    REQUIRE(!scores[1].has_value());
    REQUIRE(*scores[2] == 30);
}

// ═════════════════════════════════════════════════════════════════════════
// Deep nesting (3 levels + collections) — from_json + round-trip
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: 3-level deep nested struct", "[from_json][typed][deep_nesting]") {
    auto j = json{
        {"name", "root"},
        {"middle", {
            {"label", "mid"},
            {"inner", {{"value", "leaf"}}}
        }},
        {"items", {
            {{"value", "a"}},
            {{"value", "b"}},
            {{"value", "c"}}
        }}
    };
    auto obj = from_json<DeepOuter>(j);

    REQUIRE(obj.name.value == "root");
    REQUIRE(obj.middle.value.label.value == "mid");
    REQUIRE(obj.middle.value.inner.value.value.value == "leaf");
    REQUIRE(obj.items.value.size() == 3);
    REQUIRE(obj.items.value[0].value.value == "a");
    REQUIRE(obj.items.value[2].value.value == "c");
}

TEST_CASE("dynamic from_json: 3-level deep nested struct", "[from_json][dynamic][deep_nesting]") {
    auto t = type_def("DeepOuter_f")
        .field<std::string>("name")
        .field<DeepMiddle>("middle")
        .field<std::vector<DeepInner>>("items");

    auto j = json{
        {"name", "root"},
        {"middle", {
            {"label", "mid"},
            {"inner", {{"value", "leaf"}}}
        }},
        {"items", {
            {{"value", "a"}},
            {{"value", "b"}},
            {{"value", "c"}}
        }}
    };
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "root");
    auto mid = obj.get<DeepMiddle>("middle");
    REQUIRE(mid.label.value == "mid");
    REQUIRE(mid.inner.value.value.value == "leaf");
    auto items = obj.get<std::vector<DeepInner>>("items");
    REQUIRE(items.size() == 3);
    REQUIRE(items[0].value.value == "a");
    REQUIRE(items[2].value.value == "c");
}

TEST_CASE("typed round-trip: 3-level deep nested struct", "[json][typed][deep_nesting][roundtrip]") {
    DeepOuter original;
    original.name = "root";
    original.middle.value.label = "mid";
    original.middle.value.inner.value.value = "leaf";
    DeepInner ix; ix.value = "x";
    DeepInner iy; iy.value = "y";
    original.items.value = {ix, iy};

    auto restored = from_json<DeepOuter>(to_json(original));

    REQUIRE(restored.name.value == "root");
    REQUIRE(restored.middle.value.label.value == "mid");
    REQUIRE(restored.middle.value.inner.value.value.value == "leaf");
    REQUIRE(restored.items.value.size() == 2);
    REQUIRE(restored.items.value[0].value.value == "x");
}

TEST_CASE("dynamic round-trip: 3-level deep nested struct", "[json][dynamic][deep_nesting][roundtrip]") {
    auto t = type_def("DeepOuter_fRT")
        .field<std::string>("name")
        .field<DeepMiddle>("middle")
        .field<std::vector<DeepInner>>("items");
    auto obj = t.create();
    obj.set("name", std::string("root"));
    DeepMiddle mid; mid.label = "mid"; mid.inner.value.value = "leaf";
    obj.set("middle", mid);
    DeepInner ix; ix.value = "x";
    DeepInner iy; iy.value = "y";
    obj.set("items", std::vector<DeepInner>{ix, iy});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    REQUIRE(obj2.get<std::string>("name") == "root");
    auto mid2 = obj2.get<DeepMiddle>("middle");
    REQUIRE(mid2.label.value == "mid");
    REQUIRE(mid2.inner.value.value.value == "leaf");
    auto items = obj2.get<std::vector<DeepInner>>("items");
    REQUIRE(items.size() == 2);
    REQUIRE(items[0].value.value == "x");
}

TEST_CASE("typed from_json: deep nested with missing inner keys", "[from_json][typed][deep_nesting]") {
    auto j = json{{"name", "root"}, {"middle", {{"label", "mid"}}}};
    auto obj = from_json<DeepOuter>(j);

    REQUIRE(obj.name.value == "root");
    REQUIRE(obj.middle.value.label.value == "mid");
    REQUIRE(obj.middle.value.inner.value.value.value.empty());
    REQUIRE(obj.items.value.empty());
}

TEST_CASE("dynamic from_json: deep nested with missing inner keys", "[from_json][dynamic][deep_nesting]") {
    auto t = type_def("DeepOuter_fMiss")
        .field<std::string>("name")
        .field<DeepMiddle>("middle")
        .field<std::vector<DeepInner>>("items");

    auto j = json{{"name", "root"}, {"middle", {{"label", "mid"}}}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("name") == "root");
    auto mid = obj.get<DeepMiddle>("middle");
    REQUIRE(mid.label.value == "mid");
    REQUIRE(mid.inner.value.value.value.empty());
    REQUIRE(obj.get<std::vector<DeepInner>>("items").empty());
}

TEST_CASE("typed from_json: vector of persons with nested addresses", "[from_json][typed][deep_nesting]") {
    auto j = json{
        {"org", "Collab"},
        {"people", {
            {{"name", "Alice"}, {"address", {{"street", "1st St"}, {"zip", "10001"}}}},
            {{"name", "Bob"}, {"address", {{"street", "2nd Ave"}, {"zip", "20002"}}}}
        }}
    };
    auto pl = from_json<PersonList>(j);

    REQUIRE(pl.org.value == "Collab");
    REQUIRE(pl.people.value.size() == 2);
    REQUIRE(pl.people.value[0].name.value == "Alice");
    REQUIRE(pl.people.value[0].address.value.street.value == "1st St");
    REQUIRE(pl.people.value[1].address.value.zip.value == "20002");
}

TEST_CASE("dynamic from_json: vector of persons with nested addresses", "[from_json][dynamic][deep_nesting]") {
    auto t = type_def("PersonList_f")
        .field<std::string>("org")
        .field<std::vector<Person>>("people");

    auto j = json{
        {"org", "Collab"},
        {"people", {
            {{"name", "Alice"}, {"address", {{"street", "1st St"}, {"zip", "10001"}}}},
            {{"name", "Bob"}, {"address", {{"street", "2nd Ave"}, {"zip", "20002"}}}}
        }}
    };
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("org") == "Collab");
    auto people = obj.get<std::vector<Person>>("people");
    REQUIRE(people.size() == 2);
    REQUIRE(people[0].name.value == "Alice");
    REQUIRE(people[0].address.value.street.value == "1st St");
    REQUIRE(people[1].address.value.zip.value == "20002");
}

TEST_CASE("typed round-trip: vector of persons with nested addresses", "[json][typed][deep_nesting][roundtrip]") {
    PersonList original;
    original.org = "Pirates";
    Person p; p.name = "Rex"; p.address.value.street = "Dog Ln"; p.address.value.zip = "99999";
    original.people.value = {p};

    auto restored = from_json<PersonList>(to_json(original));

    REQUIRE(restored.people.value.size() == 1);
    REQUIRE(restored.people.value[0].address.value.street.value == "Dog Ln");
}

TEST_CASE("dynamic round-trip: vector of persons with nested addresses", "[json][dynamic][deep_nesting][roundtrip]") {
    auto t = type_def("PersonList_fRT")
        .field<std::string>("org")
        .field<std::vector<Person>>("people");
    auto obj = t.create();
    obj.set("org", std::string("Pirates"));
    Person p; p.name = "Rex"; p.address.value.street = "Dog Ln"; p.address.value.zip = "99999";
    obj.set("people", std::vector<Person>{p});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto people = obj2.get<std::vector<Person>>("people");
    REQUIRE(people.size() == 1);
    REQUIRE(people[0].address.value.street.value == "Dog Ln");
}

// ═════════════════════════════════════════════════════════════════════════
// Dense containers with struct values — from_json + round-trip
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: ankerl dense map of structs", "[from_json][typed][dense]") {
    auto j = json{{"locations", {
        {"home", {{"street", "123 Main"}, {"zip", "97201"}}},
        {"work", {{"street", "456 Corp"}, {"zip", "97202"}}}
    }}};
    auto dm = from_json<DenseMapOfStructs>(j);

    REQUIRE(dm.locations.value.at("home").street.value == "123 Main");
    REQUIRE(dm.locations.value.at("work").zip.value == "97202");
}

TEST_CASE("dynamic from_json: ankerl dense map of structs", "[from_json][dynamic][dense]") {
    auto t = type_def("DenseMapOfStructs_f")
        .field<ankerl::unordered_dense::map<std::string, Address>>("locations");

    auto j = json{{"locations", {
        {"home", {{"street", "123 Main"}, {"zip", "97201"}}},
        {"work", {{"street", "456 Corp"}, {"zip", "97202"}}}
    }}};
    auto obj = t.create(j);

    auto locs = obj.get<ankerl::unordered_dense::map<std::string, Address>>("locations");
    REQUIRE(locs.at("home").street.value == "123 Main");
    REQUIRE(locs.at("work").zip.value == "97202");
}

TEST_CASE("typed round-trip: ankerl dense map of structs", "[json][typed][dense][roundtrip]") {
    DenseMapOfStructs original;
    Address a; a.street = "Dog St"; a.zip = "00000";
    original.locations.value = {{"park", a}};

    auto restored = from_json<DenseMapOfStructs>(to_json(original));

    REQUIRE(restored.locations.value.at("park").street.value == "Dog St");
}

TEST_CASE("dynamic round-trip: ankerl dense map of structs", "[json][dynamic][dense][roundtrip]") {
    auto t = type_def("DenseMapOfStructs_fRT")
        .field<ankerl::unordered_dense::map<std::string, Address>>("locations");
    auto obj = t.create();
    Address a; a.street = "Dog St"; a.zip = "00000";
    obj.set("locations", ankerl::unordered_dense::map<std::string, Address>{{"park", a}});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto locs = obj2.get<ankerl::unordered_dense::map<std::string, Address>>("locations");
    REQUIRE(locs.at("park").street.value == "Dog St");
}

// ═════════════════════════════════════════════════════════════════════════
// Set edge cases
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: set deduplicates array with duplicates", "[from_json][typed][set][edge]") {
    auto j = json{{"tags", {"alpha", "beta", "alpha", "gamma", "beta"}}};
    auto ts = from_json<TagSet>(j);

    REQUIRE(ts.tags.value.size() == 3);
    REQUIRE(ts.tags.value.count("alpha") == 1);
    REQUIRE(ts.tags.value.count("beta") == 1);
    REQUIRE(ts.tags.value.count("gamma") == 1);
}

TEST_CASE("dynamic from_json: set deduplicates array with duplicates", "[from_json][dynamic][set][edge]") {
    auto t = type_def("TagSet_fDedup")
        .field<std::set<std::string>>("tags");

    auto j = json{{"tags", {"alpha", "beta", "alpha", "gamma", "beta"}}};
    auto obj = t.create(j);

    auto tags = obj.get<std::set<std::string>>("tags");
    REQUIRE(tags.size() == 3);
    REQUIRE(tags.count("alpha") == 1);
    REQUIRE(tags.count("beta") == 1);
    REQUIRE(tags.count("gamma") == 1);
}

TEST_CASE("typed from_json: unordered_set deduplicates", "[from_json][typed][set][edge]") {
    auto j = json{{"ids", {1, 2, 3, 1, 2}}};
    auto is = from_json<IdSet>(j);

    REQUIRE(is.ids.value.size() == 3);
}

TEST_CASE("dynamic from_json: unordered_set deduplicates", "[from_json][dynamic][set][edge]") {
    auto t = type_def("IdSet_fDedup")
        .field<std::unordered_set<int>>("ids");

    auto j = json{{"ids", {1, 2, 3, 1, 2}}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::unordered_set<int>>("ids").size() == 3);
}

TEST_CASE("typed from_json: dense set deduplicates", "[from_json][typed][dense][edge]") {
    auto j = json{{"names", {"x", "y", "x", "z", "y"}}};
    auto ds = from_json<DenseSetStruct>(j);

    REQUIRE(ds.names.value.size() == 3);
}

TEST_CASE("dynamic from_json: dense set deduplicates", "[from_json][dynamic][dense][edge]") {
    auto t = type_def("DenseSetStruct_fDedup")
        .field<ankerl::unordered_dense::set<std::string>>("names");

    auto j = json{{"names", {"x", "y", "x", "z", "y"}}};
    auto obj = t.create(j);

    REQUIRE(obj.get<ankerl::unordered_dense::set<std::string>>("names").size() == 3);
}

TEST_CASE("typed round-trip: set preserves unique elements", "[json][typed][set][roundtrip]") {
    TagSet original;
    original.tags.value = {"a", "b", "c"};

    auto j = to_json(original);
    REQUIRE(j["tags"].size() == 3);

    auto restored = from_json<TagSet>(j);
    REQUIRE(restored.tags.value.size() == 3);
    REQUIRE(restored.tags.value.count("a") == 1);
    REQUIRE(restored.tags.value.count("b") == 1);
    REQUIRE(restored.tags.value.count("c") == 1);
}

TEST_CASE("dynamic round-trip: set preserves unique elements", "[json][dynamic][set][roundtrip]") {
    auto t = type_def("TagSet_fRT")
        .field<std::set<std::string>>("tags");
    auto obj = t.create();
    obj.set("tags", std::set<std::string>{"a", "b", "c"});

    auto j = obj.to_json();
    REQUIRE(j["tags"].size() == 3);

    auto obj2 = t.create(j);
    auto tags = obj2.get<std::set<std::string>>("tags");
    REQUIRE(tags.size() == 3);
    REQUIRE(tags.count("a") == 1);
    REQUIRE(tags.count("b") == 1);
    REQUIRE(tags.count("c") == 1);
}

// ═════════════════════════════════════════════════════════════════════════
// Dynamic from_json — factory (existing tests)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic from_json: factory creates instance with values", "[from_json][dynamic]") {
    auto t = type_def("Event")
        .field<std::string>("title")
        .field<int>("count", 100)
        .field<bool>("active", false);

    auto j = json{{"title", "Party"}, {"count", 50}, {"active", true}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("title") == "Party");
    REQUIRE(obj.get<int>("count") == 50);
    REQUIRE(obj.get<bool>("active") == true);
}

TEST_CASE("dynamic from_json: factory preserves defaults for missing keys", "[from_json][dynamic]") {
    auto t = type_def("Event_facDef")
        .field<std::string>("title", std::string("Untitled"))
        .field<int>("count", 100);

    auto j = json{{"title", "Custom"}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("title") == "Custom");
    REQUIRE(obj.get<int>("count") == 100);
}

TEST_CASE("dynamic from_json: factory ignores extra keys", "[from_json][dynamic]") {
    auto t = type_def("Event_facExtra")
        .field<std::string>("title");

    auto j = json{{"title", "Party"}, {"unknown", 42}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::string>("title") == "Party");
}

TEST_CASE("dynamic from_json: factory with empty JSON", "[from_json][dynamic]") {
    auto t = type_def("Event_facEmpty")
        .field<std::string>("title", std::string("Default"))
        .field<int>("count", 0);

    auto obj = t.create(json::object());

    REQUIRE(obj.get<std::string>("title") == "Default");
    REQUIRE(obj.get<int>("count") == 0);
}

// ═════════════════════════════════════════════════════════════════════════
// Dynamic load_json — in-place overlay
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic load_json: overlays values onto existing instance", "[from_json][dynamic][load_json]") {
    auto t = type_def("Event_lo")
        .field<std::string>("title")
        .field<int>("count", 100);
    auto obj = t.create();

    obj.set("title", std::string("Original"));

    obj.load_json(json{{"count", 50}});

    REQUIRE(obj.get<std::string>("title") == "Original");
    REQUIRE(obj.get<int>("count") == 50);
}

TEST_CASE("dynamic load_json: overwrites previously set values", "[from_json][dynamic][load_json]") {
    auto t = type_def("Event_loOW")
        .field<std::string>("title");
    auto obj = t.create();

    obj.set("title", std::string("First"));
    obj.load_json(json{{"title", "Second"}});

    REQUIRE(obj.get<std::string>("title") == "Second");
}

TEST_CASE("dynamic load_json: throws on non-object JSON", "[from_json][dynamic][load_json][throw]") {
    auto t = type_def("Event_loThr")
        .field<int>("x");
    auto obj = t.create();

    REQUIRE_THROWS_AS(obj.load_json(json(42)), std::logic_error);
    REQUIRE_THROWS_AS(obj.load_json(json::array()), std::logic_error);
    REQUIRE_THROWS_AS(obj.load_json(json("string")), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// Dynamic from_json — error cases (existing)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic from_json: factory throws on non-object", "[from_json][dynamic][throw]") {
    auto t = type_def("Event_facThr")
        .field<int>("x");

    REQUIRE_THROWS_AS(t.create(json(42)), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// Dynamic from_json — type_instance preserves type() access
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic from_json: type() metadata accessible after deserialization", "[from_json][dynamic][meta]") {
    struct endpoint_info { const char* path = ""; };

    auto t = type_def("Event_meta2")
        .meta<endpoint_info>({.path = "/events"})
        .field<std::string>("title")
        .field<int>("count", 100);

    auto obj = t.create(json{{"title", "Party"}});

    REQUIRE(obj.type().name() == "Event_meta2");
    REQUIRE(obj.type().has_meta<endpoint_info>());
    REQUIRE(obj.type().field_count() == 2);
}

// ═════════════════════════════════════════════════════════════════════════
// Enum fields — from_json
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: enum field deserializes from string", "[from_json][typed][enum]") {
    auto j = json{{"method", "PUT"}, {"name", "update-dog"}};
    auto es = from_json<EnumStruct>(j);

    REQUIRE(es.method.value == HttpMethod::PUT);
    REQUIRE(es.name.value == "update-dog");
}

TEST_CASE("dynamic from_json: enum field deserializes from string", "[from_json][dynamic][enum]") {
    auto t = type_def("EnumStruct_f")
        .field<HttpMethod>("method")
        .field<std::string>("name");

    auto j = json{{"method", "PUT"}, {"name", "update-dog"}};
    auto obj = t.create(j);

    REQUIRE(obj.get<HttpMethod>("method") == HttpMethod::PUT);
    REQUIRE(obj.get<std::string>("name") == "update-dog");
}

TEST_CASE("typed from_json: enum field also accepts integer", "[from_json][typed][enum]") {
    auto j = json{{"method", 2}, {"name", "update-dog"}};
    auto es = from_json<EnumStruct>(j);

    REQUIRE(es.method.value == HttpMethod::PUT);
}

TEST_CASE("dynamic from_json: enum field also accepts integer", "[from_json][dynamic][enum]") {
    auto t = type_def("EnumStruct_fInt")
        .field<HttpMethod>("method")
        .field<std::string>("name");

    auto j = json{{"method", 2}, {"name", "update-dog"}};
    auto obj = t.create(j);

    REQUIRE(obj.get<HttpMethod>("method") == HttpMethod::PUT);
}

TEST_CASE("typed from_json: enum field default when key missing", "[from_json][typed][enum]") {
    auto j = json{{"name", "test"}};
    auto es = from_json<EnumStruct>(j);

    REQUIRE(es.method.value == HttpMethod::GET);
}

TEST_CASE("dynamic from_json: enum field default when key missing", "[from_json][dynamic][enum]") {
    auto t = type_def("EnumStruct_fDefMiss")
        .field<HttpMethod>("method")
        .field<std::string>("name");

    auto j = json{{"name", "test"}};
    auto obj = t.create(j);

    REQUIRE(obj.get<HttpMethod>("method") == HttpMethod::GET);
}

// ── Enum round-trips ────────────────────────────────────────────────────

TEST_CASE("typed: enum round-trip", "[json][typed][enum][roundtrip]") {
    EnumStruct original;
    original.method = HttpMethod::DELETE_METHOD;
    original.name = "remove-dog";

    auto j = to_json(original);
    REQUIRE(j["method"] == "DELETE_METHOD");

    auto restored = from_json<EnumStruct>(j);

    REQUIRE(restored.method.value == original.method.value);
    REQUIRE(restored.name.value == original.name.value);
}

TEST_CASE("dynamic: enum round-trip", "[json][dynamic][enum][roundtrip]") {
    auto t = type_def("EnumStruct_fRT")
        .field<HttpMethod>("method")
        .field<std::string>("name");
    auto obj = t.create();
    obj.set("method", HttpMethod::DELETE_METHOD);
    obj.set("name", std::string("remove-dog"));

    auto j = obj.to_json();
    REQUIRE(j["method"] == "DELETE_METHOD");

    auto obj2 = t.create(j);

    REQUIRE(obj2.get<HttpMethod>("method") == HttpMethod::DELETE_METHOD);
    REQUIRE(obj2.get<std::string>("name") == "remove-dog");
}

TEST_CASE("typed: multiple enum fields round-trip", "[json][typed][enum][roundtrip]") {
    MultiEnumStruct original;
    original.color = Color::blue;
    original.method = HttpMethod::POST;
    original.label = "fancy";

    auto j = to_json(original);

    REQUIRE(j["color"] == "blue");
    REQUIRE(j["method"] == "POST");
    REQUIRE(j["label"] == "fancy");

    auto restored = from_json<MultiEnumStruct>(j);

    REQUIRE(restored.color.value == Color::blue);
    REQUIRE(restored.method.value == HttpMethod::POST);
    REQUIRE(restored.label.value == "fancy");
}

TEST_CASE("dynamic: multiple enum fields round-trip", "[json][dynamic][enum][roundtrip]") {
    auto t = type_def("MultiEnumStruct_fRT")
        .field<Color>("color")
        .field<HttpMethod>("method")
        .field<std::string>("label");
    auto obj = t.create();
    obj.set("color", Color::blue);
    obj.set("method", HttpMethod::POST);
    obj.set("label", std::string("fancy"));

    auto j = obj.to_json();

    REQUIRE(j["color"] == "blue");
    REQUIRE(j["method"] == "POST");
    REQUIRE(j["label"] == "fancy");

    auto obj2 = t.create(j);

    REQUIRE(obj2.get<Color>("color") == Color::blue);
    REQUIRE(obj2.get<HttpMethod>("method") == HttpMethod::POST);
    REQUIRE(obj2.get<std::string>("label") == "fancy");
}

// ── Enum error cases ────────────────────────────────────────────────────

TEST_CASE("typed from_json: enum field throws on unknown string", "[from_json][typed][enum][throw]") {
    auto j = json{{"method", "PATCH"}, {"name", "test"}};
    REQUIRE_THROWS_AS(from_json<EnumStruct>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: enum field throws on unknown string", "[from_json][dynamic][enum][throw]") {
    auto t = type_def("EnumStruct_fThUnk")
        .field<HttpMethod>("method")
        .field<std::string>("name");

    auto j = json{{"method", "PATCH"}, {"name", "test"}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

TEST_CASE("typed from_json: enum field throws on bool value", "[from_json][typed][enum][throw]") {
    auto j = json{{"method", true}, {"name", "test"}};
    REQUIRE_THROWS_AS(from_json<EnumStruct>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: enum field throws on bool value", "[from_json][dynamic][enum][throw]") {
    auto t = type_def("EnumStruct_fThBool")
        .field<HttpMethod>("method")
        .field<std::string>("name");

    auto j = json{{"method", true}, {"name", "test"}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

TEST_CASE("typed from_json: enum field throws on object value", "[from_json][typed][enum][throw]") {
    auto j = json{{"method", json::object()}, {"name", "test"}};
    REQUIRE_THROWS_AS(from_json<EnumStruct>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: enum field throws on object value", "[from_json][dynamic][enum][throw]") {
    auto t = type_def("EnumStruct_fThObj")
        .field<HttpMethod>("method")
        .field<std::string>("name");

    auto j = json{{"method", json::object()}, {"name", "test"}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

TEST_CASE("typed from_json: enum field throws on array value", "[from_json][typed][enum][throw]") {
    auto j = json{{"method", json::array()}, {"name", "test"}};
    REQUIRE_THROWS_AS(from_json<EnumStruct>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: enum field throws on array value", "[from_json][dynamic][enum][throw]") {
    auto t = type_def("EnumStruct_fThArr")
        .field<HttpMethod>("method")
        .field<std::string>("name");

    auto j = json{{"method", json::array()}, {"name", "test"}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

// ═════════════════════════════════════════════════════════════════════════
// Enums inside collections — from_json + round-trip
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: vector of enums", "[from_json][typed][enum][collection]") {
    auto j = json{{"methods", {"GET", "POST", "DELETE_METHOD"}}};
    auto ve = from_json<VecOfEnums>(j);

    REQUIRE(ve.methods.value.size() == 3);
    REQUIRE(ve.methods.value[0] == HttpMethod::GET);
    REQUIRE(ve.methods.value[1] == HttpMethod::POST);
    REQUIRE(ve.methods.value[2] == HttpMethod::DELETE_METHOD);
}

TEST_CASE("dynamic from_json: vector of enums", "[from_json][dynamic][enum][collection]") {
    auto t = type_def("VecOfEnums_f")
        .field<std::vector<HttpMethod>>("methods");

    auto j = json{{"methods", {"GET", "POST", "DELETE_METHOD"}}};
    auto obj = t.create(j);

    auto methods = obj.get<std::vector<HttpMethod>>("methods");
    REQUIRE(methods.size() == 3);
    REQUIRE(methods[0] == HttpMethod::GET);
    REQUIRE(methods[1] == HttpMethod::POST);
    REQUIRE(methods[2] == HttpMethod::DELETE_METHOD);
}

TEST_CASE("typed from_json: vector of enums with integer values", "[from_json][typed][enum][collection]") {
    auto j = json{{"methods", {0, 1, 2}}};
    auto ve = from_json<VecOfEnums>(j);

    REQUIRE(ve.methods.value.size() == 3);
    REQUIRE(ve.methods.value[0] == HttpMethod::GET);
    REQUIRE(ve.methods.value[1] == HttpMethod::POST);
    REQUIRE(ve.methods.value[2] == HttpMethod::PUT);
}

TEST_CASE("dynamic from_json: vector of enums with integer values", "[from_json][dynamic][enum][collection]") {
    auto t = type_def("VecOfEnums_fInt")
        .field<std::vector<HttpMethod>>("methods");

    auto j = json{{"methods", {0, 1, 2}}};
    auto obj = t.create(j);

    auto methods = obj.get<std::vector<HttpMethod>>("methods");
    REQUIRE(methods.size() == 3);
    REQUIRE(methods[0] == HttpMethod::GET);
    REQUIRE(methods[1] == HttpMethod::POST);
    REQUIRE(methods[2] == HttpMethod::PUT);
}

TEST_CASE("typed round-trip: vector of enums", "[json][typed][enum][collection][roundtrip]") {
    VecOfEnums original;
    original.methods.value = {HttpMethod::DELETE_METHOD, HttpMethod::GET};

    auto restored = from_json<VecOfEnums>(to_json(original));

    REQUIRE(restored.methods.value.size() == 2);
    REQUIRE(restored.methods.value[0] == HttpMethod::DELETE_METHOD);
    REQUIRE(restored.methods.value[1] == HttpMethod::GET);
}

TEST_CASE("dynamic round-trip: vector of enums", "[json][dynamic][enum][collection][roundtrip]") {
    auto t = type_def("VecOfEnums_fRT")
        .field<std::vector<HttpMethod>>("methods");
    auto obj = t.create();
    obj.set("methods", std::vector<HttpMethod>{HttpMethod::DELETE_METHOD, HttpMethod::GET});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto methods = obj2.get<std::vector<HttpMethod>>("methods");
    REQUIRE(methods.size() == 2);
    REQUIRE(methods[0] == HttpMethod::DELETE_METHOD);
    REQUIRE(methods[1] == HttpMethod::GET);
}

TEST_CASE("typed from_json: map of enums", "[from_json][typed][enum][collection]") {
    auto j = json{{"palette", {{"sky", "blue"}, {"fire", "red"}}}};
    auto me = from_json<MapOfEnums>(j);

    REQUIRE(me.palette.value.at("sky") == Color::blue);
    REQUIRE(me.palette.value.at("fire") == Color::red);
}

TEST_CASE("dynamic from_json: map of enums", "[from_json][dynamic][enum][collection]") {
    auto t = type_def("MapOfEnums_f")
        .field<std::map<std::string, Color>>("palette");

    auto j = json{{"palette", {{"sky", "blue"}, {"fire", "red"}}}};
    auto obj = t.create(j);

    auto palette = obj.get<std::map<std::string, Color>>("palette");
    REQUIRE(palette.at("sky") == Color::blue);
    REQUIRE(palette.at("fire") == Color::red);
}

TEST_CASE("typed round-trip: map of enums", "[json][typed][enum][collection][roundtrip]") {
    MapOfEnums original;
    original.palette.value = {{"a", Color::red}, {"b", Color::green}, {"c", Color::blue}};

    auto restored = from_json<MapOfEnums>(to_json(original));

    REQUIRE(restored.palette.value.size() == 3);
    REQUIRE(restored.palette.value.at("a") == Color::red);
    REQUIRE(restored.palette.value.at("b") == Color::green);
    REQUIRE(restored.palette.value.at("c") == Color::blue);
}

TEST_CASE("dynamic round-trip: map of enums", "[json][dynamic][enum][collection][roundtrip]") {
    auto t = type_def("MapOfEnums_fRT")
        .field<std::map<std::string, Color>>("palette");
    auto obj = t.create();
    obj.set("palette", std::map<std::string, Color>{{"a", Color::red}, {"b", Color::green}, {"c", Color::blue}});

    auto j = obj.to_json();
    auto obj2 = t.create(j);

    auto palette = obj2.get<std::map<std::string, Color>>("palette");
    REQUIRE(palette.size() == 3);
    REQUIRE(palette.at("a") == Color::red);
    REQUIRE(palette.at("b") == Color::green);
    REQUIRE(palette.at("c") == Color::blue);
}

TEST_CASE("typed from_json: vector of enums throws on unknown string", "[from_json][typed][enum][collection][throw]") {
    auto j = json{{"methods", {"GET", "PATCH"}}};
    REQUIRE_THROWS_AS(from_json<VecOfEnums>(j), std::logic_error);
}

TEST_CASE("dynamic from_json: vector of enums throws on unknown string", "[from_json][dynamic][enum][collection][throw]") {
    auto t = type_def("VecOfEnums_fThUnk")
        .field<std::vector<HttpMethod>>("methods");

    auto j = json{{"methods", {"GET", "PATCH"}}};
    REQUIRE_THROWS_AS(t.create(j), std::logic_error);
}

TEST_CASE("typed from_json: empty vector of enums", "[from_json][typed][enum][collection]") {
    auto j = json{{"methods", json::array()}};
    auto ve = from_json<VecOfEnums>(j);

    REQUIRE(ve.methods.value.empty());
}

TEST_CASE("dynamic from_json: empty vector of enums", "[from_json][dynamic][enum][collection]") {
    auto t = type_def("VecOfEnums_fEmpty")
        .field<std::vector<HttpMethod>>("methods");

    auto j = json{{"methods", json::array()}};
    auto obj = t.create(j);

    REQUIRE(obj.get<std::vector<HttpMethod>>("methods").empty());
}

// ═════════════════════════════════════════════════════════════════════════
// Nested containers of structs — vector<vector<struct>>, map<string, vector<struct>>
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed from_json: vector<vector<Address>>", "[from_json][typed][nested_container]") {
    auto j = json{{"blocks", {
        {{{"street", "1st"}, {"zip", "10001"}}, {{"street", "2nd"}, {"zip", "10002"}}},
        {{{"street", "3rd"}, {"zip", "20001"}}}
    }}};
    auto vva = from_json<VecOfVecOfAddresses>(j);

    REQUIRE(vva.blocks.value.size() == 2);
    REQUIRE(vva.blocks.value[0].size() == 2);
    REQUIRE(vva.blocks.value[0][0].street.value == "1st");
    REQUIRE(vva.blocks.value[0][1].zip.value == "10002");
    REQUIRE(vva.blocks.value[1].size() == 1);
    REQUIRE(vva.blocks.value[1][0].street.value == "3rd");
}

TEST_CASE("dynamic from_json: vector<vector<Address>>", "[from_json][dynamic][nested_container]") {
    auto t = type_def("VVA_f")
        .field<std::vector<std::vector<Address>>>("blocks");

    auto j = json{{"blocks", {
        {{{"street", "1st"}, {"zip", "10001"}}, {{"street", "2nd"}, {"zip", "10002"}}},
        {{{"street", "3rd"}, {"zip", "20001"}}}
    }}};
    auto obj = t.create(j);

    auto blocks = obj.get<std::vector<std::vector<Address>>>("blocks");
    REQUIRE(blocks.size() == 2);
    REQUIRE(blocks[0].size() == 2);
    REQUIRE(blocks[0][0].street.value == "1st");
    REQUIRE(blocks[1][0].zip.value == "20001");
}

TEST_CASE("typed round-trip: vector<vector<Address>>", "[json][typed][nested_container][roundtrip]") {
    VecOfVecOfAddresses original;
    Address a1; a1.street = "Dog Ln"; a1.zip = "99999";
    Address a2; a2.street = "Cat St"; a2.zip = "88888";
    original.blocks.value = {{a1, a2}, {a1}};

    auto restored = from_json<VecOfVecOfAddresses>(to_json(original));

    REQUIRE(restored.blocks.value.size() == 2);
    REQUIRE(restored.blocks.value[0][1].street.value == "Cat St");
    REQUIRE(restored.blocks.value[1][0].zip.value == "99999");
}

TEST_CASE("typed from_json: map<string, vector<Address>>", "[from_json][typed][nested_container]") {
    auto j = json{{"regions", {
        {"west", {{{"street", "1st"}, {"zip", "90001"}}, {{"street", "2nd"}, {"zip", "90002"}}}},
        {"east", {{{"street", "3rd"}, {"zip", "10001"}}}}
    }}};
    auto mva = from_json<MapOfVecOfAddresses>(j);

    REQUIRE(mva.regions.value.size() == 2);
    REQUIRE(mva.regions.value.at("west").size() == 2);
    REQUIRE(mva.regions.value.at("west")[0].street.value == "1st");
    REQUIRE(mva.regions.value.at("east")[0].zip.value == "10001");
}

TEST_CASE("dynamic from_json: map<string, vector<Address>>", "[from_json][dynamic][nested_container]") {
    auto t = type_def("MVA_f")
        .field<std::map<std::string, std::vector<Address>>>("regions");

    auto j = json{{"regions", {
        {"west", {{{"street", "1st"}, {"zip", "90001"}}}},
        {"east", {{{"street", "3rd"}, {"zip", "10001"}}, {{"street", "4th"}, {"zip", "10002"}}}}
    }}};
    auto obj = t.create(j);

    auto regions = obj.get<std::map<std::string, std::vector<Address>>>("regions");
    REQUIRE(regions.size() == 2);
    REQUIRE(regions.at("west").size() == 1);
    REQUIRE(regions.at("east").size() == 2);
    REQUIRE(regions.at("east")[1].street.value == "4th");
}

TEST_CASE("typed round-trip: map<string, vector<Address>>", "[json][typed][nested_container][roundtrip]") {
    MapOfVecOfAddresses original;
    Address a1; a1.street = "Dog Ln"; a1.zip = "99999";
    original.regions.value = {{"home", {a1}}};

    auto restored = from_json<MapOfVecOfAddresses>(to_json(original));

    REQUIRE(restored.regions.value.size() == 1);
    REQUIRE(restored.regions.value.at("home")[0].street.value == "Dog Ln");
}

// ── Empty containers of structs ──────────────────────────────────────────

TEST_CASE("typed from_json: empty vector of structs", "[from_json][typed][nested_container][empty]") {
    auto j = json{{"org", "Empty Inc"}, {"people", json::array()}};
    auto pl = from_json<PersonList>(j);

    REQUIRE(pl.org.value == "Empty Inc");
    REQUIRE(pl.people.value.empty());
}

TEST_CASE("dynamic from_json: empty vector of structs", "[from_json][dynamic][nested_container][empty]") {
    auto t = type_def("PL_empty_f")
        .field<std::string>("org")
        .field<std::vector<Person>>("people");

    auto obj = t.create(json{{"org", "Empty Inc"}, {"people", json::array()}});

    REQUIRE(obj.get<std::string>("org") == "Empty Inc");
    REQUIRE(obj.get<std::vector<Person>>("people").empty());
}

TEST_CASE("typed from_json: empty map of structs", "[from_json][typed][nested_container][empty]") {
    auto j = json{{"locations", json::object()}};
    auto dm = from_json<DenseMapOfStructs>(j);

    REQUIRE(dm.locations.value.empty());
}

TEST_CASE("dynamic from_json: empty map of structs", "[from_json][dynamic][nested_container][empty]") {
    auto t = type_def("DM_empty_f")
        .field<std::map<std::string, Address>>("locations");

    auto obj = t.create(json{{"locations", json::object()}});

    REQUIRE(obj.get<std::map<std::string, Address>>("locations").empty());
}

// ═════════════════════════════════════════════════════════════════════════
// Hybrid from_json — enums and complex collections
// ═════════════════════════════════════════════════════════════════════════

struct FieldEnumDog {
    field<std::string>  name;
    field<HttpMethod>   method;
    field<Color>        color;
};

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<FieldEnumDog>() {
    return def_type::field_info<FieldEnumDog>("name", "method", "color");
}
#endif

TEST_CASE("hybrid from_json: enum fields deserialize from strings", "[from_json][hybrid][enum]") {
    // Round-trip to get platform-correct enum strings
    FieldEnumDog original;
    original.name = "Rex";
    original.method = HttpMethod::POST;
    original.color = Color::blue;

    auto j = to_json(original);
    auto restored = from_json<FieldEnumDog>(j);

    REQUIRE(restored.name.value == "Rex");
    REQUIRE(restored.method.value == HttpMethod::POST);
    REQUIRE(restored.color.value == Color::blue);
}

TEST_CASE("hybrid from_json: multiple enum fields round-trip", "[json][hybrid][enum][roundtrip]") {
    FieldEnumDog original;
    original.name = "Buddy";
    original.method = HttpMethod::PUT;
    original.color = Color::green;

    auto j = to_json(original);
    auto restored = from_json<FieldEnumDog>(j);

    REQUIRE(restored.name.value == "Buddy");
    REQUIRE(restored.method.value == HttpMethod::PUT);
    REQUIRE(restored.color.value == Color::green);
}

struct FieldVecOfStructs {
    field<std::string>          org;
    field<std::vector<Address>> addresses;
};

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<FieldVecOfStructs>() {
    return def_type::field_info<FieldVecOfStructs>("org", "addresses");
}
#endif

TEST_CASE("hybrid from_json: vector of structs", "[from_json][hybrid][collection]") {
    auto j = json{
        {"org", "Collab"},
        {"addresses", {
            {{"street", "1st St"}, {"zip", "10001"}},
            {{"street", "2nd Ave"}, {"zip", "20002"}}
        }}
    };
    auto result = from_json<FieldVecOfStructs>(j);

    REQUIRE(result.org.value == "Collab");
    REQUIRE(result.addresses.value.size() == 2);
    REQUIRE(result.addresses.value[0].street.value == "1st St");
    REQUIRE(result.addresses.value[1].zip.value == "20002");
}

TEST_CASE("hybrid round-trip: vector of structs", "[json][hybrid][collection][roundtrip]") {
    FieldVecOfStructs original;
    original.org = "Pirates";
    Address a; a.street = "Dog Ln"; a.zip = "99999";
    original.addresses.value = {a};

    auto j = to_json(original);
    auto restored = from_json<FieldVecOfStructs>(j);

    REQUIRE(restored.addresses.value.size() == 1);
    REQUIRE(restored.addresses.value[0].street.value == "Dog Ln");
}

struct FieldMapOfStructs {
    field<std::string>                       label;
    field<std::map<std::string, Address>>    locations;
};

#ifndef DEF_TYPE_HAS_PFR
template <>
constexpr auto def_type::struct_info<FieldMapOfStructs>() {
    return def_type::field_info<FieldMapOfStructs>("label", "locations");
}
#endif

TEST_CASE("hybrid from_json: map of structs", "[from_json][hybrid][collection]") {
    auto j = json{
        {"label", "offices"},
        {"locations", {
            {"hq", {{"street", "100 Main"}, {"zip", "97201"}}},
            {"branch", {{"street", "200 Oak"}, {"zip", "97202"}}}
        }}
    };
    auto result = from_json<FieldMapOfStructs>(j);

    REQUIRE(result.label.value == "offices");
    REQUIRE(result.locations.value.size() == 2);
    REQUIRE(result.locations.value.at("hq").street.value == "100 Main");
    REQUIRE(result.locations.value.at("branch").zip.value == "97202");
}

TEST_CASE("hybrid round-trip: map of structs", "[json][hybrid][collection][roundtrip]") {
    FieldMapOfStructs original;
    original.label = "test";
    Address a; a.street = "Cat St"; a.zip = "88888";
    original.locations.value = {{"home", a}};

    auto j = to_json(original);
    auto restored = from_json<FieldMapOfStructs>(j);

    REQUIRE(restored.locations.value.size() == 1);
    REQUIRE(restored.locations.value.at("home").zip.value == "88888");
}

// ═════════════════════════════════════════════════════════════════════════
// Typed/hybrid round-trip
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("typed/hybrid to_json + from_json round-trip", "[json][hybrid][roundtrip]") {
    SimpleArgs original;
    original.name = "Bob";
    original.age = 25;
    original.active = false;

    auto j = to_json(original);
    auto restored = from_json<SimpleArgs>(j);

    REQUIRE(restored.name.value == "Bob");
    REQUIRE(restored.age.value == 25);
    REQUIRE(restored.active.value == false);
}

// ═════════════════════════════════════════════════════════════════════════
// Dynamic round-trip: to_json -> load_json (existing tests)
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("dynamic round-trip: to_json then load_json", "[json][dynamic][roundtrip]") {
    auto t = type_def("Event_rt1")
        .field<std::string>("title")
        .field<int>("count")
        .field<bool>("active");
    auto obj = t.create();

    obj.set("title", std::string("Dog Party"));
    obj.set("count", 50);
    obj.set("active", true);

    auto j = obj.to_json();

    auto obj2 = t.create();
    obj2.load_json(j);

    REQUIRE(obj2.get<std::string>("title") == "Dog Party");
    REQUIRE(obj2.get<int>("count") == 50);
    REQUIRE(obj2.get<bool>("active") == true);
}

TEST_CASE("dynamic round-trip: create(json) then to_json", "[json][dynamic][roundtrip]") {
    auto t = type_def("Event_rt2")
        .field<std::string>("title")
        .field<int>("count", 100);

    auto original = json{{"title", "Party"}, {"count", 50}};
    auto obj = t.create(original);
    auto roundtripped = obj.to_json();

    REQUIRE(roundtripped["title"] == "Party");
    REQUIRE(roundtripped["count"] == 50);
}

TEST_CASE("dynamic round-trip: partial load preserves untouched fields in to_json", "[json][dynamic][roundtrip]") {
    auto t = type_def("Event_rt3")
        .field<std::string>("title", std::string("Default"))
        .field<int>("count", 100)
        .field<bool>("active", false);
    auto obj = t.create();

    obj.load_json(json{{"title", "Custom"}});

    auto j = obj.to_json();

    REQUIRE(j["title"] == "Custom");
    REQUIRE(j["count"] == 100);
    REQUIRE(j["active"] == false);
}

// ═════════════════════════════════════════════════════════════════════════
// Hybrid from_json
// ═════════════════════════════════════════════════════════════════════════

TEST_CASE("hybrid from_json: struct with metas — metas untouched", "[from_json][hybrid][meta]") {
    auto j = json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}};
    auto d = from_json<HJsonDog>(j);

    REQUIRE(d.name.value == "Rex");
    REQUIRE(d.age.value == 3);
    REQUIRE(d.breed.value == "Husky");
    REQUIRE(std::string_view{d.endpoint->path} == "/dogs");
    REQUIRE(std::string_view{d.endpoint->method} == "POST");
    REQUIRE(std::string_view{d.help->summary} == "A good boy");
}

TEST_CASE("hybrid from_json: missing keys preserve defaults, metas untouched", "[from_json][hybrid][meta]") {
    auto j = json{{"name", "Rex"}};
    auto d = from_json<HJsonDog>(j);

    REQUIRE(d.name.value == "Rex");
    REQUIRE(d.age.value == 0);
    REQUIRE(d.breed.value.empty());
    REQUIRE(std::string_view{d.endpoint->path} == "/dogs");
}

TEST_CASE("hybrid from_json: extra keys ignored, metas untouched", "[from_json][hybrid][meta]") {
    auto j = json{{"name", "Rex"}, {"endpoint", "junk"}, {"help", 42}};
    auto d = from_json<HJsonDog>(j);

    REQUIRE(d.name.value == "Rex");
    REQUIRE(std::string_view{d.endpoint->path} == "/dogs");
}

TEST_CASE("hybrid from_json: empty JSON gives defaults, metas untouched", "[from_json][hybrid][meta]") {
    auto d = from_json<HJsonDog>(json::object());

    REQUIRE(d.name.value.empty());
    REQUIRE(d.age.value == 0);
    REQUIRE(std::string_view{d.endpoint->path} == "/dogs");
}

TEST_CASE("hybrid from_json: throws on non-object", "[from_json][hybrid][throw]") {
    REQUIRE_THROWS_AS(from_json<HJsonDog>(json(42)), std::logic_error);
    REQUIRE_THROWS_AS(from_json<HJsonDog>(json::array()), std::logic_error);
}

TEST_CASE("hybrid from_json: throws on type mismatch", "[from_json][hybrid][throw]") {
    auto j = json{{"name", 42}};
    REQUIRE_THROWS_AS(from_json<HJsonDog>(j), std::logic_error);
}

TEST_CASE("hybrid round-trip: to_json then from_json preserves fields", "[json][hybrid][roundtrip]") {
    HJsonDog original;
    original.name = "Buddy";
    original.age = 5;
    original.breed = "Golden";

    auto j = to_json(original);
    auto restored = from_json<HJsonDog>(j);

    REQUIRE(restored.name.value == "Buddy");
    REQUIRE(restored.age.value == 5);
    REQUIRE(restored.breed.value == "Golden");
    REQUIRE(std::string_view{restored.endpoint->path} == "/dogs");
}

TEST_CASE("hybrid from_json: from JSON string", "[from_json][hybrid][string]") {
    auto d = from_json<HJsonDog>(std::string(R"({"name":"Rex","age":3,"breed":"Husky"})"));

    REQUIRE(d.name.value == "Rex");
    REQUIRE(d.age.value == 3);
}
