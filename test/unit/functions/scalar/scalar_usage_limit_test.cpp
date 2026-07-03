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
    Model::SetMockProviderFactory([](const ModelDetails& details) {
        auto provider = std::make_shared<UsageLimitAwareProvider>(details);
        provider->SetTokensPerRequest(40, 10);
        g_last_usage_limit_provider = provider;
        return provider;
    });
}

}// namespace

class ScalarUsageLimitTest : public ::testing::Test {
protected:
    void SetUp() override {
        Model::GetUsageLimiter().Reset();

        auto con = Config::GetConnection();
        con.Query("CREATE SECRET (TYPE OPENAI, API_KEY 'your-api-key');");
        InstallUsageLimitProviderFactory();
    }

    void TearDown() override {
        Model::ResetMockProvider();
        Model::GetUsageLimiter().Reset();
    }
};

TEST_F(ScalarUsageLimitTest, SyncBatchStopsWhenTotalUsageLimitExceeded) {
    Model model(MakeModelJson(kUsageLimitModelName, 100));

    EXPECT_THROW(ScalarFunctionBase::BatchAndCompleteSync(MakeTuples(3), "Summarize", ScalarFunctionType::COMPLETE,
                                                          model),
                 UsageLimitExceededError);

    ASSERT_NE(g_last_usage_limit_provider, nullptr);
    EXPECT_EQ(g_last_usage_limit_provider->collect_call_count, 2);
}

TEST_F(ScalarUsageLimitTest, AsyncBatchStopsWhenTotalUsageLimitExceeded) {
    Model model(MakeModelJson(kUsageLimitModelName, 100, /*batch_size=*/1, /*is_async=*/true));

    EXPECT_THROW(ScalarFunctionBase::BatchAndCompleteAsync(MakeTuples(3), "Summarize", ScalarFunctionType::COMPLETE,
                                                           model),
                 UsageLimitExceededError);

    ASSERT_NE(g_last_usage_limit_provider, nullptr);
    EXPECT_EQ(g_last_usage_limit_provider->collect_call_count, 1);
}

TEST_F(ScalarUsageLimitTest, UsagePersistsAcrossModelInstancesWithSameName) {
    {
        Model model(MakeModelJson(kUsageLimitModelName, 120));
        const auto responses = ScalarFunctionBase::BatchAndCompleteSync(
                MakeTuples(2), "Summarize", ScalarFunctionType::COMPLETE, model);
        ASSERT_EQ(responses.size(), 2);
    }

    Model model(MakeModelJson(kUsageLimitModelName, 120));
    EXPECT_THROW(ScalarFunctionBase::BatchAndCompleteSync(MakeTuples(2), "Summarize", ScalarFunctionType::COMPLETE,
                                                          model),
                 UsageLimitExceededError);
}

}// namespace flock
