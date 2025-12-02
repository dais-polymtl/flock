#pragma once

#include "duckdb/main/database.hpp"
#include "flock/metrics/types.hpp"

namespace flock {

// Thread-local storage for metrics context (legacy, not used in function code)
class MetricsContext {
public:
    static void SetWithDatabase(duckdb::DatabaseInstance* db, const void* state_id, FunctionType type) noexcept {
        current_database_ = db;
        current_state_id_ = state_id;
        current_function_ = type;
    }

    static void Clear() noexcept {
        current_database_ = nullptr;
        current_state_id_ = nullptr;
        current_function_ = FunctionType::UNKNOWN;
    }

    static duckdb::DatabaseInstance* GetDatabase() noexcept {
        return current_database_;
    }

    static const void* GetStateId() noexcept {
        return current_state_id_;
    }

    static FunctionType GetFunctionType() noexcept {
        return current_function_;
    }

    static bool IsActive() noexcept {
        return current_database_ != nullptr && current_state_id_ != nullptr && current_function_ != FunctionType::UNKNOWN;
    }

private:
    static thread_local duckdb::DatabaseInstance* current_database_;
    static thread_local const void* current_state_id_;
    static thread_local FunctionType current_function_;
};

}// namespace flock
