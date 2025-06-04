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
    // Common test constants
    static constexpr const char* DEFAULT_MODEL = "gpt-4o";
    static constexpr const char* TEST_PROMPT = "Explain the purpose of FlockMTL.";
    static constexpr const char* EXPECTED_RESPONSE = "FlockMTL enhances DuckDB by integrating semantic functions and robust resource management capabilities, enabling advanced analytics and language model operations directly within SQL queries.";

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

    // Helper to setup a basic 2-argument chunk for testing
    void CreateBasicChunk(duckdb::DataChunk& chunk, size_t cardinality = 1) {
        auto model_struct = CreateModelStruct();
        auto prompt_struct = CreatePromptStruct();

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct});
        chunk.SetCardinality(cardinality);
    }
};

// Test llm_complete with SQL queries
TEST_F(LLMCompleteTest, LLMCompleteWithoutInputColumns) {
    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(EXPECTED_RESPONSE));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT llm_complete({'model_name': 'gpt-4o'},{'prompt': 'Explain the purpose of FlockMTL.'}) AS flockmtl_purpose;");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), EXPECTED_RESPONSE);
}

TEST_F(LLMCompleteTest, LLMCompleteWithInputColumns) {
    const nlohmann::json expected_response = {{"tuples", {"The capital of Canada is Ottawa."}}};
    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(expected_response));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT llm_complete({'model_name': 'gpt-4o'},{'prompt': 'What is the capital of France?'}, {'input': 'France'}) AS flockmtl_capital;");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), expected_response["tuples"][0]);
}

TEST_F(LLMCompleteTest, ValidateArguments) {
    // Test valid cases
    {
        // Valid case with 2 arguments (both structs)
        duckdb::DataChunk chunk;
        auto model_struct = CreateGenericStruct({{"provider", duckdb::LogicalType::VARCHAR}, {"model", duckdb::LogicalType::VARCHAR}});
        auto prompt_struct = CreateGenericStruct({{"text", duckdb::LogicalType::VARCHAR}});

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct});
        chunk.SetCardinality(1);

        EXPECT_NO_THROW(LlmComplete::ValidateArguments(chunk));
    }

    {
        // Valid case with 3 arguments (all structs)
        duckdb::DataChunk chunk;
        auto model_struct = CreateGenericStruct({{"provider", duckdb::LogicalType::VARCHAR}});
        auto prompt_struct = CreateGenericStruct({{"text", duckdb::LogicalType::VARCHAR}});
        auto input_struct = CreateGenericStruct({{"variables", duckdb::LogicalType::VARCHAR}});

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
        chunk.SetCardinality(1);

        EXPECT_NO_THROW(LlmComplete::ValidateArguments(chunk));
    }

    // Test invalid cases - use a parameterized approach to reduce redundancy
    struct InvalidTestCase {
        std::string description;
        duckdb::vector<duckdb::LogicalType> types;
    };

    std::vector<InvalidTestCase> invalid_cases = {
            {"too few arguments (1 argument)", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}})}},
            {"too many arguments (4 arguments)", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}})}},
            {"first argument is not a struct", {duckdb::LogicalType::VARCHAR, CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}})}},
            {"second argument is not a struct", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), duckdb::LogicalType::INTEGER}},
            {"third argument exists but is not a struct", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), duckdb::LogicalType::BOOLEAN}}};

    for (const auto& test_case: invalid_cases) {
        SCOPED_TRACE("Testing: " + test_case.description);
        duckdb::DataChunk chunk;
        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), test_case.types);
        chunk.SetCardinality(1);

        EXPECT_THROW(LlmComplete::ValidateArguments(chunk), std::runtime_error);
    }
}

TEST_F(LLMCompleteTest, Operation_TwoArguments_SimplePrompt) {
    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(EXPECTED_RESPONSE));

    duckdb::DataChunk chunk;
    CreateBasicChunk(chunk);

    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", TEST_PROMPT}}});

    auto results = LlmComplete::Operation(chunk);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], EXPECTED_RESPONSE);
}

TEST_F(LLMCompleteTest, Operation_ThreeArguments_BatchProcessing) {
    const nlohmann::json expected_response = {{"tuples", {"response 1", "response 2"}}};
    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(expected_response));

    duckdb::DataChunk chunk;
    auto model_struct = CreateModelStruct();
    auto prompt_struct = CreatePromptStruct();
    auto input_struct = CreateInputStruct({"variable1", "variable2"});

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
    chunk.SetCardinality(2);

    // Set data for batch processing
    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", TEST_PROMPT}}});
    SetStructStringData(chunk.data[2], {{{"variable1", "Hello"}, {"variable2", "World"}},
                                        {{"variable1", "Good"}, {"variable2", "Morning"}}});

    auto results = LlmComplete::Operation(chunk);

    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results, expected_response["tuples"]);
}

TEST_F(LLMCompleteTest, Operation_InvalidArguments_ThrowsException) {
    duckdb::DataChunk chunk;
    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {duckdb::LogicalType::VARCHAR});
    chunk.SetCardinality(1);

    EXPECT_THROW(LlmComplete::Operation(chunk), std::runtime_error);
}

TEST_F(LLMCompleteTest, Operation_EmptyPrompt_HandlesGracefully) {
    duckdb::DataChunk chunk;
    CreateBasicChunk(chunk);

    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"prompt", ""}}});

    EXPECT_THROW(LlmComplete::Operation(chunk), std::runtime_error);
}

TEST_F(LLMCompleteTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;

    nlohmann::json expected_response = {{"tuples", {}}};
    for (size_t i = 0; i < input_count; i++) {
        expected_response["tuples"].push_back("response " + std::to_string(i));
    }

    EXPECT_CALL(*mock_provider, CallComplete(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(expected_response));

    duckdb::DataChunk chunk;
    auto model_struct = CreateModelStruct();
    auto prompt_struct = CreatePromptStruct();
    auto input_struct = CreateInputStruct({"text"});

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, prompt_struct, input_struct});
    chunk.SetCardinality(input_count);

    // Set model and prompt data
    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
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
    EXPECT_EQ(results, expected_response["tuples"]);
}

}// namespace flockmtl