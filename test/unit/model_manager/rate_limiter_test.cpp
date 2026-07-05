#include "flock/model_manager/rate_limiter.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

namespace flock {

namespace {
// 30 requests per rolling minute.
constexpr int kRateLimit = 30;
}// namespace

class ModelRateLimiterTest : public ::testing::Test {
protected:
    void SetUp() override {
        limiter = std::make_unique<ModelRateLimiter>(std::chrono::seconds(10));
    }

    std::unique_ptr<ModelRateLimiter> limiter;
};

TEST_F(ModelRateLimiterTest, BurstUpToLimitDoesNotWait) {
    const auto start = std::chrono::steady_clock::now();

    // A full minute's worth of requests may fire back-to-back.
    limiter->WaitForBatchIfNeeded(kRateLimit, kRateLimit);

    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 0.05);
}

TEST_F(ModelRateLimiterTest, SeparateBatchesUnderLimitDoNotWait) {
    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < kRateLimit; i++) {
        limiter->WaitForBatchIfNeeded(1, kRateLimit);
    }

    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 0.05);
}

TEST_F(ModelRateLimiterTest, IndependentBucketsForDifferentModels) {
    const auto start = std::chrono::steady_clock::now();

    // Each model has its own limiter instance, so filling one does not throttle the other.
    ModelRateLimiter limiter_a(std::chrono::seconds(10));
    ModelRateLimiter limiter_b(std::chrono::seconds(10));
    limiter_a.WaitForBatchIfNeeded(kRateLimit, kRateLimit);
    limiter_b.WaitForBatchIfNeeded(kRateLimit, kRateLimit);

    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 0.05);
}

TEST_F(ModelRateLimiterTest, BlocksWhenOverLimitUntilWindowRolls) {
    // Use a short 10s window so the throttle is observable without a full minute.

    // Fill the window with a full burst; this returns immediately.
    limiter->WaitForBatchIfNeeded(kRateLimit, kRateLimit);

    // The next request is over the limit and must wait for the burst to age out.
    const auto start = std::chrono::steady_clock::now();
    limiter->WaitForBatchIfNeeded(1, kRateLimit);
    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    EXPECT_GE(elapsed, 9.5);
    EXPECT_LT(elapsed, 12.0);
}

TEST_F(ModelRateLimiterTest, IgnoresInvalidInput) {
    const auto start = std::chrono::steady_clock::now();

    limiter->WaitForBatchIfNeeded(0, kRateLimit);
    limiter->WaitForBatchIfNeeded(1, 0);

    const auto elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 0.02);
}

TEST_F(ModelRateLimiterTest, ConcurrentModelsDoNotBlockEachOther) {
    constexpr int kSmallLimit = 2;
    auto limiter_a = std::make_shared<ModelRateLimiter>(std::chrono::seconds(2));
    auto limiter_b = std::make_shared<ModelRateLimiter>(std::chrono::seconds(2));

    // Fill limiter_a so the next call must sleep.
    limiter_a->WaitForBatchIfNeeded(kSmallLimit, kSmallLimit);

    std::atomic<bool> thread_b_finished = false;
    const auto thread_b_start = std::chrono::steady_clock::now();

    auto future_a = std::async(std::launch::async, [&]() {
        limiter_a->WaitForBatchIfNeeded(1, kSmallLimit);
    });

    // Give thread A time to enter the sleep path.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto future_b = std::async(std::launch::async, [&]() {
        limiter_b->WaitForBatchIfNeeded(kSmallLimit, kSmallLimit);
        thread_b_finished = true;
    });

    future_b.wait();
    const auto thread_b_elapsed =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - thread_b_start).count();

    EXPECT_TRUE(thread_b_finished);
    EXPECT_LT(thread_b_elapsed, 0.5);

    future_a.wait();
}

}// namespace flock
