#include "flockmtl/functions/scalar/llm_complete.hpp"
#include "flockmtl/core/config.hpp"
#include "flockmtl/model_manager/model.hpp"
#include "flockmtl/model_manager/providers/adapters/openai.hpp"
#include "nlohmann/json.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace flockmtl {

// Mock class for OpenAI API to avoid real API calls during tests
class MockOpenAIProvider : public OpenAIProvider {
public:
    explicit MockOpenAIProvider() : OpenAIProvider(ModelDetails()) {}

    // Override the API call methods for testing
    MOCK_METHOD(nlohmann::json, CallComplete, (const std::string& prompt, bool json_response), (override));
    MOCK_METHOD(nlohmann::json, CallEmbedding, (const std::vector<std::string>& inputs), (override));
};

class LLMCompleteTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto con = Config::GetConnection();
        con.Query(" CREATE SECRET ("
                  "       TYPE OPENAI,"
                  "    API_KEY 'your-api-key');");
    }

    void TearDown() override {
        Model::ResetMockProvider();
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
};

// Test llm_complete
TEST_F(LLMCompleteTest, LLMCompleteWithoutInputColumns) {

    auto mock_provider = std::make_shared<MockOpenAIProvider>();
    const std::string test_prompt = "Explain the purpose of FlockMTL.";
    const std::string expected_complete_response = "FlockMTL enhances DuckDB by integrating semantic functions and robust resource management capabilities, enabling advanced analytics and language model operations directly within SQL queries.";
    Model::SetMockProvider(mock_provider);

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(expected_complete_response));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT llm_complete({'model_name': 'gpt-4o'},{'prompt': 'Explain the purpose of FlockMTL.'}) AS flockmtl_purpose;");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), expected_complete_response);
}

TEST_F(LLMCompleteTest, LLMCompleteWithInputColumns) {
    auto mock_provider = std::make_shared<MockOpenAIProvider>();
    const std::string test_prompt = "What is the capital of Canada?";
    const nlohmann::json expected_complete_response = {{"tuples", {"The capital of Canada is Ottawa."}}};
    Model::SetMockProvider(mock_provider);

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(expected_complete_response));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT llm_complete({'model_name': 'gpt-4o'},{'prompt': 'What is the capital of France?'}, {'input': 'France'}) AS flockmtl_capital;");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), expected_complete_response["tuples"][0]);
}

