#include "flock/core/config.hpp"
#include "flock/model_manager/model.hpp"
#include "flock/model_manager/providers/adapters/typesafe.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

namespace flock {

namespace {

// Replaces HTTP so the request and the answer mapping can be inspected.
class RecordingHandler : public IModelProviderHandler {
public:
    std::vector<nlohmann::json> requests;
    std::vector<nlohmann::json> canned_responses;

    void AddRequest(const nlohmann::json& json, RequestType type = RequestType::Completion) override {
        (void) type;
        requests.push_back(json);
    }
    std::vector<nlohmann::json> CollectCompletions(const std::string& = "application/json") override {
        return canned_responses;
    }
    std::vector<nlohmann::json> CollectEmbeddings(const std::string& = "application/json") override {
        return {};
    }
    std::vector<nlohmann::json> CollectTranscriptions(const std::string& = "multipart/form-data") override {
        return {};
    }
};

ModelDetails MakeModelDetails(const std::optional<double> threshold = 0.5) {
    ModelDetails details;
    details.provider_name = TYPESAFE;
    details.model_name = "jev";
    details.model = "jev-latest";
    details.secret = {{"api_key", "test-key"}};
    details.threshold = threshold;
    return details;
}

// A single text column. A null entry marks a row with no context at all.
nlohmann::json MakeTuples(const std::vector<nlohmann::json>& values) {
    auto data = nlohmann::json::array();
    for (const auto& value: values) {
        data.push_back(value);
    }
    return nlohmann::json::array({{{"name", "review"}, {"data", data}}});
}

RecordingHandler& InstallRecordingHandler(TypeSafeProvider& provider) {
    auto handler = std::make_unique<RecordingHandler>();
    auto& reference = *handler;
    provider.model_handler_ = std::move(handler);
    return reference;
}

nlohmann::json NoulAnswer(const double probability) {
    return {{"type", "noul"}, {"noul", probability}};
}

}// namespace

// Answers must land on their own rows, not shift past skipped ones.
TEST(TypeSafeProviderTest, ScattersAnswersPastSkippedRows) {
    auto provider = TypeSafeProvider(MakeModelDetails());
    auto& handler = InstallRecordingHandler(provider);

    auto tuples = MakeTuples({"battery dies in an hour", nullptr, "great screen", "dead by lunchtime"});
    const std::string predicate = "The review complains about battery life";
    provider.AddStructuredCompletionRequest({BatchContext(tuples), predicate, ScalarFunctionType::FILTER});

    handler.canned_responses = {{{"model", "jev-1.13.0"},
                                 {"answers",
                                  {{"0", NoulAnswer(0.97)}, {"2", NoulAnswer(0.04)}, {"3", NoulAnswer(0.91)}}},
                                 {"usage", {{"input_tokens", 83}, {"output_tokens", 6}}}}};

    const auto results = provider.CollectCompletions();
    ASSERT_EQ(results.size(), 1u);
    const auto& items = results[0]["items"];

    ASSERT_EQ(items.size(), 4u);
    EXPECT_TRUE(items[0].get<bool>());
    EXPECT_TRUE(items[1].is_null());// skipped, never asked about
    EXPECT_FALSE(items[2].get<bool>());
    EXPECT_TRUE(items[3].get<bool>());
}

TEST(TypeSafeProviderTest, CarriesThePredicateOnceInTheState) {
    auto provider = TypeSafeProvider(MakeModelDetails());
    auto& handler = InstallRecordingHandler(provider);

    auto tuples = MakeTuples({"a", nullptr, "c"});
    const std::string predicate = "The review complains about battery life";
    provider.AddStructuredCompletionRequest({BatchContext(tuples), predicate, ScalarFunctionType::FILTER});

    ASSERT_EQ(handler.requests.size(), 1u);
    const auto& payload = handler.requests[0];

    EXPECT_EQ(payload["model"], "jev-latest");
    EXPECT_EQ(payload["state"]["criterion"], predicate);

    // The null row contributes neither state nor a question.
    EXPECT_TRUE(payload["state"]["rows"].contains("0"));
    EXPECT_FALSE(payload["state"]["rows"].contains("1"));
    EXPECT_TRUE(payload["state"]["rows"].contains("2"));

    const auto& questions = payload["questions"];
    EXPECT_EQ(questions.size(), 2u);
    EXPECT_EQ(questions["0"]["type"], "noul");
    EXPECT_FALSE(questions.contains("1"));

    // The predicate is stated once, not repeated per question.
    for (const auto& question: questions) {
        EXPECT_EQ(question["instructions"].get<std::string>().find(predicate), std::string::npos);
    }
}

TEST(TypeSafeProviderTest, SendsNothingWhenEveryRowIsUnevaluated) {
    auto provider = TypeSafeProvider(MakeModelDetails());
    auto& handler = InstallRecordingHandler(provider);

    auto tuples = MakeTuples({nullptr, nullptr});
    provider.AddStructuredCompletionRequest({BatchContext(tuples), "anything", ScalarFunctionType::FILTER});

    // A System One request must carry at least one question.
    EXPECT_TRUE(handler.requests.empty());

    const auto results = provider.CollectCompletions();
    ASSERT_EQ(results.size(), 1u);
    const auto& items = results[0]["items"];
    ASSERT_EQ(items.size(), 2u);
    EXPECT_TRUE(items[0].is_null());
    EXPECT_TRUE(items[1].is_null());
}

TEST(TypeSafeProviderTest, TreatsTheNullStringAsMissing) {
    auto provider = TypeSafeProvider(MakeModelDetails());
    auto& handler = InstallRecordingHandler(provider);

    auto tuples = nlohmann::json::array({{{"data", {"battery dies fast", "NULL"}}},
                                         {{"name", "stars"}, {"data", {"NULL", "NULL"}}}});
    provider.AddStructuredCompletionRequest({BatchContext(tuples), "predicate", ScalarFunctionType::FILTER});

    ASSERT_EQ(handler.requests.size(), 1u);
    const auto& payload = handler.requests[0];
    EXPECT_TRUE(payload["questions"].contains("0"));
    EXPECT_FALSE(payload["questions"].contains("1"));
    EXPECT_EQ(payload["state"]["rows"]["0"]["COLUMN 1"], "battery dies fast");
    EXPECT_TRUE(payload["state"]["rows"]["0"]["stars"].is_null());

    handler.canned_responses = {{{"answers", {{"0", NoulAnswer(0.9)}}}}};
    const auto items = provider.CollectCompletions()[0]["items"];
    EXPECT_TRUE(items[0].get<bool>());
    EXPECT_TRUE(items[1].is_null());
}

TEST(TypeSafeProviderTest, ClassifiesEachRowWithOneChoiceQuestion) {
    auto provider = TypeSafeProvider(MakeModelDetails());
    auto& handler = InstallRecordingHandler(provider);

    StructuredCompletionRequest request{BatchContext(MakeTuples({"app crashes", nullptr, "add dark mode"})),
                                        "What kind of issue is this?", ScalarFunctionType::CLASSIFY};
    request.choices = {{"bug", nullptr}, {"feature", "A request for something new"}};
    provider.AddStructuredCompletionRequest(request);

    ASSERT_EQ(handler.requests.size(), 1u);
    const auto& payload = handler.requests[0];
    EXPECT_EQ(payload["state"]["task"], "What kind of issue is this?");
    EXPECT_EQ(payload["questions"]["0"]["type"], "choice");
    EXPECT_EQ(payload["questions"]["0"]["criteria"], request.choices);
    EXPECT_FALSE(payload["questions"].contains("1"));

    handler.canned_responses = {{{"answers",
                                  {{"0", {{"type", "choice"}, {"choice", "bug"}}},
                                   {"2", {{"type", "choice"}, {"choice", "feature"}}}}}}};
    const auto items = provider.CollectCompletions()[0]["items"];
    EXPECT_EQ(items[0]["choice"], "bug");
    EXPECT_TRUE(items[1].is_null());
    EXPECT_EQ(items[2]["choice"], "feature");
}

TEST(TypeSafeProviderTest, ClassifyRequiresAtLeastTwoLabels) {
    auto provider = TypeSafeProvider(MakeModelDetails());
    InstallRecordingHandler(provider);
    StructuredCompletionRequest request{BatchContext(MakeTuples({"a"})), "task", ScalarFunctionType::CLASSIFY};
    request.choices = {{"only", nullptr}};
    EXPECT_THROW(provider.AddStructuredCompletionRequest(request), std::runtime_error);
}

TEST(TypeSafeProviderTest, FilterRequiresAThreshold) {
    auto provider = TypeSafeProvider(MakeModelDetails(std::nullopt));
    InstallRecordingHandler(provider);
    EXPECT_THROW(provider.AddStructuredCompletionRequest(
                         {BatchContext(MakeTuples({"a"})), "predicate", ScalarFunctionType::FILTER}),
                 std::runtime_error);
}

TEST(TypeSafeProviderTest, AppliesTheConfiguredThreshold) {
    auto provider = TypeSafeProvider(MakeModelDetails(0.8));
    auto& handler = InstallRecordingHandler(provider);

    auto tuples = MakeTuples({"a", "b"});
    provider.AddStructuredCompletionRequest({BatchContext(tuples), "predicate", ScalarFunctionType::FILTER});

    handler.canned_responses = {
            {{"answers", {{"0", NoulAnswer(0.79)}, {"1", NoulAnswer(0.80)}}}}};

    const auto results = provider.CollectCompletions();
    const auto& items = results[0]["items"];
    EXPECT_FALSE(items[0].get<bool>());// below
    EXPECT_TRUE(items[1].get<bool>()); // exactly at the threshold counts as true
}

TEST(TypeSafeProviderTest, KeepsTheValuesOfUnnamedColumns) {
    auto provider = TypeSafeProvider(MakeModelDetails());
    auto& handler = InstallRecordingHandler(provider);

    auto tuples = nlohmann::json::array({{{"data", {"battery dies fast"}}},
                                         {{"name", "stars"}, {"data", {"1"}}},
                                         {{"data", {"returned it"}}}});
    provider.AddStructuredCompletionRequest({BatchContext(tuples), "predicate", ScalarFunctionType::FILTER});

    ASSERT_EQ(handler.requests.size(), 1u);
    const auto& row = handler.requests[0]["state"]["rows"]["0"];
    // Unnamed columns are numbered the way the prompt renderer numbers them.
    EXPECT_EQ(row["COLUMN 1"], "battery dies fast");
    EXPECT_EQ(row["stars"], "1");
    EXPECT_EQ(row["COLUMN 2"], "returned it");
    EXPECT_EQ(row.size(), 3u);
}

TEST(TypeSafeProviderTest, RefusesImageAndAudioColumns) {
    for (const auto* type: {"image", "audio"}) {
        auto provider = TypeSafeProvider(MakeModelDetails());
        auto& handler = InstallRecordingHandler(provider);
        auto tuples = nlohmann::json::array({{{"type", type}, {"data", {"/tmp/file"}}}});
        EXPECT_THROW(provider.AddStructuredCompletionRequest(
                             {BatchContext(tuples), "predicate", ScalarFunctionType::FILTER}),
                     std::runtime_error)
                << type;
        EXPECT_TRUE(handler.requests.empty()) << type;
    }
}

TEST(TypeSafeProviderTest, PassesTokenLimitMarkerThrough) {
    auto provider = TypeSafeProvider(MakeModelDetails());
    auto& handler = InstallRecordingHandler(provider);

    auto tuples = MakeTuples({"a", "b"});
    provider.AddStructuredCompletionRequest({BatchContext(tuples), "predicate", ScalarFunctionType::FILTER});

    handler.canned_responses = {TokenLimitExceededMarker()};

    const auto results = provider.CollectCompletions();
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(IsTokenLimitExceededMarker(results[0]));
}

namespace {

class ExposedTypeSafeHandler : public TypeSafeModelManager {
public:
    explicit ExposedTypeSafeHandler(const std::string& base_url = "")
        : TypeSafeModelManager("test-key", base_url, true) {}
    using TypeSafeModelManager::checkProviderSpecificResponse;
    using TypeSafeModelManager::getCompletionUrl;
};

}// namespace

// The bodies below are the ones the live API returned.
TEST(TypeSafeHandlerTest, HalvesOnlyWhenTheBatchIsTooLarge) {
    ExposedTypeSafeHandler handler;
    const auto oversized = nlohmann::json::parse(R"({"detail": {"error_type": "max_tokens_exceeded"}})");
    EXPECT_THROW(handler.checkProviderSpecificResponse(oversized, IModelProviderHandler::RequestType::Completion),
                 TokenLimitExceededError);
}

TEST(TypeSafeHandlerTest, ReportsOtherRejectionsInsteadOfHalving) {
    ExposedTypeSafeHandler handler;
    const auto invalid = nlohmann::json::parse(
            R"({"detail": [{"loc": ["body", "questions"], "msg": "Field required", "type": "missing"}]})");
    try {
        handler.checkProviderSpecificResponse(invalid, IModelProviderHandler::RequestType::Completion);
        FAIL() << "expected a rejection";
    } catch (const TokenLimitExceededError&) {
        FAIL() << "a validation failure must not trigger the halving retry";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("Field required"), std::string::npos) << e.what();
    }
}

