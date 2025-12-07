#include "flock/core/config.hpp"
#include "flock/metrics/manager.hpp"
#include <gtest/gtest.h>

namespace flock {

class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto con = Config::GetConnection();
        // Reset metrics before each test to ensure clean state
        auto& manager = MetricsManager::GetForDatabase(GetDatabase());
        manager.Reset();
    }

    duckdb::DatabaseInstance* GetDatabase() {
        return Config::db;
    }

    MetricsManager& GetMetricsManager() {
        return MetricsManager::GetForDatabase(GetDatabase());
    }
};

TEST_F(MetricsTest, InitialMetricsAreZero) {
    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    EXPECT_TRUE(metrics.is_object());
    EXPECT_TRUE(metrics.empty());
}

TEST_F(MetricsTest, UpdateTokensForLlmComplete) {
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0x1234);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_COMPLETE);
    MetricsManager::UpdateTokens(100, 50);

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    bool found = false;
    for (const auto& [key, value]: metrics.items()) {
        if (key.find("llm_complete_") == 0) {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 100);
            EXPECT_EQ(value["output_tokens"].get<int64_t>(), 50);
            EXPECT_EQ(value["total_tokens"].get<int64_t>(), 150);
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(MetricsTest, TracksDifferentFunctionsSeparately) {
    auto* db = GetDatabase();
    const void* state_id1 = reinterpret_cast<const void*>(0x1234);
    const void* state_id2 = reinterpret_cast<const void*>(0x5678);

    MetricsManager::StartInvocation(db, state_id1, FunctionType::LLM_COMPLETE);
    MetricsManager::UpdateTokens(100, 50);
    MetricsManager::AddExecutionTime(1000.0);

    MetricsManager::StartInvocation(db, state_id2, FunctionType::LLM_FILTER);
    MetricsManager::UpdateTokens(200, 100);
    MetricsManager::AddExecutionTime(2000.0);

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    bool found_complete = false;
    bool found_filter = false;
    int64_t total_input = 0;
    int64_t total_output = 0;

    for (const auto& [key, value]: metrics.items()) {
        if (key.find("llm_complete_") == 0) {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 100);
            total_input += value["input_tokens"].get<int64_t>();
            total_output += value["output_tokens"].get<int64_t>();
            found_complete = true;
        } else if (key.find("llm_filter_") == 0) {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 200);
            total_input += value["input_tokens"].get<int64_t>();
            total_output += value["output_tokens"].get<int64_t>();
            found_filter = true;
        }
    }

    EXPECT_TRUE(found_complete);
    EXPECT_TRUE(found_filter);
    EXPECT_EQ(total_input, 300);
    EXPECT_EQ(total_output, 150);
}

TEST_F(MetricsTest, IncrementApiCalls) {
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0x1234);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_COMPLETE);
    MetricsManager::IncrementApiCalls();
    MetricsManager::IncrementApiCalls();

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_FILTER);
    MetricsManager::IncrementApiCalls();

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    int64_t total_api_calls = 0;
    int64_t complete_calls = 0;
    int64_t filter_calls = 0;

    for (const auto& [key, value]: metrics.items()) {
        if (key.find("llm_complete_") == 0) {
            complete_calls = value["api_calls"].get<int64_t>();
            total_api_calls += complete_calls;
        } else if (key.find("llm_filter_") == 0) {
            filter_calls = value["api_calls"].get<int64_t>();
            total_api_calls += filter_calls;
        }
    }

    EXPECT_EQ(total_api_calls, 3);
    EXPECT_EQ(complete_calls, 2);
    EXPECT_EQ(filter_calls, 1);
}

TEST_F(MetricsTest, AddApiDuration) {
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0x1234);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_COMPLETE);
    MetricsManager::AddApiDuration(100.5);
    MetricsManager::AddApiDuration(200.25);

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    bool found = false;
    for (const auto& [key, value]: metrics.items()) {
        if (key.find("llm_complete_") == 0) {
            EXPECT_NEAR(value["api_duration_ms"].get<double>(), 300.75, 0.01);
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(MetricsTest, AddExecutionTime) {
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0x1234);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_COMPLETE);
    MetricsManager::AddExecutionTime(150.0);
    MetricsManager::AddExecutionTime(250.0);

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    bool found = false;
    for (const auto& [key, value]: metrics.items()) {
        if (key.find("llm_complete_") == 0) {
            EXPECT_NEAR(value["execution_time_ms"].get<double>(), 400.0, 0.01);
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(MetricsTest, ResetClearsAllMetrics) {
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0x1234);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_COMPLETE);
    MetricsManager::UpdateTokens(100, 50);
    MetricsManager::IncrementApiCalls();
    MetricsManager::AddApiDuration(100.0);
    MetricsManager::AddExecutionTime(150.0);

    auto& manager = GetMetricsManager();
    manager.Reset();

    auto metrics = manager.GetMetrics();
    EXPECT_TRUE(metrics.is_object());
    EXPECT_TRUE(metrics.empty());
}

TEST_F(MetricsTest, SqlFunctionFlockGetMetrics) {
    auto con = Config::GetConnection();
    auto results = con.Query("SELECT flock_get_metrics() AS metrics;");

    ASSERT_FALSE(results->HasError()) << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);

    auto json_str = results->GetValue(0, 0).GetValue<std::string>();
    auto metrics = nlohmann::json::parse(json_str);

    EXPECT_TRUE(metrics.is_object());
}

