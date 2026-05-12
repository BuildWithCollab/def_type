#include <catch2/catch_all.hpp>

#include <cstdint>
#include <optional>
#include <string>

#include <def_type.hpp>
#include <nlohmann/json.hpp>

using namespace def_type;

namespace test_unknown {

    // Two totally different shapes — disjoint field sets so shape-only
    // `oneof<A, B>` can pick the right one without a discriminator.

    struct DogParams {
        std::string breed;
        int         tail_wags;
    };

    struct WeatherForecast {
        double latitude;
        double longitude;
        int    days_ahead;
    };

    using Params = oneof<DogParams, WeatherForecast>;

    struct JsonRpcEnvelope {
        std::string jsonrpc;
        int64_t     id;
        std::string method;
        Params      params = DogParams{};
    };

}  // namespace test_unknown

TEST_CASE("envelope with oneof params: dog shape", "[unknown][oneof][envelope]") {
    using namespace test_unknown;

    auto envelope = from_json_string<JsonRpcEnvelope>(R"({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "generic-must-not-be-able-to-know-params-type-from-this",
        "params": { "breed": "Husky", "tail_wags": 42 }
    })");

    REQUIRE(envelope.jsonrpc == "2.0");
    REQUIRE(envelope.id == 1);
    REQUIRE(envelope.method == "generic-must-not-be-able-to-know-params-type-from-this");

    REQUIRE(envelope.params.is<DogParams>());
    REQUIRE_FALSE(envelope.params.is<WeatherForecast>());

    auto* dog = envelope.params.as<DogParams>();
    REQUIRE(dog != nullptr);
    REQUIRE(dog->breed == "Husky");
    REQUIRE(dog->tail_wags == 42);
}

TEST_CASE("envelope with oneof params: weather shape", "[unknown][oneof][envelope]") {
    using namespace test_unknown;

    auto envelope = from_json_string<JsonRpcEnvelope>(R"({
        "jsonrpc": "2.0",
        "id": 2,
        "method": "literally-anything",
        "params": { "latitude": 47.6, "longitude": -122.3, "days_ahead": 7 }
    })");

    REQUIRE(envelope.jsonrpc == "2.0");
    REQUIRE(envelope.id == 2);
    REQUIRE(envelope.method == "literally-anything");

    REQUIRE(envelope.params.is<WeatherForecast>());
    REQUIRE_FALSE(envelope.params.is<DogParams>());

    auto* forecast = envelope.params.as<WeatherForecast>();
    REQUIRE(forecast != nullptr);
    REQUIRE(forecast->latitude == Catch::Approx(47.6));
    REQUIRE(forecast->longitude == Catch::Approx(-122.3));
    REQUIRE(forecast->days_ahead == 7);
}

TEST_CASE("envelope with oneof params: match() reads through the variant", "[unknown][oneof][envelope]") {
    using namespace test_unknown;

    auto envelope = from_json_string<JsonRpcEnvelope>(R"({
        "jsonrpc": "2.0",
        "id": 3,
        "method": "still-opaque",
        "params": { "breed": "Corgi", "tail_wags": 999 }
    })");

    auto summary = envelope.params.match(
        [](const DogParams& d)        { return std::string("dog:") + d.breed; },
        [](const WeatherForecast& w)  { (void)w; return std::string("weather"); }
    );

    REQUIRE(summary == "dog:Corgi");
}

// ═════════════════════════════════════════════════════════════════════════
// def_type::unknown — type-erased JSON value with typed probing
// ═════════════════════════════════════════════════════════════════════════

namespace test_unknown_type {

    struct Dog {
        std::string name;
        int         age;
        std::string breed;
    };

    struct DogBreed {
        std::string name;
        std::string origin;
    };

    struct DogWithBreed {
        std::string name;
        int         age;
        DogBreed    breed;
    };

    struct Cat {
        std::string name;
        bool        indoor;
    };

    struct help_meta { const char* summary = ""; };

    struct must_parse_as_dog {
        std::vector<validation_error> operator()(const unknown& u) const {
            if (u.is<Dog>()) return {};
            return { { .message = "params do not match Dog shape" } };
        }
    };

    struct ReqWithMeta {
        std::string method;
        field<unknown, with<help_meta>> params {
            .with = {{.summary = "the params blob"}}
        };
    };

    struct ReqWithValidator {
        std::string method;
        field<unknown> params {
            .validators = validators(must_parse_as_dog{})
        };
    };

}  // namespace test_unknown_type

TEST_CASE("unknown: construct from nlohmann::json, raw() round-trips", "[unknown][type]") {
    nlohmann::json j = {{"a", 1}, {"b", "two"}};
    unknown u(j);
    REQUIRE(u.raw() == j);
}