TEST(TypeSafeHandlerTest, RecognisesTheOversizedRejectionThroughOpenRouter) {
    EXPECT_TRUE(IsTokenLimitErrorMessage(R"(HTTP 400: {"detail":{"error_type":"max_tokens_exceeded"}})"));
}

TEST(TypeSafeHandlerTest, AcceptsTheCommonFormsOfABaseUrl) {
    EXPECT_EQ(ExposedTypeSafeHandler().getCompletionUrl(), "https://api.typesafe.ai/v1/systemone");
    for (const auto* base: {"https://openrouter.ai/api", "https://openrouter.ai/api/", "https://openrouter.ai/api/v1",
                            "https://openrouter.ai/api/v1/", "https://openrouter.ai/api/v1/systemone"}) {
        EXPECT_EQ(ExposedTypeSafeHandler(base).getCompletionUrl(), "https://openrouter.ai/api/v1/systemone") << base;
    }
    EXPECT_EQ(ExposedTypeSafeHandler("http://127.0.0.1:18765").getCompletionUrl(),
              "http://127.0.0.1:18765/v1/systemone");
}

namespace {

// llm_filter on a typesafe model through SQL, with the provider replaced by a
// recorder.
class RecordingDecisionProvider : public IProvider {
public:
    static inline std::vector<std::optional<double>> seen_model_thresholds;
    static inline std::vector<nlohmann::json> seen_tuples;
    static inline std::vector<nlohmann::json> seen_choices;