TEST_F(MetricsTest, SqlFunctionFlockResetMetrics) {
    auto con = Config::GetConnection();
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0x1234);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_COMPLETE);
    MetricsManager::UpdateTokens(100, 50);
    MetricsManager::IncrementApiCalls();

    auto results = con.Query("SELECT flock_reset_metrics() AS result;");

    ASSERT_FALSE(results->HasError()) << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();
    EXPECT_TRUE(metrics.is_object());
    EXPECT_TRUE(metrics.empty());
}

TEST_F(MetricsTest, SequentialNumberingForMultipleCalls) {
    auto* db = GetDatabase();
    const void* state_id1 = reinterpret_cast<const void*>(0x1111);
    const void* state_id2 = reinterpret_cast<const void*>(0x2222);
    const void* state_id3 = reinterpret_cast<const void*>(0x3333);

    MetricsManager::StartInvocation(db, state_id1, FunctionType::LLM_FILTER);
    MetricsManager::UpdateTokens(10, 5);

    MetricsManager::StartInvocation(db, state_id2, FunctionType::LLM_FILTER);
    MetricsManager::UpdateTokens(20, 10);

    MetricsManager::StartInvocation(db, state_id3, FunctionType::LLM_FILTER);
    MetricsManager::UpdateTokens(30, 15);

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    bool found_1 = false, found_2 = false, found_3 = false;
    for (const auto& [key, value]: metrics.items()) {
        if (key == "llm_filter_1") {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 10);
            found_1 = true;
        } else if (key == "llm_filter_2") {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 20);
            found_2 = true;
        } else if (key == "llm_filter_3") {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 30);
            found_3 = true;
        }
    }

    EXPECT_TRUE(found_1) << "llm_filter_1 not found";
    EXPECT_TRUE(found_2) << "llm_filter_2 not found";
    EXPECT_TRUE(found_3) << "llm_filter_3 not found";
}

TEST_F(MetricsTest, DebugMetricsReturnsNestedStructure) {
    auto* db = GetDatabase();
    const void* state_id1 = reinterpret_cast<const void*>(0x1111);
    const void* state_id2 = reinterpret_cast<const void*>(0x2222);

    MetricsManager::StartInvocation(db, state_id1, FunctionType::LLM_COMPLETE);
    MetricsManager::UpdateTokens(100, 50);
    MetricsManager::SetModelInfo("gpt-4o", "openai");

    MetricsManager::StartInvocation(db, state_id2, FunctionType::LLM_FILTER);
    MetricsManager::UpdateTokens(200, 100);
    MetricsManager::SetModelInfo("gpt-4o", "openai");

    auto& manager = GetMetricsManager();
    auto debug_metrics = manager.GetDebugMetrics();

    EXPECT_TRUE(debug_metrics.is_object());
    EXPECT_TRUE(debug_metrics.contains("threads"));
    EXPECT_TRUE(debug_metrics.contains("thread_count"));
    EXPECT_GE(debug_metrics["thread_count"].get<size_t>(), 1);

    auto threads = debug_metrics["threads"];
    EXPECT_TRUE(threads.is_object());
}

TEST_F(MetricsTest, DebugMetricsContainsRegistrationOrder) {
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0x1234);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_COMPLETE);
    MetricsManager::UpdateTokens(100, 50);

    auto& manager = GetMetricsManager();
    auto debug_metrics = manager.GetDebugMetrics();

    bool found_registration_order = false;
    for (const auto& [thread_id, thread_data]: debug_metrics["threads"].items()) {
        for (const auto& [state_id_str, state_data]: thread_data.items()) {
            if (state_data.contains("llm_complete")) {
                EXPECT_TRUE(state_data["llm_complete"].contains("registration_order"));
                EXPECT_GT(state_data["llm_complete"]["registration_order"].get<size_t>(), 0);
                found_registration_order = true;
            }
        }
    }
    EXPECT_TRUE(found_registration_order);
}

