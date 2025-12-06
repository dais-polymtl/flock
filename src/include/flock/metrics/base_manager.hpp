#pragma once

#include "flock/metrics/data_structures.hpp"
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <type_traits>
#include <unordered_map>

namespace flock {

// Core metrics tracking functionality shared between scalar and aggregate functions
template<typename StateId>
class BaseMetricsManager {
public:
    ThreadMetrics& GetThreadMetrics(const StateId& state_id) {
        const auto tid = std::this_thread::get_id();
        auto& thread_map = thread_metrics_[tid];

        auto it = thread_map.find(state_id);
        if (it != thread_map.end()) {
            return it->second;
        }

        return thread_map[state_id];
    }

    void RegisterThread(const StateId& state_id) {
        GetThreadMetrics(state_id);
    }

    // Initialize metrics tracking and assign registration order
    void StartInvocation(const StateId& state_id, FunctionType type) {
        RegisterThread(state_id);

        const auto tid = std::this_thread::get_id();
        ThreadFunctionKey thread_function_key{tid, type};

        if (thread_function_counters_.find(thread_function_key) == thread_function_counters_.end()) {
            thread_function_counters_[thread_function_key] = 0;
        }

        StateFunctionKey state_function_key{state_id, type};
        if (state_function_registration_order_.find(state_function_key) == state_function_registration_order_.end()) {
            thread_function_counters_[thread_function_key]++;
            state_function_registration_order_[state_function_key] = thread_function_counters_[thread_function_key];
        }

        GetThreadMetrics(state_id).GetMetrics(type);
    }

    // Store model name and provider (first call wins)
    void SetModelInfo(const StateId& state_id, FunctionType type, const std::string& model_name, const std::string& provider) {
        auto& thread_metrics = GetThreadMetrics(state_id);
        auto& metrics = thread_metrics.GetMetrics(type);
        if (metrics.model_name.empty()) {
            metrics.model_name = model_name;
        }
        if (metrics.provider.empty()) {
            metrics.provider = provider;
        }
    }

    // Add input and output tokens (accumulative)
    void UpdateTokens(const StateId& state_id, FunctionType type, int64_t input, int64_t output) {
        auto& thread_metrics = GetThreadMetrics(state_id);
        auto& metrics = thread_metrics.GetMetrics(type);
        metrics.input_tokens += input;
        metrics.output_tokens += output;
    }

    // Increment API call counter
    void IncrementApiCalls(const StateId& state_id, FunctionType type) {
        GetThreadMetrics(state_id).GetMetrics(type).api_calls++;
    }

    // Add API duration in microseconds (accumulative)
    void AddApiDuration(const StateId& state_id, FunctionType type, int64_t duration_us) {
        GetThreadMetrics(state_id).GetMetrics(type).api_duration_us += duration_us;
    }

    // Add execution time in microseconds (accumulative)
    void AddExecutionTime(const StateId& state_id, FunctionType type, int64_t duration_us) {
        GetThreadMetrics(state_id).GetMetrics(type).execution_time_us += duration_us;
    }

    // Get flattened metrics structure (merged across threads)
    nlohmann::json GetMetrics() const {
        nlohmann::json result = nlohmann::json::object();

        struct Key {
            FunctionType function_type;
            size_t registration_order;

            bool operator==(const Key& other) const {
                return function_type == other.function_type && registration_order == other.registration_order;
            }
        };

        struct KeyHash {
            size_t operator()(const Key& k) const {
                return std::hash<size_t>{}(static_cast<size_t>(k.function_type)) ^
                       (std::hash<size_t>{}(k.registration_order) << 1);
            }
        };

        std::unordered_map<Key, FunctionMetricsData, KeyHash> merged_metrics;

        // Collect and merge metrics by (function_type, registration_order)
        for (const auto& [tid, state_map]: thread_metrics_) {
            for (const auto& [state_id, thread_metrics]: state_map) {
                if (thread_metrics.IsEmpty()) {
                    continue;
                }

                for (size_t i = 0; i < ThreadMetrics::NUM_FUNCTION_TYPES - 1; ++i) {
                    const auto function_type = static_cast<FunctionType>(i);
                    const auto& metrics = thread_metrics.GetMetrics(function_type);

                    if (!metrics.IsEmpty()) {
                        StateFunctionKey state_function_key{state_id, function_type};
                        auto order_it = state_function_registration_order_.find(state_function_key);
                        size_t registration_order = (order_it != state_function_registration_order_.end())
                                                            ? order_it->second
                                                            : SIZE_MAX;

                        Key key{function_type, registration_order};

                        auto& merged = merged_metrics[key];
                        merged.input_tokens += metrics.input_tokens;
                        merged.output_tokens += metrics.output_tokens;
                        merged.api_calls += metrics.api_calls;
                        merged.api_duration_us += metrics.api_duration_us;
                        merged.execution_time_us += metrics.execution_time_us;

                        if (merged.model_name.empty() && !metrics.model_name.empty()) {
                            merged.model_name = metrics.model_name;
                        }
                        if (merged.provider.empty() && !metrics.provider.empty()) {
                            merged.provider = metrics.provider;
                        }
                    }
                }
            }
        }

        struct MetricEntry {
            FunctionType function_type;
            size_t registration_order;
            FunctionMetricsData metrics;
        };

        std::vector<MetricEntry> entries;
        entries.reserve(merged_metrics.size());

        for (const auto& [key, metrics]: merged_metrics) {
            entries.push_back({key.function_type, key.registration_order, metrics});
        }

        std::sort(entries.begin(), entries.end(), [](const MetricEntry& a, const MetricEntry& b) {
            if (a.function_type != b.function_type) {
                return a.function_type < b.function_type;
            }
            return a.registration_order < b.registration_order;
        });

        std::unordered_map<FunctionType, size_t> function_counters;

        for (const auto& entry: entries) {
            if (function_counters.find(entry.function_type) == function_counters.end()) {
                function_counters[entry.function_type] = 0;
            }

            function_counters[entry.function_type]++;
            const std::string key = std::string(FunctionTypeToString(entry.function_type)) + "_" + std::to_string(function_counters[entry.function_type]);

            result[key] = entry.metrics.ToJson();
        }

        return result;
    }

