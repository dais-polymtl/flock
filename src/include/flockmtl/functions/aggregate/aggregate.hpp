#pragma once

#include <nlohmann/json.hpp>
#include <tuple>

#include "flockmtl/core/common.hpp"
#include "flockmtl/functions/batch_response_builder.hpp"
#include "flockmtl/model_manager/model.hpp"

namespace flockmtl {

class AggregateFunctionState {
public:
    std::vector<nlohmann::json>* value;
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
    explicit AggregateFunctionBase() : model(std::move(Model())), user_query("") {};

public:
    static void ValidateArguments(duckdb::Vector inputs[], idx_t input_count);
    static std::tuple<nlohmann::json, nlohmann::json, std::vector<nlohmann::json>>
    CastInputsToJson(duckdb::Vector inputs[], idx_t count);

    static bool IgnoreNull() { return true; };

    template<class Derived>
    static void Initialize(const duckdb::AggregateFunction&, duckdb::data_ptr_t state_p) {
        auto state = reinterpret_cast<AggregateFunctionState*>(state_p);

        // Use placement new to properly construct the AggregateFunctionState object
        new (state) AggregateFunctionState();

        if (!state->initialized) {
            state->Initialize();
            state->initialized = true;
        }
    }

    template<class Derived>
    static void Operation(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                          duckdb::Vector& states, idx_t count) {
        ValidateArguments(inputs, input_count);

        auto [model_details, prompt_details, tuples] = CastInputsToJson(inputs, count);
        auto function_instance = GetInstance<Derived>();
        function_instance->model = Model(model_details);
        function_instance->user_query = PromptManager::CreatePromptDetails(prompt_details).prompt;

        auto state_map_p = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));

        for (idx_t i = 0; i < count; i++) {
            auto tuple = tuples[i];
            auto state = state_map_p[i];

            if (state) {
                state->Update(tuple);
            }
        }
    }

    template<class Derived>
    static void SimpleUpdate(duckdb::Vector inputs[], duckdb::AggregateInputData& aggr_input_data, idx_t input_count,
                             duckdb::data_ptr_t state_p, idx_t count) {
        ValidateArguments(inputs, input_count);

        auto [model_details, prompt_details, tuples] = CastInputsToJson(inputs, count);
        auto function_instance = GetInstance<Derived>();
        function_instance->model = Model(model_details);
        function_instance->user_query = PromptManager::CreatePromptDetails(prompt_details).prompt;

        auto state = reinterpret_cast<AggregateFunctionState*>(state_p);

        for (idx_t i = 0; i < count; i++) {
            auto tuple = tuples[i];

            if (state) {
                state->Update(tuple);
            }
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
                state->~AggregateFunctionState();// Explicitly call destructor
            }
        }
    }

    static void Finalize(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                         idx_t count, idx_t offset);

    template<class Derived>
    static void FinalizeSafe(duckdb::Vector& states, duckdb::AggregateInputData& aggr_input_data, duckdb::Vector& result,
                             idx_t count, idx_t offset) {
        const auto states_vector = reinterpret_cast<AggregateFunctionState**>(duckdb::FlatVector::GetData<duckdb::data_ptr_t>(states));
        auto function_instance = GetInstance<Derived>();

        for (idx_t i = 0; i < count; i++) {
            auto idx = i + offset;
            auto* state = states_vector[idx];

            if (state && state->value) {
                // Call the derived class's processing method
                auto processed_result = static_cast<Derived*>(function_instance)->ProcessStateForFinalize(*state);
                result.SetValue(idx, processed_result);
            } else {
                result.SetValue(idx, "[]");// Empty JSON array as default
            }
        }
    }

    // Static method to get or create an instance of the derived class
    template<typename Derived>
    static Derived* GetInstance() {
        if (instance == nullptr) {
            instance = new Derived();
        }
        return static_cast<Derived*>(instance);
    }

private:
    static AggregateFunctionBase* instance;
};

}// namespace flockmtl
