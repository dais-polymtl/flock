#pragma once

#include "flock/core/common.hpp"
#include "flock/metrics/types.hpp"
#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <thread>
#include <unordered_map>

namespace flock {

// Stores aggregated metrics for a single function call
struct FunctionMetricsData {
    std::string model_name;
    std::string provider;
    int64_t input_tokens = 0;
    int64_t output_tokens = 0;
    int64_t api_calls = 0;
    int64_t api_duration_us = 0;
    int64_t execution_time_us = 0;

    int64_t total_tokens() const noexcept {
        return input_tokens + output_tokens;
    }

    double api_duration_ms() const noexcept {
        return api_duration_us / 1000.0;
    }

    double execution_time_ms() const noexcept {
        return execution_time_us / 1000.0;
    }

    bool IsEmpty() const noexcept {
        return input_tokens == 0 && output_tokens == 0 && api_calls == 0 &&
               api_duration_us == 0 && execution_time_us == 0;
    }

    nlohmann::json ToJson() const {
        nlohmann::json result = {
                {"input_tokens", input_tokens},
                {"output_tokens", output_tokens},
                {"total_tokens", total_tokens()},
                {"api_calls", api_calls},
                {"api_duration_ms", api_duration_ms()},
                {"execution_time_ms", execution_time_ms()}};

        if (!model_name.empty()) {
            result["model_name"] = model_name;
        }
        if (!provider.empty()) {
            result["provider"] = provider;
        }

        return result;
    }
};

// Stores metrics for all function types in a single state
class ThreadMetrics {
public:
    static constexpr size_t NUM_FUNCTION_TYPES = 8;

    void Reset() noexcept {
        for (auto& func_metrics: by_function_) {
            func_metrics = FunctionMetricsData{};
        }
    }

    FunctionMetricsData& GetMetrics(FunctionType type) {
        return by_function_[FunctionTypeToIndex(type)];
    }

    const FunctionMetricsData& GetMetrics(FunctionType type) const noexcept {
        return by_function_[FunctionTypeToIndex(type)];
    }

    bool IsEmpty() const noexcept {
        for (const auto& func_metrics: by_function_) {
            if (!func_metrics.IsEmpty()) {
                return false;
            }
        }
        return true;
    }

private:
    FunctionMetricsData by_function_[NUM_FUNCTION_TYPES];
};

struct ThreadIdHash {
    size_t operator()(const std::thread::id& id) const noexcept {
        return std::hash<std::thread::id>{}(id);
    }
};

}// namespace flock