    explicit RecordingDecisionProvider(const ModelDetails& details)
        : IProvider(details) {
        seen_model_thresholds.push_back(details.threshold);
    }

    void AddCompletionRequest(const std::string&, const int, OutputType,
                              const nlohmann::json&) override {
        throw std::runtime_error("expected a structured request");
    }
    void AddEmbeddingRequest(const std::vector<std::string>&) override {}
    void AddTranscriptionRequest(const nlohmann::json&) override {}

    bool AcceptsStructuredTuples() const override { return true; }
    void AddStructuredCompletionRequest(
            const StructuredCompletionRequest& request) override {
        seen_tuples.push_back(request.batch.Columns());
        seen_choices.push_back(request.choices);
        pending_rows_.push_back(request.batch.RowCount());
    }
    std::vector<nlohmann::json>
    CollectCompletions(const std::string& = "application/json") override {
        std::vector<nlohmann::json> results;
        for (const auto rows: pending_rows_) {
            results.push_back({{"items", nlohmann::json(rows, true)}});
        }
        pending_rows_.clear();
        return results;
    }

private:
    std::vector<size_t> pending_rows_;
};

}// namespace

class LlmFilterTypeSafeTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto con = Config::GetConnection();
        con.Query("CREATE SECRET (TYPE TYPESAFE, API_KEY 'test-key');");
        con.Query("CREATE SECRET (TYPE OPENAI, API_KEY 'test-key');");
        RecordingDecisionProvider::seen_model_thresholds.clear();
        RecordingDecisionProvider::seen_tuples.clear();
        RecordingDecisionProvider::seen_choices.clear();
        Model::SetMockProviderFactory([](const ModelDetails& details,
                                         std::shared_ptr<ModelRateLimiter>,
                                         std::shared_ptr<ModelUsageLimiter>) {
            return std::make_shared<RecordingDecisionProvider>(details);
        });
    }

    void TearDown() override { Model::ResetMockProvider(); }
};

