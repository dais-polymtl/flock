#pragma once

#include "flock/core/common.hpp"
#include "flock/functions/input_parser.hpp"
#include "flock/metrics/manager.hpp"
#include "flock/model_manager/model.hpp"
#include <nlohmann/json.hpp>

namespace flock {

class AggregateFunctionState {
public:
    nlohmann::basic_json<>* value;
    bool initialized;
    nlohmann::json model_details;
    std::string user_query;

    AggregateFunctionState() : value(nullptr), initialized(false), model_details(nlohmann::json::object()), user_query("") {}

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
    nlohmann::json model_details;

public:
    explicit AggregateFunctionBase() = default;

public:
    static void ValidateArguments(duckdb::Vector inputs[], idx_t input_count);
    static std::tuple<nlohmann::json, nlohmann::json, nlohmann::json>
    CastInputsToJson(duckdb::Vector inputs[], idx_t count);

    static bool IgnoreNull() { return true; };

    template<class Derived>
    static void Initialize(const duckdb::AggregateFunction&, duckdb::data_ptr_t state_p) {
        auto state = reinterpret_cast<AggregateFunctionState*>(state_p);

        // Use placement new to properly construct the AggregateFunctionState object
        // This handles memory allocation done by DuckDB
        new (state) AggregateFunctionState();

        // Initialize the state (allocates JSON array, resets all fields)
        state->Initialize();
    }

    template<class Derived>
    static void Operation(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                          duckdb::Vector& states, idx_t count) {
        // ValidateArguments(inputs, input_count);

        auto [model_details_json, prompt_details, columns] = CastInputsToJson(inputs, count);
        auto prompt_str = PromptManager::CreatePromptDetails(prompt_details).prompt;

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
                // Store model_details and user_query in the state (only set once, on first update)
                if (state->model_details.empty()) {
                    state->model_details = model_details_json;
                    state->user_query = prompt_str;
                }
                state->Update(tuple);
            }
        }
    }

    template<class Derived>
    static void SimpleUpdate(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                             duckdb::data_ptr_t state_p, idx_t count) {
        // ValidateArguments(inputs, input_count);

        auto [model_details_json, prompt_details, tuples] = CastInputsToJson(inputs, count);
        auto prompt_str = PromptManager::CreatePromptDetails(prompt_details).prompt;

        if (const auto state = reinterpret_cast<AggregateFunctionState*>(state_p)) {
            // Store model_details and user_query in the state (only set once, on first update)
            if (state->model_details.empty()) {
                state->model_details = model_details_json;
                state->user_query = prompt_str;
            }
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