TEST_CASE("unknown: construct from typed value serializes via to_json", "[unknown][type]") {
    using namespace test_unknown_type;
    Dog rex{ "Rex", 3, "Husky" };
    unknown u(rex);

    REQUIRE(u.raw().is_object());
    REQUIRE(u.raw().at("name") == "Rex");
    REQUIRE(u.raw().at("age") == 3);
    REQUIRE(u.raw().at("breed") == "Husky");
}

TEST_CASE("unknown: assign from typed value re-serializes", "[unknown][type]") {
    using namespace test_unknown_type;
    unknown u;
    Dog rex{ "Rex", 3, "Husky" };
    u = rex;

    REQUIRE(u.raw().at("name") == "Rex");
    REQUIRE(u.raw().at("breed") == "Husky");

    Cat whiskers{ "Whiskers", true };
    u = whiskers;
    REQUIRE_FALSE(u.raw().contains("breed"));
    REQUIRE(u.raw().at("indoor") == true);
}

TEST_CASE("unknown: is<T>() true when JSON matches the shape", "[unknown][type]") {
    using namespace test_unknown_type;
    unknown u(nlohmann::json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}});

    REQUIRE(u.is<Dog>());
}

TEST_CASE("unknown: is<T>() false when JSON does not match", "[unknown][type]") {
    using namespace test_unknown_type;
    unknown u(nlohmann::json{{"name", "Whiskers"}, {"indoor", true}});

    REQUIRE_FALSE(u.is<Dog>());
    REQUIRE(u.is<Cat>());
}

TEST_CASE("unknown: as<T>() returns deserialized value", "[unknown][type]") {
    using namespace test_unknown_type;
    unknown u(nlohmann::json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}});

    Dog d = u.as<Dog>();
    REQUIRE(d.name == "Rex");
    REQUIRE(d.age == 3);
    REQUIRE(d.breed == "Husky");
}

TEST_CASE("unknown: as<T>() throws when JSON does not match", "[unknown][type]") {
    using namespace test_unknown_type;
    unknown u(nlohmann::json{{"name", "Rex"}, {"age", "not-a-number"}});

    REQUIRE_THROWS_AS(u.as<Dog>(), std::logic_error);
}

TEST_CASE("unknown: try_as<T>() returns optional", "[unknown][type]") {
    using namespace test_unknown_type;
    unknown good(nlohmann::json{{"name", "Rex"}, {"age", 3}, {"breed", "Husky"}});
    unknown bad (nlohmann::json{{"name", "Rex"}, {"age", "nope"}});

    auto ok  = good.try_as<Dog>();
    auto err = bad.try_as<Dog>();

    REQUIRE(ok.has_value());
    REQUIRE(ok->name == "Rex");
    REQUIRE_FALSE(err.has_value());
}

TEST_CASE("unknown: get<T>(path) drills into nested JSON via dotted path", "[unknown][type][path]") {
    using namespace test_unknown_type;
    unknown u(nlohmann::json{
        {"name", "Rover"},
        {"age", 12},
        {"breed", {{"name", "Golden Retriever"}, {"origin", "Scotland"}}}
    });

    REQUIRE(u.get<std::string>("name") == "Rover");
    REQUIRE(u.get<int>("age") == 12);
    REQUIRE(u.get<std::string>("breed.name") == "Golden Retriever");
    REQUIRE(u.get<std::string>("breed.origin") == "Scotland");
}

TEST_CASE("unknown: get<DogBreed>(path) deserializes a struct from nested path", "[unknown][type][path]") {
    using namespace test_unknown_type;
    unknown u(nlohmann::json{
        {"name", "Rover"},
        {"breed", {{"name", "Golden Retriever"}, {"origin", "Scotland"}}}
    });

    DogBreed b = u.get<DogBreed>("breed");
    REQUIRE(b.name == "Golden Retriever");
    REQUIRE(b.origin == "Scotland");
}

TEST_CASE("unknown: leading slash treated as JSON Pointer", "[unknown][type][path]") {
    using namespace test_unknown_type;
    unknown u(nlohmann::json{
        {"breed", {{"name", "Golden Retriever"}}}
    });

    REQUIRE(u.get<std::string>("/breed/name") == "Golden Retriever");
    REQUIRE(u.get<std::string>("breed.name")  == "Golden Retriever");
}

TEST_CASE("unknown: JSON Pointer handles array indexing", "[unknown][type][path]") {
    unknown u(nlohmann::json{
        {"items", {"first", "second", "third"}}
    });
    REQUIRE(u.get<std::string>("/items/0") == "first");
    REQUIRE(u.get<std::string>("/items/2") == "third");
}

TEST_CASE("unknown: is<T>(path) probes nested location", "[unknown][type][path]") {
    using namespace test_unknown_type;
    unknown u(nlohmann::json{
        {"breed", {{"name", "Golden Retriever"}}}
    });

    REQUIRE(u.is<std::string>("breed.name"));
    REQUIRE_FALSE(u.is<int>("breed.name"));
    REQUIRE_FALSE(u.is<std::string>("breed.nonexistent"));
}

