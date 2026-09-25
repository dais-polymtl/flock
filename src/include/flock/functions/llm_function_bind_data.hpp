#pragma once

#include "flock/core/common.hpp"
#include "flock/model_manager/model.hpp"

namespace flock {

struct LlmFunctionBindData : public duckdb::FunctionData {
    nlohmann::json model_json;// Store model JSON to create fresh Model instances per call
    std::string prompt;

    LlmFunctionBindData() = default;

    // Create a fresh Model instance (thread-safe, each call gets its own provider)
    Model CreateModel() const {
        return Model(model_json);
    }

    duckdb::unique_ptr<duckdb::FunctionData> Copy() const override {
        auto result = duckdb::make_uniq<LlmFunctionBindData>();
        result->model_json = model_json;
        result->prompt = prompt;
        return std::move(result);
    }

    bool Equals(const duckdb::FunctionData& other) const override {
        auto& other_bind = other.Cast<LlmFunctionBindData>();
        return prompt == other_bind.prompt && model_json == other_bind.model_json;
    }
};

}// namespace flock
