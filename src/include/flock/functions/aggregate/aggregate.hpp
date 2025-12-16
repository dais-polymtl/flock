#pragma once

#include "flock/core/common.hpp"
#include "flock/functions/input_parser.hpp"
#include "flock/functions/llm_function_bind_data.hpp"
#include "flock/metrics/manager.hpp"
#include "flock/model_manager/model.hpp"
#include <nlohmann/json.hpp>
#include <optional>

namespace flock {

class AggregateFunctionState {
public:
    nlohmann::basic_json<>* value;
    bool initialized;

    AggregateFunctionState() : value(nullptr), initialized(false) {}

    ~AggregateFunctionState() {
        if (value) {
            delete value;
            value = nullptr;
        }
    }

    void Initialize();
    void Update(const nlohmann::json& input);
    void Combine(const AggregateFunctionState& source);
    void Destroy();
};

class AggregateFunctionBase {
public:
    Model model;
    std::string user_query;

public:
    explicit AggregateFunctionBase() = default;

private:
    struct PromptStructInfo {
        bool has_context_columns;
        std::optional<idx_t> prompt_field_index;
        std::string prompt_field_name;
    };

    static void ValidateArgumentCount(const duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
                                      const std::string& function_name);

    static void ValidateArgumentTypes(const duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
                                      const std::string& function_name);

    static PromptStructInfo ExtractPromptStructInfo(const duckdb::LogicalType& prompt_type);

    static void ValidatePromptStructFields(const PromptStructInfo& info, const std::string& function_name);

    static void InitializeModelJson(duckdb::ClientContext& context,
                                    const duckdb::unique_ptr<duckdb::Expression>& model_expr,
                                    LlmFunctionBindData& bind_data);

    static void InitializePrompt(duckdb::ClientContext& context,
                                 const duckdb::unique_ptr<duckdb::Expression>& prompt_expr,
                                 LlmFunctionBindData& bind_data);

public:
    static std::tuple<nlohmann::json, nlohmann::json>
    CastInputsToJson(duckdb::Vector inputs[], idx_t count);

    static duckdb::unique_ptr<LlmFunctionBindData> ValidateAndInitializeBindData(
            duckdb::ClientContext& context,
            duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments,
            const std::string& function_name);

    static bool IgnoreNull() { return true; };


    template<class Derived>
    static void Initialize(const duckdb::AggregateFunction&, duckdb::data_ptr_t state_p) {
        auto state = reinterpret_cast<AggregateFunctionState*>(state_p);
        new (state) AggregateFunctionState();
        state->Initialize();
    }

    template<class Derived>
    static void Operation(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                          duckdb::Vector& states, idx_t count) {
        auto [prompt_details, columns] = CastInputsToJson(inputs, count);

        auto state_map_p = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

        for (idx_t i = 0; i < count; i++) {
            auto state = state_map_p[i];
            auto tuple = nlohmann::json::array();
            auto idx = 0u;
            for (const auto& column: columns) {
                tuple.push_back(nlohmann::json::object());
                for (const auto& item: column.items()) {
                    if (item.key() == "data") {
                        tuple[idx][item.key()].push_back(item.value()[i]);
                    } else {
                        tuple[idx][item.key()] = item.value();
                    }
                }
                idx++;
            }

            if (state) {
                state->Update(tuple);
            }
        }
    }

    template<class Derived>
    static void SimpleUpdate(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                             duckdb::data_ptr_t state_p, idx_t count) {
        auto [prompt_details, tuples] = CastInputsToJson(inputs, count);

        if (const auto state = reinterpret_cast<AggregateFunctionState*>(state_p)) {
            state->Update(tuples);
        }
    }

    template<class Derived>
    static void Combine(duckdb::Vector& source, duckdb::Vector& target, duckdb::AggregateInputData& aggr_input_data,
                        const idx_t count) {
        const auto source_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(source));
        const auto target_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(target));

        for (auto i = 0; i < static_cast<int>(count); i++) {
            auto* source_state = source_vector[i];
            auto* target_state = target_vector[i];

            if (!source_state || !target_state) {
                continue;
            }

            target_state->Combine(*source_state);
        }
    }

    template<class Derived>
    static void Destroy(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, idx_t count) {
        auto state_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

        for (idx_t i = 0; i < count; i++) {
            auto* state = state_vector[i];
            if (state) {
                state->Destroy();
            }
        }
    }

    static void Finalize(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                         idx_t count, idx_t offset);

    template<class Derived>
    static void FinalizeSafe(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                             idx_t count, idx_t offset) {
        for (idx_t i = 0; i < count; i++) {
            auto result_idx = i + offset;
            result.SetValue(result_idx, "[]");
        }
    }
};

}// namespace flock
