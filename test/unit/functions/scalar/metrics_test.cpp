#include "flock/core/config.hpp"
#include "flock/metrics/metrics.hpp"
#include <gtest/gtest.h>

namespace flock {

class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        FlockMetrics::GetInstance().Reset();
    }

    void TearDown() override {
        FlockMetrics::GetInstance().Reset();
    }
};

TEST_F(MetricsTest, InitialMetricsAreZero) {
    auto metrics = FlockMetrics::GetInstance().GetMetrics();

    EXPECT_EQ(metrics["total_input_tokens"].get<int64_t>(), 0);
    EXPECT_EQ(metrics["total_output_tokens"].get<int64_t>(), 0);
    EXPECT_EQ(metrics["total_tokens"].get<int64_t>(), 0);
    EXPECT_EQ(metrics["total_api_calls"].get<int64_t>(), 0);
    EXPECT_DOUBLE_EQ(metrics["total_api_duration_ms"].get<double>(), 0.0);
    EXPECT_DOUBLE_EQ(metrics["total_execution_time_ms"].get<double>(), 0.0);
}

TEST_F(MetricsTest, UpdateTokenUsage) {
    FlockMetrics::GetInstance().UpdateTokenUsage(100, 50);

    auto metrics = FlockMetrics::GetInstance().GetMetrics();
    EXPECT_EQ(metrics["total_input_tokens"].get<int64_t>(), 100);
    EXPECT_EQ(metrics["total_output_tokens"].get<int64_t>(), 50);
    EXPECT_EQ(metrics["total_tokens"].get<int64_t>(), 150);
}

TEST_F(MetricsTest, IncrementApiCalls) {
    FlockMetrics::GetInstance().IncrementApiCalls();
    FlockMetrics::GetInstance().IncrementApiCalls();
    FlockMetrics::GetInstance().IncrementApiCalls();

    auto metrics = FlockMetrics::GetInstance().GetMetrics();
    EXPECT_EQ(metrics["total_api_calls"].get<int64_t>(), 3);
}

TEST_F(MetricsTest, AddApiDuration) {
    FlockMetrics::GetInstance().AddApiDuration(100.5);
    FlockMetrics::GetInstance().AddApiDuration(200.25);

    auto metrics = FlockMetrics::GetInstance().GetMetrics();
    EXPECT_DOUBLE_EQ(metrics["total_api_duration_ms"].get<double>(), 300.75);
}

TEST_F(MetricsTest, AddExecutionTime) {
    FlockMetrics::GetInstance().AddExecutionTime(150.0);
    FlockMetrics::GetInstance().AddExecutionTime(250.0);

    auto metrics = FlockMetrics::GetInstance().GetMetrics();
    EXPECT_DOUBLE_EQ(metrics["total_execution_time_ms"].get<double>(), 400.0);
}

TEST_F(MetricsTest, AveragesCalculatedCorrectly) {
    FlockMetrics::GetInstance().IncrementApiCalls();
    FlockMetrics::GetInstance().IncrementApiCalls();
    FlockMetrics::GetInstance().AddApiDuration(100.0);
    FlockMetrics::GetInstance().AddApiDuration(200.0);
    FlockMetrics::GetInstance().AddExecutionTime(150.0);
    FlockMetrics::GetInstance().AddExecutionTime(250.0);

    auto metrics = FlockMetrics::GetInstance().GetMetrics();
    EXPECT_DOUBLE_EQ(metrics["avg_api_duration_ms"].get<double>(), 150.0);
    EXPECT_DOUBLE_EQ(metrics["avg_execution_time_ms"].get<double>(), 200.0);
}

TEST_F(MetricsTest, AveragesZeroWhenNoApiCalls) {
    FlockMetrics::GetInstance().AddApiDuration(100.0);
    FlockMetrics::GetInstance().AddExecutionTime(150.0);

    auto metrics = FlockMetrics::GetInstance().GetMetrics();
    EXPECT_DOUBLE_EQ(metrics["avg_api_duration_ms"].get<double>(), 0.0);
    EXPECT_DOUBLE_EQ(metrics["avg_execution_time_ms"].get<double>(), 0.0);
}

TEST_F(MetricsTest, ResetClearsAllMetrics) {
    FlockMetrics::GetInstance().UpdateTokenUsage(100, 50);
    FlockMetrics::GetInstance().IncrementApiCalls();
    FlockMetrics::GetInstance().AddApiDuration(100.0);
    FlockMetrics::GetInstance().AddExecutionTime(150.0);

    FlockMetrics::GetInstance().Reset();

    auto metrics = FlockMetrics::GetInstance().GetMetrics();
    EXPECT_EQ(metrics["total_input_tokens"].get<int64_t>(), 0);
    EXPECT_EQ(metrics["total_output_tokens"].get<int64_t>(), 0);
    EXPECT_EQ(metrics["total_api_calls"].get<int64_t>(), 0);
    EXPECT_DOUBLE_EQ(metrics["total_api_duration_ms"].get<double>(), 0.0);
    EXPECT_DOUBLE_EQ(metrics["total_execution_time_ms"].get<double>(), 0.0);
}

TEST_F(MetricsTest, AccumulatesMultipleUpdates) {
    FlockMetrics::GetInstance().UpdateTokenUsage(100, 50);
    FlockMetrics::GetInstance().UpdateTokenUsage(200, 100);
    FlockMetrics::GetInstance().UpdateTokenUsage(50, 25);

    auto metrics = FlockMetrics::GetInstance().GetMetrics();
    EXPECT_EQ(metrics["total_input_tokens"].get<int64_t>(), 350);
    EXPECT_EQ(metrics["total_output_tokens"].get<int64_t>(), 175);
    EXPECT_EQ(metrics["total_tokens"].get<int64_t>(), 525);
}

TEST_F(MetricsTest, SqlFunctionFlockGetMetrics) {
    FlockMetrics::GetInstance().UpdateTokenUsage(100, 50);
    FlockMetrics::GetInstance().IncrementApiCalls();

    auto con = Config::GetConnection();
    auto results = con.Query("SELECT flock_get_metrics() AS metrics;");

    ASSERT_FALSE(results->HasError()) << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);

    auto json_str = results->GetValue(0, 0).GetValue<std::string>();
    auto metrics = nlohmann::json::parse(json_str);

    EXPECT_EQ(metrics["total_input_tokens"].get<int64_t>(), 100);
    EXPECT_EQ(metrics["total_output_tokens"].get<int64_t>(), 50);
    EXPECT_EQ(metrics["total_api_calls"].get<int64_t>(), 1);
}

TEST_F(MetricsTest, SqlFunctionFlockResetMetrics) {
    FlockMetrics::GetInstance().UpdateTokenUsage(100, 50);
    FlockMetrics::GetInstance().IncrementApiCalls();

    auto con = Config::GetConnection();
    auto results = con.Query("SELECT flock_reset_metrics() AS result;");

    ASSERT_FALSE(results->HasError()) << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);

    auto metrics = FlockMetrics::GetInstance().GetMetrics();
    EXPECT_EQ(metrics["total_input_tokens"].get<int64_t>(), 0);
    EXPECT_EQ(metrics["total_output_tokens"].get<int64_t>(), 0);
    EXPECT_EQ(metrics["total_api_calls"].get<int64_t>(), 0);
}

}// namespace flock