TEST_F(LLMCompleteTest, ValidateArguments) {
    // Test 1: Valid case with 2 arguments (both structs)
    {
        duckdb::DataChunk chunk;

        // Create struct types for model_details and prompt_details
        duckdb::child_list_t<duckdb::LogicalType> model_fields = {
                {"provider", duckdb::LogicalType::VARCHAR},
                {"model", duckdb::LogicalType::VARCHAR}};
        duckdb::child_list_t<duckdb::LogicalType> prompt_fields = {
                {"text", duckdb::LogicalType::VARCHAR}};

        duckdb::LogicalType model_struct = duckdb::LogicalType::STRUCT(model_fields);
        duckdb::LogicalType prompt_struct = duckdb::LogicalType::STRUCT(prompt_fields);

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct});
        chunk.SetCardinality(1);

        // Should not throw
        EXPECT_NO_THROW(LlmComplete::ValidateArguments(chunk));
    }

    // Test 2: Valid case with 3 arguments (all structs)
    {
        duckdb::DataChunk chunk;

        duckdb::child_list_t<duckdb::LogicalType> model_fields = {
                {"provider", duckdb::LogicalType::VARCHAR}};
        duckdb::child_list_t<duckdb::LogicalType> prompt_fields = {
                {"text", duckdb::LogicalType::VARCHAR}};
        duckdb::child_list_t<duckdb::LogicalType> input_fields = {
                {"variables", duckdb::LogicalType::VARCHAR}};

        duckdb::LogicalType model_struct = duckdb::LogicalType::STRUCT(model_fields);
        duckdb::LogicalType prompt_struct = duckdb::LogicalType::STRUCT(prompt_fields);
        duckdb::LogicalType input_struct = duckdb::LogicalType::STRUCT(input_fields);

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(),
                         {model_struct, prompt_struct, input_struct});
        chunk.SetCardinality(1);

        // Should not throw
        EXPECT_NO_THROW(LlmComplete::ValidateArguments(chunk));
    }

    // Test 3: Invalid - too few arguments (1 argument)
    {
        duckdb::DataChunk chunk;

        duckdb::child_list_t<duckdb::LogicalType> fields = {
                {"field", duckdb::LogicalType::VARCHAR}};
        duckdb::LogicalType struct_type = duckdb::LogicalType::STRUCT(fields);

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {struct_type});
        chunk.SetCardinality(1);

        EXPECT_THROW(LlmComplete::ValidateArguments(chunk), std::runtime_error);
    }

    // Test 4: Invalid - too many arguments (4 arguments)
    {
        duckdb::DataChunk chunk;

        duckdb::child_list_t<duckdb::LogicalType> fields = {
                {"field", duckdb::LogicalType::VARCHAR}};
        duckdb::LogicalType struct_type = duckdb::LogicalType::STRUCT(fields);

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(),
                         {struct_type, struct_type, struct_type, struct_type});
        chunk.SetCardinality(1);

        EXPECT_THROW(LlmComplete::ValidateArguments(chunk), std::runtime_error);
    }

    // Test 5: Invalid - first argument is not a struct
    {
        duckdb::DataChunk chunk;

        duckdb::child_list_t<duckdb::LogicalType> fields = {
                {"field", duckdb::LogicalType::VARCHAR}};
        duckdb::LogicalType struct_type = duckdb::LogicalType::STRUCT(fields);

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(),
                         {duckdb::LogicalType::VARCHAR, struct_type});
        chunk.SetCardinality(1);

        EXPECT_THROW(LlmComplete::ValidateArguments(chunk), std::runtime_error);
    }

    // Test 6: Invalid - second argument is not a struct
    {
        duckdb::DataChunk chunk;

        duckdb::child_list_t<duckdb::LogicalType> fields = {
                {"field", duckdb::LogicalType::VARCHAR}};
        duckdb::LogicalType struct_type = duckdb::LogicalType::STRUCT(fields);

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(),
                         {struct_type, duckdb::LogicalType::INTEGER});
        chunk.SetCardinality(1);

        EXPECT_THROW(LlmComplete::ValidateArguments(chunk), std::runtime_error);
    }

    // Test 7: Invalid - third argument exists but is not a struct
    {
        duckdb::DataChunk chunk;

        duckdb::child_list_t<duckdb::LogicalType> fields = {
                {"field", duckdb::LogicalType::VARCHAR}};
        duckdb::LogicalType struct_type = duckdb::LogicalType::STRUCT(fields);

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(),
                         {struct_type, struct_type, duckdb::LogicalType::BOOLEAN});
        chunk.SetCardinality(1);

        EXPECT_THROW(LlmComplete::ValidateArguments(chunk), std::runtime_error);
    }
}

TEST_F(LLMCompleteTest, Operation_TwoArguments_SimplePrompt) {

    auto mock_provider = std::make_shared<MockOpenAIProvider>();
    const std::string test_prompt = "Explain the purpose of FlockMTL.";
    const std::string expected_complete_response = "FlockMTL enhances DuckDB by integrating semantic functions and robust resource management capabilities, enabling advanced analytics and language model operations directly within SQL queries.";
    Model::SetMockProvider(mock_provider);

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(expected_complete_response));

    duckdb::DataChunk chunk;

    duckdb::child_list_t<duckdb::LogicalType> model_fields = {
            {"model_name", duckdb::LogicalType::VARCHAR}};

    duckdb::child_list_t<duckdb::LogicalType> prompt_fields = {
            {"prompt", duckdb::LogicalType::VARCHAR},
    };

    duckdb::LogicalType model_struct = duckdb::LogicalType::STRUCT(model_fields);
    duckdb::LogicalType prompt_struct = duckdb::LogicalType::STRUCT(prompt_fields);

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct});
    chunk.SetCardinality(1);

    // Set model details data
    SetStructStringData(chunk.data[0], {{{"model_name", "gpt-4o"}}});

    // Set prompt details data
    SetStructStringData(chunk.data[1], {{{"prompt", test_prompt}}});

    auto results = LlmComplete::Operation(chunk);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], expected_complete_response);
}

