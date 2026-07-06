#include "flock/model_manager/providers/provider.hpp"
#include "flock/model_manager/usage_limiter.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace flock {

class ModelUsageLimiterTest : public ::testing::Test {
protected:
    ModelUsageLimiter limiter;

    static UsageLimit MakeLimit(int64_t prompt = -1, int64_t completion = -1, int64_t total = -1) {
        UsageLimit limit;
        if (prompt > 0) {
            limit.prompt_tokens_limit = prompt;
        }
        if (completion > 0) {
            limit.completion_tokens_limit = completion;
        }
        if (total > 0) {
            limit.total_tokens_limit = total;
        }
        return limit;
    }
};

TEST_F(ModelUsageLimiterTest, AccumulatesUsageAcrossCalls) {
    const auto limit = MakeLimit(1000);

    limiter.RecordUsage(10, 5, limit);
    limiter.RecordUsage(20, 10, limit);

    const auto usage = limiter.GetUsage();
    EXPECT_EQ(usage.prompt_tokens, 30);
    EXPECT_EQ(usage.completion_tokens, 15);
    EXPECT_EQ(usage.total_tokens(), 45);
}

TEST_F(ModelUsageLimiterTest, ThrowsWhenPromptLimitExceeded) {
    const auto limit = MakeLimit(100);

    limiter.RecordUsage(60, 10, limit);
    EXPECT_THROW(limiter.RecordUsage(50, 0, limit), UsageLimitExceededError);
}

TEST_F(ModelUsageLimiterTest, ThrowsWhenCompletionLimitExceeded) {
    const auto limit = MakeLimit(-1, 50);

    limiter.RecordUsage(10, 30, limit);
    EXPECT_THROW(limiter.RecordUsage(0, 25, limit), UsageLimitExceededError);
}

TEST_F(ModelUsageLimiterTest, ThrowsWhenTotalLimitExceeded) {
    const auto limit = MakeLimit(-1, -1, 100);

    limiter.RecordUsage(40, 40, limit);
    EXPECT_THROW(limiter.RecordUsage(10, 15, limit), UsageLimitExceededError);
}

TEST_F(ModelUsageLimiterTest, IndependentBucketsForDifferentModels) {
    const auto limit = MakeLimit(100);

    ModelUsageLimiter limiter_a;
    ModelUsageLimiter limiter_b;
    limiter_a.RecordUsage(90, 0, limit);
    EXPECT_NO_THROW(limiter_b.RecordUsage(90, 0, limit));

    EXPECT_EQ(limiter_a.GetUsage().prompt_tokens, 90);
    EXPECT_EQ(limiter_b.GetUsage().prompt_tokens, 90);
}

TEST_F(ModelUsageLimiterTest, IgnoresWhenNoLimitConfigured) {
    EXPECT_NO_THROW(limiter.RecordUsage(1'000'000, 1'000'000, std::nullopt));
    EXPECT_EQ(limiter.GetUsage().total_tokens(), 0);
}

TEST_F(ModelUsageLimiterTest, UsageLimitExceededErrorHasExpectedProperties) {
    const auto limit = MakeLimit(-1, -1, 100);

    try {
        limiter.RecordUsage(100, 0, limit);
        FAIL() << "Expected UsageLimitExceededError";
    } catch (const UsageLimitExceededError& e) {
        const auto error_json = nlohmann::json::parse(e.what());
        EXPECT_EQ(error_json["exception_type"], "HTTP");
        EXPECT_EQ(error_json["status_code"], "429");
        EXPECT_EQ(error_json["reason"], "usage_limit_exceeded");

        const std::string message = error_json["exception_message"];
        EXPECT_NE(message.find("total_tokens"), std::string::npos);
        EXPECT_NE(message.find("exceeded limit of 100"), std::string::npos);
    }
}

TEST_F(ModelUsageLimiterTest, IsLimitExceededReturnsFalseWhenBelowLimit) {
    const auto limit = MakeLimit(-1, -1, 100);

    limiter.RecordUsage(40, 40, limit);

    EXPECT_FALSE(limiter.IsLimitExceeded(limit));
}

TEST_F(ModelUsageLimiterTest, IsLimitExceededReturnsTrueWhenAtOrAboveLimit) {
    const auto limit = MakeLimit(-1, -1, 100);

    limiter.RecordUsage(60, 0, limit);
    try {
        limiter.RecordUsage(40, 0, limit);
    } catch (const UsageLimitExceededError&) {
    }

    EXPECT_TRUE(limiter.IsLimitExceeded(limit));
}

TEST_F(ModelUsageLimiterTest, ThrowIfLimitExceededThrowsWithSameSemanticsAsRecordUsage) {
    const auto limit = MakeLimit(-1, -1, 100);

    limiter.RecordUsage(60, 0, limit);
    try {
        limiter.RecordUsage(40, 0, limit);
    } catch (const UsageLimitExceededError&) {
    }

    EXPECT_THROW(limiter.ThrowIfLimitExceeded(limit), UsageLimitExceededError);

    ModelUsageLimiter fresh_limiter;
    EXPECT_NO_THROW(fresh_limiter.ThrowIfLimitExceeded(limit));
}

TEST_F(ModelUsageLimiterTest, ResetClearsUsage) {
    const auto limit = MakeLimit(1000);

    limiter.RecordUsage(100, 50, limit);
    limiter.Reset();

    EXPECT_EQ(limiter.GetUsage().total_tokens(), 0);
    EXPECT_NO_THROW(limiter.RecordUsage(100, 50, limit));
}

}// namespace flock
