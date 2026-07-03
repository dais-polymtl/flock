#include "../rate_limit_provider.hpp"
#include "flock/core/config.hpp"
#include "flock/functions/scalar/scalar.hpp"
#include "flock/model_manager/model.hpp"
#include "flock/model_manager/rate_limiter.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <memory>

namespace flock {

namespace {

// 30 requests per rolling minute. The inputs below stay well under this, so no
// throttling delay is expected; these tests verify the limiter is wired into
// both the sync and async execution paths with the correct per-batch counts.
constexpr int kRateLimit = 30;

nlohmann::json MakeModelJson(bool is_async, const std::string& model_name = "rate-limit-test",
                             int rate_limit = kRateLimit, int batch_size = 2) {
    return {
            {"model_name", model_name},
            {"model", "gpt-4o"},
            {"provider", "openai"},
            {"tuple_format", static_cast<int>(TupleFormat::JSON)},
            {"batch_size", batch_size},
            {"rate_limit", rate_limit},
            {"is_async", is_async},
            {"model_parameters", nlohmann::json::object()},
            {"secret", {{"api_key", "test-key"}}}};
}

nlohmann::json MakeTuples(int row_count) {
    nlohmann::json data = nlohmann::json::array();
    for (int i = 0; i < row_count; i++) {
        data.push_back("row-" + std::to_string(i));
    }

    return nlohmann::json::array({{{"name", "content"}, {"data", data}}});
}

std::shared_ptr<RateLimitAwareProvider> g_last_rate_limit_provider;

void InstallRateLimitProviderFactory() {
    Model::SetMockProviderFactory([](const ModelDetails& details) {
        auto provider = std::make_shared<RateLimitAwareProvider>(details);
        g_last_rate_limit_provider = provider;
        return provider;
    });
}

std::shared_ptr<RateLimitAwareProvider> GetLastRateLimitProvider() {
    return g_last_rate_limit_provider;
}

}// namespace

class ScalarRateLimitTest : public ::testing::Test {
protected:
    void SetUp() override {
        Model::GetRateLimiter().Reset();

        auto con = Config::GetConnection();
        con.Query("CREATE SECRET (TYPE OPENAI, API_KEY 'your-api-key');");
        InstallRateLimitProviderFactory();
    }

    void TearDown() override {
        Model::ResetMockProvider();
        Model::GetRateLimiter().Reset();
    }
};

TEST_F(ScalarRateLimitTest, SyncInvokesLimiterPerCollectRoundUnderLimit) {
    Model model(MakeModelJson(false));

    const auto start = std::chrono::steady_clock::now();
    const auto responses =
            ScalarFunctionBase::BatchAndCompleteSync(MakeTuples(4), "Summarize", ScalarFunctionType::COMPLETE, model);
    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    ASSERT_EQ(responses.size(), 4);
    // 4 rows / batch_size 2 => two sequential collect rounds, one request each.
    ASSERT_NE(GetLastRateLimitProvider(), nullptr);
    EXPECT_EQ(GetLastRateLimitProvider()->collect_call_count, 2);
    EXPECT_EQ(GetLastRateLimitProvider()->last_batch_request_count, 1);
    // Well under the limit, so no throttling delay.
    EXPECT_LT(elapsed, 1.0);
}

TEST_F(ScalarRateLimitTest, AsyncInvokesLimiterOncePerBatchGroupUnderLimit) {
    Model model(MakeModelJson(true));

    const auto start = std::chrono::steady_clock::now();
    const auto responses =
            ScalarFunctionBase::BatchAndCompleteAsync(MakeTuples(4), "Summarize", ScalarFunctionType::COMPLETE, model);
    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    ASSERT_EQ(responses.size(), 4);
    ASSERT_NE(GetLastRateLimitProvider(), nullptr);
    // 4 rows / batch_size 2 => one collect round covering two batched requests.
    EXPECT_EQ(GetLastRateLimitProvider()->collect_call_count, 1);
    EXPECT_EQ(GetLastRateLimitProvider()->last_batch_request_count, 2);
    EXPECT_LT(elapsed, 1.0);
}

TEST_F(ScalarRateLimitTest, SyncAndAsyncBothRouteThroughLimiterWithDifferentGrouping) {
    constexpr int row_count = 8;

    Model sync_model(MakeModelJson(false, "rate-limit-sync-test"));
    ScalarFunctionBase::BatchAndCompleteSync(MakeTuples(row_count), "Summarize", ScalarFunctionType::COMPLETE,
                                             sync_model);
    ASSERT_NE(GetLastRateLimitProvider(), nullptr);
    const int sync_collect_calls = GetLastRateLimitProvider()->collect_call_count;
    const int sync_last_batch = GetLastRateLimitProvider()->last_batch_request_count;

    Model async_model(MakeModelJson(true, "rate-limit-async-test"));
    ScalarFunctionBase::BatchAndCompleteAsync(MakeTuples(row_count), "Summarize", ScalarFunctionType::COMPLETE,
                                              async_model);
    ASSERT_NE(GetLastRateLimitProvider(), nullptr);

    // Sync collects one batch at a time (4 rounds); async collects all batches at once.
    EXPECT_EQ(sync_collect_calls, 4);
    EXPECT_EQ(sync_last_batch, 1);
    EXPECT_EQ(GetLastRateLimitProvider()->collect_call_count, 1);
    EXPECT_EQ(GetLastRateLimitProvider()->last_batch_request_count, 4);
}

TEST_F(ScalarRateLimitTest, TwoModelsSequentialEachAtLimitDoNotThrottleEachOther) {
    constexpr int kFivePerMinute = 5;
    constexpr int kRequestsPerModel = 5;

    const auto start = std::chrono::steady_clock::now();

    Model model_a(MakeModelJson(true, "model-a", kFivePerMinute, 1));
    const auto responses_a = ScalarFunctionBase::BatchAndCompleteAsync(
            MakeTuples(kRequestsPerModel), "Summarize", ScalarFunctionType::COMPLETE, model_a);
    ASSERT_EQ(responses_a.size(), kRequestsPerModel);
    ASSERT_NE(GetLastRateLimitProvider(), nullptr);
    EXPECT_EQ(GetLastRateLimitProvider()->collect_call_count, 1);
    EXPECT_EQ(GetLastRateLimitProvider()->last_batch_request_count, kRequestsPerModel);

    Model model_b(MakeModelJson(true, "model-b", kFivePerMinute, 1));
    const auto responses_b = ScalarFunctionBase::BatchAndCompleteAsync(
            MakeTuples(kRequestsPerModel), "Summarize", ScalarFunctionType::COMPLETE, model_b);
    ASSERT_EQ(responses_b.size(), kRequestsPerModel);
    ASSERT_NE(GetLastRateLimitProvider(), nullptr);
    EXPECT_EQ(GetLastRateLimitProvider()->collect_call_count, 1);
    EXPECT_EQ(GetLastRateLimitProvider()->last_batch_request_count, kRequestsPerModel);

    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    // 5 + 5 requests across independent per-model buckets — no throttling wait.
    EXPECT_LT(elapsed, 1.0);
}

}// namespace flock