TEST_F(MetricsTest, SqlFunctionFlockGetDebugMetrics) {
    auto con = Config::GetConnection();
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0x1234);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_COMPLETE);
    MetricsManager::UpdateTokens(100, 50);

    auto results = con.Query("SELECT flock_get_debug_metrics() AS debug_metrics;");

    ASSERT_FALSE(results->HasError()) << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);

    auto json_str = results->GetValue(0, 0).GetValue<std::string>();
    auto debug_metrics = nlohmann::json::parse(json_str);

    EXPECT_TRUE(debug_metrics.is_object());
    EXPECT_TRUE(debug_metrics.contains("threads"));
    EXPECT_TRUE(debug_metrics.contains("thread_count"));
}

TEST_F(MetricsTest, AggregateFunctionMetricsTracking) {
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0xAAAA);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_REDUCE);
    MetricsManager::UpdateTokens(500, 200);
    MetricsManager::SetModelInfo("gpt-4o", "openai");
    MetricsManager::IncrementApiCalls();
    MetricsManager::AddApiDuration(2000.0);
    MetricsManager::AddExecutionTime(2500.0);

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    bool found = false;
    for (const auto& [key, value]: metrics.items()) {
        if (key.find("llm_reduce_") == 0) {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 500);
            EXPECT_EQ(value["output_tokens"].get<int64_t>(), 200);
            EXPECT_EQ(value["total_tokens"].get<int64_t>(), 700);
            EXPECT_EQ(value["api_calls"].get<int64_t>(), 1);
            EXPECT_NEAR(value["api_duration_ms"].get<double>(), 2000.0, 0.01);
            EXPECT_NEAR(value["execution_time_ms"].get<double>(), 2500.0, 0.01);
            EXPECT_EQ(value["model_name"].get<std::string>(), "gpt-4o");
            EXPECT_EQ(value["provider"].get<std::string>(), "openai");
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(MetricsTest, MultipleAggregateFunctionsSequentialNumbering) {
    auto* db = GetDatabase();
    const void* state_id1 = reinterpret_cast<const void*>(0xBBBB);
    const void* state_id2 = reinterpret_cast<const void*>(0xCCCC);

    MetricsManager::StartInvocation(db, state_id1, FunctionType::LLM_REDUCE);
    MetricsManager::UpdateTokens(100, 50);

    MetricsManager::StartInvocation(db, state_id2, FunctionType::LLM_REDUCE);
    MetricsManager::UpdateTokens(200, 100);

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    bool found_1 = false, found_2 = false;
    for (const auto& [key, value]: metrics.items()) {
        if (key == "llm_reduce_1") {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 100);
            found_1 = true;
        } else if (key == "llm_reduce_2") {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 200);
            found_2 = true;
        }
    }

    EXPECT_TRUE(found_1) << "llm_reduce_1 not found";
    EXPECT_TRUE(found_2) << "llm_reduce_2 not found";
}

TEST_F(MetricsTest, AggregateFunctionMetricsMerging) {
    auto* db = GetDatabase();
    const void* state_id1 = reinterpret_cast<const void*>(0xAAAA);
    const void* state_id2 = reinterpret_cast<const void*>(0xBBBB);
    const void* state_id3 = reinterpret_cast<const void*>(0xCCCC);

    // Simulate multiple states being processed in a single aggregate call
    // Each state tracks its own metrics
    MetricsManager::StartInvocation(db, state_id1, FunctionType::LLM_REDUCE);
    MetricsManager::SetModelInfo("gpt-4o", "openai");
    MetricsManager::UpdateTokens(100, 50);
    MetricsManager::IncrementApiCalls();
    MetricsManager::AddApiDuration(100.0);
    MetricsManager::AddExecutionTime(150.0);

    MetricsManager::StartInvocation(db, state_id2, FunctionType::LLM_REDUCE);
    MetricsManager::SetModelInfo("gpt-4o", "openai");
    MetricsManager::UpdateTokens(200, 100);
    MetricsManager::IncrementApiCalls();
    MetricsManager::AddApiDuration(200.0);
    MetricsManager::AddExecutionTime(250.0);

    MetricsManager::StartInvocation(db, state_id3, FunctionType::LLM_REDUCE);
    MetricsManager::SetModelInfo("gpt-4o", "openai");
    MetricsManager::UpdateTokens(150, 75);
    MetricsManager::IncrementApiCalls();
    MetricsManager::AddApiDuration(150.0);
    MetricsManager::AddExecutionTime(200.0);

    // Now merge all metrics into the first state
    std::vector<const void*> processed_state_ids = {state_id1, state_id2, state_id3};
    MetricsManager::MergeAggregateMetrics(db, processed_state_ids, FunctionType::LLM_REDUCE, "gpt-4o", "openai");

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    // Should have exactly ONE llm_reduce entry (merged)
    int reduce_count = 0;
    int64_t total_input_tokens = 0;
    int64_t total_output_tokens = 0;
    int64_t total_api_calls = 0;
    double total_api_duration = 0.0;
    double total_execution_time = 0.0;

    for (const auto& [key, value]: metrics.items()) {
        if (key.find("llm_reduce_") == 0) {
            reduce_count++;
            total_input_tokens += value["input_tokens"].get<int64_t>();
            total_output_tokens += value["output_tokens"].get<int64_t>();
            total_api_calls += value["api_calls"].get<int64_t>();
            total_api_duration += value["api_duration_ms"].get<double>();
            total_execution_time += value["execution_time_ms"].get<double>();
        }
    }

    // Should have exactly one merged entry
    EXPECT_EQ(reduce_count, 1) << "Expected exactly 1 merged llm_reduce metrics entry";

    // Verify merged values are the sum of all states
    EXPECT_EQ(total_input_tokens, 450) << "Merged input tokens should be sum of all states (100+200+150)";
    EXPECT_EQ(total_output_tokens, 225) << "Merged output tokens should be sum of all states (50+100+75)";
    EXPECT_EQ(total_api_calls, 3) << "Merged API calls should be sum of all states (1+1+1)";
    EXPECT_NEAR(total_api_duration, 450.0, 0.01) << "Merged API duration should be sum of all states (100+200+150)";
    EXPECT_NEAR(total_execution_time, 600.0, 0.01) << "Merged execution time should be sum of all states (150+250+200)";
}