TEST_CASE("unknown: try_get<T>(path) returns nullopt for bad path", "[unknown][type][path]") {
    using namespace test_unknown_type;
    unknown u(nlohmann::json{
        {"breed", {{"name", "Golden Retriever"}}}
    });

    REQUIRE(u.try_get<std::string>("breed.name").has_value());
    REQUIRE(*u.try_get<std::string>("breed.name") == "Golden Retriever");
    REQUIRE_FALSE(u.try_get<std::string>("breed.nope").has_value());
    REQUIRE_FALSE(u.try_get<std::string>("missing.path").has_value());
}

TEST_CASE("unknown: get<T>(bad path) throws", "[unknown][type][path]") {
    using namespace test_unknown_type;
    unknown u(nlohmann::json{{"a", 1}});
    REQUIRE_THROWS(u.get<int>("does.not.exist"));
}

namespace test_unknown_envelope {

    struct JsonRpcRequest {
        std::string jsonrpc;
        int64_t     id;
        std::string method;
        unknown     params;
    };

}  // namespace test_unknown_envelope

TEST_CASE("unknown: parses unknown params from a JSON-RPC envelope", "[unknown][type][envelope]") {
    using namespace test_unknown_envelope;
    using namespace test_unknown_type;

    auto req = from_json_string<JsonRpcRequest>(R"({
        "jsonrpc": "2.0",
        "id": 7,
        "method": "describe-dog",
        "params": { "name": "Rover", "age": 12, "breed": "Golden Retriever" }
    })");

    REQUIRE(req.jsonrpc == "2.0");
    REQUIRE(req.id == 7);
    REQUIRE(req.method == "describe-dog");

    REQUIRE(req.params.is<Dog>());
    Dog d = req.params.as<Dog>();
    REQUIRE(d.name == "Rover");
    REQUIRE(d.age == 12);
    REQUIRE(d.breed == "Golden Retriever");

    REQUIRE(req.params.get<std::string>("name") == "Rover");
    REQUIRE(req.params.get<int>("age") == 12);
}

TEST_CASE("unknown: drills into nested params via path on an envelope", "[unknown][type][envelope][path]") {
    using namespace test_unknown_envelope;
    using namespace test_unknown_type;

    auto req = from_json_string<JsonRpcRequest>(R"({
        "jsonrpc": "2.0",
        "id": 8,
        "method": "describe-dog-deep",
        "params": {
            "name": "Rover",
            "age": 12,
            "breed": { "name": "Golden Retriever", "origin": "Scotland" }
        }
    })");

    REQUIRE(req.params.get<std::string>("name") == "Rover");
    REQUIRE(req.params.get<std::string>("breed.name") == "Golden Retriever");
    REQUIRE(req.params.get<DogBreed>("breed").origin == "Scotland");
}

TEST_CASE("unknown: round-trips through envelope to_json", "[unknown][type][envelope][roundtrip]") {
    using namespace test_unknown_envelope;

    nlohmann::json original = {
        {"jsonrpc", "2.0"},
        {"id", 9},
        {"method", "echo"},
        {"params", {{"name", "Rover"}, {"age", 12}, {"breed", "Golden Retriever"}}}
    };

    auto req   = from_json<JsonRpcRequest>(original);
    auto round = to_json(req);

    REQUIRE(round == original);
}

TEST_CASE("unknown: envelope built in C++ with typed params serializes correctly", "[unknown][type][envelope][build]") {
    using namespace test_unknown_envelope;
    using namespace test_unknown_type;

    JsonRpcRequest req{
        .jsonrpc = "2.0",
        .id      = 10,
        .method  = "create-dog",
        .params  = Dog{ "Rex", 3, "Husky" }
    };

    auto j = to_json(req);
    REQUIRE(j["params"]["name"] == "Rex");
    REQUIRE(j["params"]["age"] == 3);
    REQUIRE(j["params"]["breed"] == "Husky");
}

TEST_CASE("unknown: field<unknown, with<...>> attaches metadata", "[unknown][type][meta]") {
    using namespace test_unknown_type;

    type_def<ReqWithMeta> t;
    REQUIRE(t.field("params").has_meta<help_meta>());
    REQUIRE(std::string(t.field("params").meta<help_meta>().summary) == "the params blob");
}

TEST_CASE("unknown: field<unknown> with a validator that probes via is<T>()", "[unknown][type][validation]") {
    using namespace test_unknown_type;

    ReqWithValidator req;
    req.method = "create-dog";
    req.params = Dog{ "Rex", 3, "Husky" };

    type_def<ReqWithValidator> t;
    REQUIRE(t.valid(req));

    req.params = Cat{ "Whiskers", true };
    REQUIRE_FALSE(t.valid(req));

    auto result = t.validate(req);
    REQUIRE(result.error_count() == 1);
    REQUIRE(result.errors()[0].path == "params");
    REQUIRE(result.errors()[0].validator == "must_parse_as_dog");
}