TEST_F(LLMCompleteTest, Operation_ThreeArguments_BatchProcessing) {

    auto mock_provider = std::make_shared<MockOpenAIProvider>();
    const std::string test_prompt = "Explain the purpose of FlockMTL.";
    const nlohmann::json expected_complete_response = {{"tuples", {"response 1", "response 2"}}};

    Model::SetMockProvider(mock_provider);

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(expected_complete_response));

    duckdb::DataChunk chunk;

    duckdb::child_list_t<duckdb::LogicalType> model_fields = {
            {"model_name", duckdb::LogicalType::VARCHAR}};

    duckdb::child_list_t<duckdb::LogicalType> prompt_fields = {
            {"prompt", duckdb::LogicalType::VARCHAR},
    };

    // Create inputs struct
    duckdb::child_list_t<duckdb::LogicalType> input_fields = {
            {"variable1", duckdb::LogicalType::VARCHAR},
            {"variable2", duckdb::LogicalType::VARCHAR}};

    duckdb::LogicalType model_struct = duckdb::LogicalType::STRUCT(model_fields);
    duckdb::LogicalType prompt_struct = duckdb::LogicalType::STRUCT(prompt_fields);
    duckdb::LogicalType input_struct = duckdb::LogicalType::STRUCT(input_fields);

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(),
                     {model_struct, prompt_struct, input_struct});
    chunk.SetCardinality(2);// Two rows of input data

    // Set model details data
    SetStructStringData(chunk.data[0], {{{"model_name", "gpt-4o"}}});

    // Set prompt details data
    SetStructStringData(chunk.data[1], {{{"prompt", test_prompt}}});

    // Set input data (different for each row)
    SetStructStringData(chunk.data[2], {{{"variable1", "Hello"},
                                         {"variable2", "World"}},
                                        {{"variable1", "Good"},
                                         {"variable2", "Morning"}}});

    auto results = LlmComplete::Operation(chunk);

    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results, expected_complete_response["tuples"]);
}

TEST_F(LLMCompleteTest, Operation_InvalidArguments_ThrowsException) {

    duckdb::DataChunk chunk;

    // Create invalid chunk with only one argument
    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {duckdb::LogicalType::VARCHAR});
    chunk.SetCardinality(1);

    EXPECT_THROW(LlmComplete::Operation(chunk), std::runtime_error);
}

TEST_F(LLMCompleteTest, Operation_EmptyPrompt_HandlesGracefully) {

    duckdb::DataChunk chunk;

    duckdb::child_list_t<duckdb::LogicalType> model_fields = {
            {"model_name", duckdb::LogicalType::VARCHAR}};

    duckdb::child_list_t<duckdb::LogicalType> prompt_fields = {
            {"prompt", duckdb::LogicalType::VARCHAR}};

    duckdb::LogicalType model_struct = duckdb::LogicalType::STRUCT(model_fields);
    duckdb::LogicalType prompt_struct = duckdb::LogicalType::STRUCT(prompt_fields);

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct});
    chunk.SetCardinality(1);

    SetStructStringData(chunk.data[0], {{{"model_name", "gpt-4o"}}});

    SetStructStringData(chunk.data[1], {{{"prompt", ""}}});

    EXPECT_THROW(
            LlmComplete::Operation(chunk),
            std::runtime_error);
}

TEST_F(LLMCompleteTest, Operation_LargeInputSet_ProcessesCorrectly) {

    constexpr size_t input_count = 100;// Test with larger dataset

    auto mock_provider = std::make_shared<MockOpenAIProvider>();
    nlohmann::json expected_complete_response = {{"tuples", {}}};
    for (size_t i = 0; i < input_count; i++) {
        expected_complete_response["tuples"].push_back("response " + std::to_string(i));
    }

    Model::SetMockProvider(mock_provider);

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(expected_complete_response));

    duckdb::DataChunk chunk;

    duckdb::child_list_t<duckdb::LogicalType> model_fields = {
            {"model_name", duckdb::LogicalType::VARCHAR}};

    duckdb::child_list_t<duckdb::LogicalType> prompt_fields = {
            {"prompt", duckdb::LogicalType::VARCHAR}};

    duckdb::child_list_t<duckdb::LogicalType> input_fields = {
            {"text", duckdb::LogicalType::VARCHAR}};

    duckdb::LogicalType model_struct = duckdb::LogicalType::STRUCT(model_fields);
    duckdb::LogicalType prompt_struct = duckdb::LogicalType::STRUCT(prompt_fields);
    duckdb::LogicalType input_struct = duckdb::LogicalType::STRUCT(input_fields);

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(),
                     {model_struct, prompt_struct, input_struct});
    chunk.SetCardinality(input_count);

    // Set model and prompt data
    SetStructStringData(chunk.data[0], {{{"model_name", "gpt-4o"}}});

    SetStructStringData(chunk.data[1], {{{"prompt", "Summarize the following text"}}});

    // Create large input dataset
    std::vector<std::map<std::string, std::string>> large_input;
    large_input.reserve(input_count);
    for (size_t i = 0; i < input_count; i++) {
        large_input.push_back({{"text", "Input text " + std::to_string(i)}});
    }

    SetStructStringData(chunk.data[2], large_input);

    const auto results = LlmComplete::Operation(chunk);

    EXPECT_EQ(results.size(), input_count);
    EXPECT_EQ(results, expected_complete_response["tuples"]);
}

}// namespace flockmtl