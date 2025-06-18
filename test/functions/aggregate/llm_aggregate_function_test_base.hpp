#pragma once

#include "../mock_provider.hpp"
#include "flockmtl/core/config.hpp"
#include "flockmtl/functions/aggregate/aggregate.hpp"
#include "flockmtl/model_manager/model.hpp"
#include "flockmtl/model_manager/providers/adapters/openai.hpp"
#include "flockmtl/prompt_manager/repository.hpp"
#include "nlohmann/json.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace flockmtl {

// Base template class for LLM aggregate function tests
template<typename FunctionClass>
class LLMAggregateTestBase : public ::testing::Test {
protected:
    // Common test constants
    static constexpr const char* DEFAULT_MODEL = "gpt-4o";
    static constexpr const char* TEST_PROMPT = "Summarize the following data";

    std::shared_ptr<MockOpenAIProvider> mock_provider;

    void SetUp() override {
        auto con = Config::GetConnection();
        con.Query(" CREATE SECRET ("
                  "       TYPE OPENAI,"
                  "    API_KEY 'your-api-key');");

        mock_provider = std::make_shared<MockOpenAIProvider>();
        Model::SetMockProvider(mock_provider);
    }

    void TearDown() override {
        Model::ResetMockProvider();
        mock_provider = nullptr;
    }

    // Helper to create struct types with common patterns
    static duckdb::LogicalType CreateModelStruct() {
        duckdb::child_list_t<duckdb::LogicalType> fields = {
                {"model_name", duckdb::LogicalType::VARCHAR}};
        return duckdb::LogicalType::STRUCT(fields);
    }

    static duckdb::LogicalType CreatePromptStruct() {
        duckdb::child_list_t<duckdb::LogicalType> fields = {
                {"prompt", duckdb::LogicalType::VARCHAR}};
        return duckdb::LogicalType::STRUCT(fields);
    }

    static duckdb::LogicalType CreateInputStruct(const std::vector<std::string>& field_names) {
        duckdb::child_list_t<duckdb::LogicalType> fields;
        for (const auto& name: field_names) {
            fields.emplace_back(name, duckdb::LogicalType::VARCHAR);
        }
        return duckdb::LogicalType::STRUCT(fields);
    }

    // Generic helper to create struct types for validation tests
    static duckdb::LogicalType CreateGenericStruct(const std::vector<std::pair<std::string, duckdb::LogicalType>>& fields) {
        duckdb::child_list_t<duckdb::LogicalType> child_fields;
        for (const auto& [name, type]: fields) {
            child_fields.emplace_back(name, type);
        }
        return duckdb::LogicalType::STRUCT(child_fields);
    }

    static void SetStructStringData(duckdb::Vector& struct_vector, const std::vector<std::map<std::string, std::string>>& data) {
        auto& child_vectors = duckdb::StructVector::GetEntries(struct_vector);

        for (size_t row = 0; row < data.size(); row++) {
            for (const auto& [field_name, field_value]: data[row]) {
                // Find the correct child vector for this field
                const auto& struct_type = struct_vector.GetType();
                auto& child_types = duckdb::StructType::GetChildTypes(struct_type);

                for (size_t i = 0; i < child_types.size(); i++) {
                    if (child_types[i].first == field_name) {
                        const auto string_data = duckdb::FlatVector::GetData<duckdb::string_t>(*child_vectors[i]);
                        string_data[row] = duckdb::StringVector::AddString(*child_vectors[i], field_value);
                        break;
                    }
                }
            }
        }
    }

    // Helper to setup a basic 3-argument chunk for aggregate function testing
    void CreateBasicAggregateChunk(duckdb::DataChunk& chunk, size_t cardinality = 3) {
        auto model_struct = CreateModelStruct();
        auto prompt_struct = CreatePromptStruct();
        auto input_struct = CreateInputStruct({"data"});

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
        chunk.SetCardinality(cardinality);
    }

    // Common test methods for aggregate functions
    void TestValidateArguments() {
        // Test valid case with 3 arguments (all structs) - aggregate functions require 3 arguments
        {
            duckdb::DataChunk chunk;
            auto model_struct = CreateGenericStruct({{"provider", duckdb::LogicalType::VARCHAR}, {"model", duckdb::LogicalType::VARCHAR}});
            auto prompt_struct = CreateGenericStruct({{"text", duckdb::LogicalType::VARCHAR}});
            auto input_struct = CreateGenericStruct({{"variables", duckdb::LogicalType::VARCHAR}});

            chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
            chunk.SetCardinality(1);

            duckdb::Vector inputs[] = {chunk.data[0], chunk.data[1], chunk.data[2]};
            EXPECT_NO_THROW(AggregateFunctionBase::ValidateArguments(inputs, 3));
        }

        // Test invalid cases
        struct InvalidTestCase {
            std::string description;
            duckdb::vector<duckdb::LogicalType> types;
            size_t input_count;
        };

        std::vector<InvalidTestCase> invalid_cases = {
                {"too few arguments (2 arguments)", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}})}, 2},
                {"too many arguments (4 arguments)", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}})}, 4},
                {"first argument is not a struct", {duckdb::LogicalType::VARCHAR, CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}})}, 3},
                {"second argument is not a struct", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), duckdb::LogicalType::INTEGER, CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}})}, 3},
                {"third argument is not a struct", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), duckdb::LogicalType::BOOLEAN}, 3}};

        for (const auto& test_case: invalid_cases) {
            SCOPED_TRACE("Testing: " + test_case.description);
            duckdb::DataChunk chunk;
            chunk.Initialize(duckdb::Allocator::DefaultAllocator(), test_case.types);
            chunk.SetCardinality(1);

            // Create a temporary array to pass the correct number of input vectors
            if (test_case.input_count == 2) {
                duckdb::Vector inputs[] = {chunk.data[0], chunk.data[1]};
                EXPECT_THROW(AggregateFunctionBase::ValidateArguments(inputs, test_case.input_count), std::runtime_error);
            } else if (test_case.input_count == 3) {
                duckdb::Vector inputs[] = {chunk.data[0], chunk.data[1], chunk.data[2]};
                EXPECT_THROW(AggregateFunctionBase::ValidateArguments(inputs, test_case.input_count), std::runtime_error);
            } else if (test_case.input_count == 4) {
                duckdb::Vector inputs[] = {chunk.data[0], chunk.data[1], chunk.data[2], chunk.data[3]};
                EXPECT_THROW(AggregateFunctionBase::ValidateArguments(inputs, test_case.input_count), std::runtime_error);
            }
        }
    }

    void TestOperationInvalidArguments() {
        duckdb::DataChunk chunk;
        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {duckdb::LogicalType::VARCHAR});
        chunk.SetCardinality(1);

        duckdb::Vector inputs[] = {chunk.data[0]};
        EXPECT_THROW(AggregateFunctionBase::ValidateArguments(inputs, 1), std::runtime_error);
    }

    // Virtual methods for function-specific logic
    virtual std::string GetExpectedResponse() const = 0;
    virtual nlohmann::json GetExpectedJsonResponse() const = 0;
    virtual std::string GetFunctionName() const = 0;
    virtual AggregateFunctionType GetFunctionType() const = 0;
    virtual nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const = 0;
    virtual nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const = 0;
    virtual std::string FormatExpectedResult(const nlohmann::json& response) const = 0;
};

}// namespace flockmtl