TEST_F(LlmFilterTypeSafeTest, ModelThresholdReachesTheProvider) {
    auto con = Config::GetConnection();
    con.Query("DELETE MODEL 'jev-stored-threshold';");
    ASSERT_FALSE(con.Query("CREATE MODEL('jev-stored-threshold', 'jev-1.13', "
                           "'typesafe', {\"threshold\": 0.8});")
                         ->HasError());
    for (const auto& [model, expected]:
         std::vector<std::pair<std::string, double>>{
                 {"{'model_name': 'jev-stored-threshold'}", 0.8},
                 {"{'model_name': 'jev', 'threshold': 0.7}", 0.7}}) {
        const auto results = con.Query(
                "SELECT llm_filter(" + model +
                ", {'prompt': 'complains', 'context_columns': [{'data': t}]}) "
                "FROM unnest(['a']) AS tbl(t);");
        ASSERT_FALSE(results->HasError()) << model << ": " << results->GetError();
        ASSERT_FALSE(RecordingDecisionProvider::seen_model_thresholds.empty())
                << model;
        EXPECT_EQ(RecordingDecisionProvider::seen_model_thresholds.back(),
                  expected)
                << model;
    }
}

TEST_F(LlmFilterTypeSafeTest, RejectsAnInvalidThreshold) {
    auto con = Config::GetConnection();
    for (const auto* query:
         {"SELECT llm_filter({'model_name': 'jev', 'threshold': 1.5}, "
          "{'prompt': 'complains', 'context_columns': [{'data': t}]}) FROM unnest(['a']) AS tbl(t);",
          "SELECT llm_filter({'model_name': 'jev', 'threshold': 'nan'::DOUBLE}, "
          "{'prompt': 'complains', 'context_columns': [{'data': t}]}) FROM unnest(['a']) AS tbl(t);",
          "SELECT llm_filter({'model_name': 'jev', 'threshold': 'high'}, "
          "{'prompt': 'complains', "
          "'context_columns': [{'data': t}]}) FROM unnest(['a']) AS tbl(t);",
          "CREATE MODEL('jev-bad-threshold', 'jev-1.13', 'typesafe', "
          "{\"threshold\": 1.5});"}) {
        const auto results = con.Query(query);
        ASSERT_TRUE(results->HasError()) << query;
        EXPECT_NE(results->GetError().find("threshold"), std::string::npos)
                << results->GetError();
    }
}

