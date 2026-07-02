#include "flock/model_manager/rate_limiter.hpp"

#include <chrono>
#include <gtest/gtest.h>

namespace flock {

namespace {
// 30 requests per rolling minute.
constexpr int kRateLimit = 30;
}// namespace

class ModelRateLimiterTest : public ::testing::Test {
protected:
    void SetUp() override { ModelRateLimiter::Instance().Reset(); }

    void TearDown() override { ModelRateLimiter::Instance().Reset(); }

    // Friendship does not extend to the TEST_F subclass, so expose the test-only
    // window override through the fixture.
    static void SetWindow(std::chrono::steady_clock::duration window) {
        ModelRateLimiter::Instance().SetWindowForTesting(window);
    }
};

TEST_F(ModelRateLimiterTest, BurstUpToLimitDoesNotWait) {
    auto& limiter = ModelRateLimiter::Instance();
    const auto start = std::chrono::steady_clock::now();

    // A full minute's worth of requests may fire back-to-back.
    limiter.WaitForBatch("model-a", kRateLimit, kRateLimit);

    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 0.05);
}

TEST_F(ModelRateLimiterTest, SeparateBatchesUnderLimitDoNotWait) {
    auto& limiter = ModelRateLimiter::Instance();
    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < kRateLimit; i++) {
        limiter.WaitForBatch("model-a", 1, kRateLimit);
    }

    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 0.05);
}

TEST_F(ModelRateLimiterTest, IndependentBucketsForDifferentModels) {
    auto& limiter = ModelRateLimiter::Instance();
    const auto start = std::chrono::steady_clock::now();

    // Each model has its own rolling window, so filling one does not throttle the other.
    limiter.WaitForBatch("model-a", kRateLimit, kRateLimit);
    limiter.WaitForBatch("model-b", kRateLimit, kRateLimit);

    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 0.05);
}

TEST_F(ModelRateLimiterTest, BlocksWhenOverLimitUntilWindowRolls) {
    auto& limiter = ModelRateLimiter::Instance();
    // Use a short 10s window so the throttle is observable without a full minute.
    SetWindow(std::chrono::seconds(10));

    // Fill the window with a full burst; this returns immediately.
    limiter.WaitForBatch("model-a", kRateLimit, kRateLimit);

    // The next request is over the limit and must wait for the burst to age out.
    const auto start = std::chrono::steady_clock::now();
    limiter.WaitForBatch("model-a", 1, kRateLimit);
    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    EXPECT_GE(elapsed, 9.5);
    EXPECT_LT(elapsed, 12.0);
}

TEST_F(ModelRateLimiterTest, IgnoresInvalidInput) {
    auto& limiter = ModelRateLimiter::Instance();
    const auto start = std::chrono::steady_clock::now();

    limiter.WaitForBatch("", 1, kRateLimit);
    limiter.WaitForBatch("model-a", 0, kRateLimit);
    limiter.WaitForBatch("model-a", 1, 0);

    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 0.02);
}

}// namespace flock