TEST_F(MetricsTest, AggregateFunctionDebugMetrics) {
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0xDDDD);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_RERANK);
    MetricsManager::UpdateTokens(300, 150);
    MetricsManager::SetModelInfo("gpt-4o", "openai");

    auto& manager = GetMetricsManager();
    auto debug_metrics = manager.GetDebugMetrics();

    bool found_rerank = false;
    for (const auto& [thread_id, thread_data]: debug_metrics["threads"].items()) {
        for (const auto& [state_id_str, state_data]: thread_data.items()) {
            if (state_data.contains("llm_rerank")) {
                EXPECT_EQ(state_data["llm_rerank"]["input_tokens"].get<int64_t>(), 300);
                EXPECT_EQ(state_data["llm_rerank"]["output_tokens"].get<int64_t>(), 150);
                EXPECT_TRUE(state_data["llm_rerank"].contains("registration_order"));
                found_rerank = true;
            }
        }
    }
    EXPECT_TRUE(found_rerank);
}

TEST_F(MetricsTest, MixedScalarAndAggregateMetrics) {
    auto* db = GetDatabase();
    const void* scalar_state = reinterpret_cast<const void*>(0xEEEE);
    const void* aggregate_state = reinterpret_cast<const void*>(0xFFFF);

    MetricsManager::StartInvocation(db, scalar_state, FunctionType::LLM_COMPLETE);
    MetricsManager::UpdateTokens(100, 50);

    MetricsManager::StartInvocation(db, aggregate_state, FunctionType::LLM_REDUCE);
    MetricsManager::UpdateTokens(200, 100);

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    bool found_complete = false, found_reduce = false;
    for (const auto& [key, value]: metrics.items()) {
        if (key.find("llm_complete_") == 0) {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 100);
            found_complete = true;
        } else if (key.find("llm_reduce_") == 0) {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 200);
            found_reduce = true;
        }
    }

    EXPECT_TRUE(found_complete);
    EXPECT_TRUE(found_reduce);
}

TEST_F(MetricsTest, EmbeddingMetricsTracking) {
    auto* db = GetDatabase();
    const void* state_id = reinterpret_cast<const void*>(0xABCD);

    MetricsManager::StartInvocation(db, state_id, FunctionType::LLM_EMBEDDING);
    MetricsManager::SetModelInfo("text-embedding-3-small", "openai");
    // For embeddings, typically only input tokens are used (no output tokens)
    MetricsManager::UpdateTokens(150, 0);
    MetricsManager::IncrementApiCalls();
    MetricsManager::AddApiDuration(250.0);
    MetricsManager::AddExecutionTime(300.0);

    auto& manager = GetMetricsManager();
    auto metrics = manager.GetMetrics();

    bool found = false;
    for (const auto& [key, value]: metrics.items()) {
        if (key.find("llm_embedding_") == 0) {
            EXPECT_EQ(value["input_tokens"].get<int64_t>(), 150);
            EXPECT_EQ(value["output_tokens"].get<int64_t>(), 0);
            EXPECT_EQ(value["total_tokens"].get<int64_t>(), 150);
            EXPECT_EQ(value["api_calls"].get<int64_t>(), 1);
            EXPECT_NEAR(value["api_duration_ms"].get<double>(), 250.0, 0.01);
            EXPECT_NEAR(value["execution_time_ms"].get<double>(), 300.0, 0.01);
            EXPECT_EQ(value["model_name"].get<std::string>(), "text-embedding-3-small");
            EXPECT_EQ(value["provider"].get<std::string>(), "openai");
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

}// namespace flock