TEST_F(LlmFilterTypeSafeTest,
       NullContextValuesReachTheProviderAsTheNullString) {
    auto con = Config::GetConnection();
    const auto results =
            con.Query("SELECT llm_filter({'model_name': 'jev', 'threshold': 0.5}, {'prompt': "
                      "'complains', 'context_columns': [{'data': t}]}) "
                      "FROM (VALUES ('a'), (NULL)) AS tbl(t);");
    ASSERT_FALSE(results->HasError()) << results->GetError();
    ASSERT_EQ(RecordingDecisionProvider::seen_tuples.size(), 1u);
    const auto& data = RecordingDecisionProvider::seen_tuples[0][0]["data"];
    ASSERT_EQ(data.size(), 2u);
    EXPECT_EQ(data[0], "a");
    EXPECT_EQ(data[1], "NULL");
}

TEST_F(LlmFilterTypeSafeTest, AiClassifyPassesTheChoicesToTheProvider) {
    auto con = Config::GetConnection();
    for (const auto& [choice, expected]: std::vector<std::pair<std::string, nlohmann::json>>{
                 {"['bug', 'feature']", {{"bug", nullptr}, {"feature", nullptr}}},
                 {"[{'label': 'bug', 'description': 'Something broke'}, {'label': 'feature', 'description': 'Something new'}]",
                  {{"bug", "Something broke"}, {"feature", "Something new"}}}}) {
        RecordingDecisionProvider::seen_choices.clear();
        const auto results = con.Query("SELECT ai_classify({'model_name': 'jev'}, {'prompt': 'What kind of issue is this?', "
                                       "'choice': " +
                                       choice + ", 'context_columns': [{'data': t}]}) "
                                                "FROM unnest(['a', 'b']) AS tbl(t);");
        ASSERT_FALSE(results->HasError()) << results->GetError();
        ASSERT_EQ(RecordingDecisionProvider::seen_choices.size(), 1u);
        EXPECT_EQ(RecordingDecisionProvider::seen_choices[0], expected) << choice;
    }
}