    // Get nested metrics structure preserving thread/state info (for debugging)
    nlohmann::json GetDebugMetrics() const {
        nlohmann::json result;
        nlohmann::json threads_json = nlohmann::json::object();

        size_t threads_with_output = 0;

        for (const auto& [tid, state_map]: thread_metrics_) {
            std::ostringstream oss;
            oss << tid;
            const std::string thread_id_str = oss.str();

            nlohmann::json thread_data;
            bool thread_has_output = false;

            for (const auto& [state_id, thread_metrics]: state_map) {
                if (thread_metrics.IsEmpty()) {
                    continue;
                }

                std::ostringstream state_oss;
                state_oss << state_id;
                const std::string state_id_str = state_oss.str();

                nlohmann::json state_data;

                for (size_t i = 0; i < ThreadMetrics::NUM_FUNCTION_TYPES - 1; ++i) {
                    const auto function_type = static_cast<FunctionType>(i);
                    const auto& metrics = thread_metrics.GetMetrics(function_type);

                    if (!metrics.IsEmpty()) {
                        StateFunctionKey state_function_key{state_id, function_type};
                        auto order_it = state_function_registration_order_.find(state_function_key);
                        size_t registration_order = (order_it != state_function_registration_order_.end())
                                                            ? order_it->second
                                                            : 0;

                        nlohmann::json function_data = metrics.ToJson();
                        function_data["registration_order"] = registration_order;
                        state_data[FunctionTypeToString(function_type)] = std::move(function_data);
                    }
                }

                if (!state_data.empty()) {
                    thread_has_output = true;
                    thread_data[state_id_str] = std::move(state_data);
                }
            }

            if (thread_has_output) {
                threads_with_output++;
                threads_json[thread_id_str] = std::move(thread_data);
            }
        }

        result["threads"] = threads_json.empty() ? nlohmann::json::object() : std::move(threads_json);
        result["thread_count"] = threads_with_output;
        return result;
    }

    // Clear all metrics and registration tracking
    void Reset() {
        thread_metrics_.clear();
        state_function_registration_order_.clear();
        thread_function_counters_.clear();
    }

protected:
    // Main storage: thread_id -> state_id -> ThreadMetrics
    std::unordered_map<std::thread::id, std::unordered_map<StateId, ThreadMetrics>, ThreadIdHash> thread_metrics_;

    // Registration order tracking structures
    struct ThreadFunctionKey {
        std::thread::id thread_id;
        FunctionType function_type;

        bool operator==(const ThreadFunctionKey& other) const {
            return thread_id == other.thread_id && function_type == other.function_type;
        }
    };

    struct ThreadFunctionKeyHash {
        size_t operator()(const ThreadFunctionKey& k) const {
            return ThreadIdHash{}(k.thread_id) ^
                   (std::hash<size_t>{}(static_cast<size_t>(k.function_type)) << 1);
        }
    };

    struct StateFunctionKey {
        StateId state_id;
        FunctionType function_type;

        bool operator==(const StateFunctionKey& other) const {
            return state_id == other.state_id && function_type == other.function_type;
        }
    };

    struct StateFunctionKeyHash {
        size_t operator()(const StateFunctionKey& k) const {
            size_t state_hash = 0;
            if constexpr (std::is_pointer_v<StateId>) {
                state_hash = std::hash<uintptr_t>{}(reinterpret_cast<uintptr_t>(k.state_id));
            } else {
                state_hash = std::hash<StateId>{}(k.state_id);
            }
            return state_hash ^ (std::hash<size_t>{}(static_cast<size_t>(k.function_type)) << 1);
        }
    };

    std::unordered_map<StateFunctionKey, size_t, StateFunctionKeyHash> state_function_registration_order_;
    std::unordered_map<ThreadFunctionKey, size_t, ThreadFunctionKeyHash> thread_function_counters_;
};

}// namespace flock
