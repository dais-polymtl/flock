#pragma once

#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/database.hpp"
#include "flock/metrics/base_manager.hpp"
#include "flock/metrics/types.hpp"
#include <atomic>
#include <memory>
#include <unordered_map>

namespace flock {

// Database-level metrics storage and unified API for scalar and aggregate functions
class MetricsManager : public BaseMetricsManager<const void*> {
public:
    // Get metrics manager for a database instance (creates if needed)
    static MetricsManager& GetForDatabase(duckdb::DatabaseInstance* db) {
        if (db == nullptr) {
            throw std::runtime_error("Database instance is null");
        }

        static std::unordered_map<duckdb::DatabaseInstance*, std::unique_ptr<MetricsManager>> db_managers;

        auto it = db_managers.find(db);
        if (it == db_managers.end()) {
            auto manager = std::make_unique<MetricsManager>();
            auto* manager_ptr = manager.get();
            db_managers[db] = std::move(manager);
            return *manager_ptr;
        }
        return *it->second;
    }

    // Generate a unique invocation ID for scalar functions
    static const void* GenerateUniqueId() {
        static std::atomic<uint64_t> counter{0};
        return reinterpret_cast<const void*>(++counter);
    }

    // Initialize metrics tracking (stores context for subsequent calls)
    static void StartInvocation(duckdb::DatabaseInstance* db, const void* state_id, FunctionType type) {
        if (db != nullptr && state_id != nullptr) {
            current_db_ = db;
            current_state_id_ = state_id;
            current_function_type_ = type;

            auto& manager = GetForDatabase(db);
            manager.RegisterThread(state_id);
            manager.BaseMetricsManager<const void*>::StartInvocation(state_id, type);
        }
    }

    // Record model name and provider
    static void SetModelInfo(const std::string& model_name, const std::string& provider) {
        if (current_db_ != nullptr && current_state_id_ != nullptr) {
            auto& manager = GetForDatabase(current_db_);
            manager.BaseMetricsManager<const void*>::SetModelInfo(current_state_id_, current_function_type_, model_name, provider);
        }
    }

    // Record token usage (accumulative)
    static void UpdateTokens(int64_t input, int64_t output) {
        if (current_db_ != nullptr && current_state_id_ != nullptr) {
            auto& manager = GetForDatabase(current_db_);
            manager.BaseMetricsManager<const void*>::UpdateTokens(current_state_id_, current_function_type_, input, output);
        }
    }

    // Increment API call counter
    static void IncrementApiCalls() {
        if (current_db_ != nullptr && current_state_id_ != nullptr) {
            auto& manager = GetForDatabase(current_db_);
            manager.BaseMetricsManager<const void*>::IncrementApiCalls(current_state_id_, current_function_type_);
        }
    }

    // Record API call duration in milliseconds (accumulative)
    static void AddApiDuration(double duration_ms) {
        if (current_db_ != nullptr && current_state_id_ != nullptr) {
            const int64_t duration_us = static_cast<int64_t>(duration_ms * 1000.0);
            auto& manager = GetForDatabase(current_db_);
            manager.BaseMetricsManager<const void*>::AddApiDuration(current_state_id_, current_function_type_, duration_us);
        }
    }

    // Record execution time in milliseconds (accumulative)
    static void AddExecutionTime(double duration_ms) {
        if (current_db_ != nullptr && current_state_id_ != nullptr) {
            const int64_t duration_us = static_cast<int64_t>(duration_ms * 1000.0);
            auto& manager = GetForDatabase(current_db_);
            manager.BaseMetricsManager<const void*>::AddExecutionTime(current_state_id_, current_function_type_, duration_us);
        }
    }

    // Clear stored context (optional, auto-cleared on next StartInvocation)
    static void ClearContext() {
        current_db_ = nullptr;
        current_state_id_ = nullptr;
        current_function_type_ = FunctionType::UNKNOWN;
    }

    // Merge metrics from multiple states into a single state
    // This is used by aggregate functions to consolidate metrics from all processed states
    static void MergeAggregateMetrics(duckdb::DatabaseInstance* db,
                                      const std::vector<const void*>& processed_state_ids,
                                      FunctionType function_type,
                                      const std::string& model_name = "",
                                      const std::string& provider = "");

    // SQL function implementations
    static void ExecuteGetMetrics(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
    static void ExecuteGetDebugMetrics(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
    static void ExecuteResetMetrics(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);

private:
    // Thread-local storage for current metrics context
    static thread_local duckdb::DatabaseInstance* current_db_;
    static thread_local const void* current_state_id_;
    static thread_local FunctionType current_function_type_;
};

}// namespace flock
