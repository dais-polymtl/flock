#pragma once

#include "duckdb/function/scalar_function.hpp"
#include <cstdint>
#include <mutex>
#include <nlohmann/json.hpp>

namespace flock {

class FlockMetrics {
public:
    static FlockMetrics& GetInstance() {
        static FlockMetrics instance;
        return instance;
    }

    FlockMetrics(const FlockMetrics&) = delete;
    FlockMetrics& operator=(const FlockMetrics&) = delete;

    void UpdateTokenUsage(int64_t input_tokens, int64_t output_tokens) {
        std::lock_guard<std::mutex> lock(mutex_);
        total_input_tokens_ += input_tokens;
        total_output_tokens_ += output_tokens;
    }

    void IncrementApiCalls() {
        std::lock_guard<std::mutex> lock(mutex_);
        total_api_calls_++;
    }

    void AddApiDuration(double duration_ms) {
        std::lock_guard<std::mutex> lock(mutex_);
        total_api_duration_ms_ += duration_ms;
    }

    void AddExecutionTime(double execution_time_ms) {
        std::lock_guard<std::mutex> lock(mutex_);
        total_execution_time_ms_ += execution_time_ms;
    }

    nlohmann::json GetMetrics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return {
                {"total_input_tokens", total_input_tokens_},
                {"total_output_tokens", total_output_tokens_},
                {"total_tokens", total_input_tokens_ + total_output_tokens_},
                {"total_api_calls", total_api_calls_},
                {"total_api_duration_ms", total_api_duration_ms_},
                {"total_execution_time_ms", total_execution_time_ms_},
                {"avg_api_duration_ms", total_api_calls_ > 0 ? total_api_duration_ms_ / total_api_calls_ : 0.0},
                {"avg_execution_time_ms", total_api_calls_ > 0 ? total_execution_time_ms_ / total_api_calls_ : 0.0}};
    }

    void Reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        total_input_tokens_ = 0;
        total_output_tokens_ = 0;
        total_api_calls_ = 0;
        total_api_duration_ms_ = 0.0;
        total_execution_time_ms_ = 0.0;
    }

    static void ExecuteGetMetrics(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
    static void ExecuteResetMetrics(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);

private:
    FlockMetrics() = default;

    mutable std::mutex mutex_;
    int64_t total_input_tokens_ = 0;
    int64_t total_output_tokens_ = 0;
    int64_t total_api_calls_ = 0;
    double total_api_duration_ms_ = 0.0;
    double total_execution_time_ms_ = 0.0;
};

}// namespace flock