TEST_F(LlmFilterTypeSafeTest, AiClassifyReturnsTheChoiceWithProbabilities) {
    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT typeof(ai_classify({'model_name': 'jev'}, {'prompt': 'x', 'choice': ['a', 'b'], "
                                   "'context_columns': [{'data': t}]})) "
                                   "FROM unnest(['a']) AS tbl(t);");
    ASSERT_FALSE(results->HasError()) << results->GetError();
    EXPECT_EQ(results->GetValue(0, 0).ToString(),
              "STRUCT(choice VARCHAR, confidence DOUBLE, probabilities MAP(VARCHAR, DOUBLE))");
}

TEST_F(LlmFilterTypeSafeTest, AiClassifyRequiresALabelInEveryChoiceStruct) {
    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT ai_classify({'model_name': 'jev'}, {'prompt': 'x', "
                                   "'choice': [{'description': 'no label'}, {'description': 'none here either'}], "
                                   "'context_columns': [{'data': t}]}) FROM unnest(['a']) AS tbl(t);");
    ASSERT_TRUE(results->HasError());
    EXPECT_NE(results->GetError().find("needs a 'label'"), std::string::npos) << results->GetError();
}

TEST_F(LlmFilterTypeSafeTest, AiClassifyIsTypeSafeOnly) {
    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT ai_classify({'model_name': 'gpt-4o'}, {'prompt': 'x', 'choice': ['a', 'b'], "
                                   "'context_columns': [{'data': t}]}) FROM unnest(['a']) AS tbl(t);");
    ASSERT_TRUE(results->HasError());
    EXPECT_NE(results->GetError().find("supported only by the 'typesafe' provider"), std::string::npos)
            << results->GetError();
}

TEST_F(LlmFilterTypeSafeTest, RefusesUnsupportedFunctionsAtBind) {
    auto con = Config::GetConnection();
    for (const auto* function: {"llm_complete", "llm_embedding"}) {
        const auto results = con.Query(std::string("SELECT ") + function +
                                       "({'model_name': 'jev'}, {'prompt': 'x', "
                                       "'context_columns': [{'data': t}]}) "
                                       "FROM unnest(['a']) AS tbl(t);");
        ASSERT_TRUE(results->HasError()) << function;
        EXPECT_NE(
                results->GetError().find("is not supported by the 'typesafe' provider"),
                std::string::npos)
                << results->GetError();
    }
    for (const auto* aggregate: {"llm_rerank", "llm_first", "llm_last"}) {
        const auto results = con.Query(std::string("SELECT ") + aggregate +
                                       "({'model_name': 'jev'}, {'prompt': 'x', "
                                       "'context_columns': [{'data': t}]}) FROM unnest(['a', 'b']) AS tbl(t);");
        ASSERT_TRUE(results->HasError()) << aggregate;
        EXPECT_NE(results->GetError().find("is not supported by the 'typesafe' provider"), std::string::npos)
                << results->GetError();
    }
    EXPECT_TRUE(RecordingDecisionProvider::seen_tuples.empty());
}

}// namespace flock
