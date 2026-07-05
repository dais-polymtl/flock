#include "../usage_limit_provider.hpp"
#include "flock/core/config.hpp"
#include "flock/functions/scalar/scalar.hpp"
#include "flock/model_manager/model.hpp"
#include "flock/model_manager/providers/provider.hpp"
#include "flock/model_manager/usage_limiter.hpp"

#include <gtest/gtest.h>
#include <memory>

namespace flock {

namespace {

nlohmann::json MakeModelJson(const std::string& model_name, int total_tokens_limit, int batch_size = 1,
                             bool is_async = false) {
    return {
            {"model_name", model_name},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"tuple_format", static_cast<int>(TupleFormat::JSON)},
            {"batch_size", batch_size},
            {"is_async", is_async},
            {"model_parameters", nlohmann::json::object()},
            {"usage_limit", {{"total_tokens_limit", total_tokens_limit}}},
            {"secret", {{"api_key", "test-key"}}}};
}

constexpr const char* kUsageLimitModelName = "gpt-4o-test";

nlohmann::json MakeTuples(int row_count) {
    nlohmann::json data = nlohmann::json::array();
    for (int i = 0; i < row_count; i++) {
        data.push_back("row-" + std::to_string(i));
    }

    return nlohmann::json::array({{{"name", "content"}, {"data", data}}});
}

std::shared_ptr<UsageLimitAwareProvider> g_last_usage_limit_provider;

void InstallUsageLimitProviderFactory() {
    Model::SetMockProviderFactory([](const ModelDetails& details, std::shared_ptr<ModelRateLimiter> rate_limiter,
                                     std::shared_ptr<ModelUsageLimiter> usage_limiter) {
        auto provider = std::make_shared<UsageLimitAwareProvider>(details, std::move(rate_limiter), std::move(usage_limiter));
        provider->SetTokensPerRequest(40, 10);
        g_last_usage_limit_provider = provider;
        return provider;
    });
}

}// namespace

class ScalarUsageLimitTest : public ::testing::Test {
protected:
    void SetUp() override {
        Model::ResetUsageLimiters();

        auto con = Config::GetConnection();
        con.Query("CREATE SECRET (TYPE OPENAI, API_KEY 'your-api-key');");
        InstallUsageLimitProviderFactory();
    }

    void TearDown() override {
        Model::ResetMockProvider();
        Model::ResetUsageLimiters();
    }
};

TEST_F(ScalarUsageLimitTest, SyncBatchStopsWhenTotalUsageLimitExceeded) {
    Model model(MakeModelJson(kUsageLimitModelName, 100));

    const auto responses =
            ScalarFunctionBase::BatchAndCompleteSync(MakeTuples(3), "Summarize", ScalarFunctionType::COMPLETE, model);

    ASSERT_EQ(responses.size(), 3);
    EXPECT_EQ(responses[0].get<std::string>(), "response-0");
    EXPECT_EQ(responses[1].get<std::string>(), "response-1");
    EXPECT_TRUE(responses[2].is_null());

    ASSERT_NE(g_last_usage_limit_provider, nullptr);
    EXPECT_EQ(g_last_usage_limit_provider->collect_call_count, 3);
    EXPECT_EQ(g_last_usage_limit_provider->requests_processed_count, 2);
    EXPECT_EQ(g_last_usage_limit_provider->collects_blocked_at_precheck, 1);
}

TEST_F(ScalarUsageLimitTest, SoftCapReturnsCrossingRequestAndBlocksSubsequentCollects) {
    Model model(MakeModelJson(kUsageLimitModelName, 100));

    model.AddCompletionRequest("prompt", 1, OutputType::STRING);
    const auto first = model.CollectCompletions();
    ASSERT_EQ(first.size(), 1);
    EXPECT_EQ(first[0]["items"][0].get<std::string>(), "response-0");

    model.AddCompletionRequest("prompt", 1, OutputType::STRING);
    const auto crossing = model.CollectCompletions();
    ASSERT_EQ(crossing.size(), 1);
    EXPECT_EQ(crossing[0]["items"][0].get<std::string>(), "response-1");

    model.AddCompletionRequest("prompt", 1, OutputType::STRING);
    EXPECT_THROW(model.CollectCompletions(), UsageLimitExceededError);

    ASSERT_NE(g_last_usage_limit_provider, nullptr);
    EXPECT_EQ(g_last_usage_limit_provider->requests_processed_count, 2);
    EXPECT_EQ(g_last_usage_limit_provider->collect_call_count, 3);
    EXPECT_EQ(g_last_usage_limit_provider->collects_blocked_at_precheck, 1);
    EXPECT_TRUE(Model::GetOrCreateUsageLimiter(kUsageLimitModelName)
                        ->IsLimitExceeded(*model.GetModelDetails().usage_limit));
}

TEST_F(ScalarUsageLimitTest, AsyncBatchStopsWhenTotalUsageLimitExceeded) {
    Model model(MakeModelJson(kUsageLimitModelName, 100, /*batch_size=*/1, /*is_async=*/true));

    const auto responses =
            ScalarFunctionBase::BatchAndCompleteAsync(MakeTuples(3), "Summarize", ScalarFunctionType::COMPLETE, model);

    ASSERT_EQ(responses.size(), 3);
    EXPECT_EQ(responses[0].get<std::string>(), "response-0");
    EXPECT_EQ(responses[1].get<std::string>(), "response-1");
    EXPECT_EQ(responses[2].get<std::string>(), "response-2");

    ASSERT_NE(g_last_usage_limit_provider, nullptr);
    EXPECT_EQ(g_last_usage_limit_provider->collect_call_count, 1);
    EXPECT_EQ(g_last_usage_limit_provider->requests_processed_count, 3);
    EXPECT_EQ(g_last_usage_limit_provider->collects_blocked_at_precheck, 0);
}

TEST_F(ScalarUsageLimitTest, UsagePersistsAcrossModelInstancesWithSameName) {
    {
        Model model(MakeModelJson(kUsageLimitModelName, 120));
        const auto responses = ScalarFunctionBase::BatchAndCompleteSync(
                MakeTuples(2), "Summarize", ScalarFunctionType::COMPLETE, model);
        ASSERT_EQ(responses.size(), 2);
        EXPECT_EQ(responses[0].get<std::string>(), "response-0");
        EXPECT_EQ(responses[1].get<std::string>(), "response-1");
    }

    Model model(MakeModelJson(kUsageLimitModelName, 120));
    const auto responses =
            ScalarFunctionBase::BatchAndCompleteSync(MakeTuples(2), "Summarize", ScalarFunctionType::COMPLETE, model);
    ASSERT_EQ(responses.size(), 2);
    EXPECT_EQ(responses[0].get<std::string>(), "response-0");
    EXPECT_TRUE(responses[1].is_null());

    ASSERT_NE(g_last_usage_limit_provider, nullptr);
    EXPECT_EQ(g_last_usage_limit_provider->requests_processed_count, 1);
    EXPECT_EQ(g_last_usage_limit_provider->collects_blocked_at_precheck, 1);
}

}// namespace flock
