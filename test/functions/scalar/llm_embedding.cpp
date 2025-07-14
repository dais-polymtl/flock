#include "flockmtl/functions/scalar/llm_embedding.hpp"
#include "fmt/format.h"
#include "llm_function_test_base.hpp"

namespace flockmtl {

class LLMEmbeddingTest : public LLMFunctionTestBase<LlmEmbedding> {
protected:
    // Expected embedding response - typical dimension for embeddings
    static const std::vector<std::vector<double>> EXPECTED_EMBEDDINGS;
    static constexpr const char* EXPECTED_TEXT_INPUT = "This is a test input for embedding generation.";

    std::string GetExpectedResponse() const override {
        // For embedding, this represents the first embedding as string representation
        return "[0.1, 0.2, 0.3, 0.4, 0.5]";
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json::array({{0.1, 0.2, 0.3, 0.4, 0.5}});
    }

    std::string GetFunctionName() const override {
        return "llm_embedding";
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        // For embeddings, we need to convert string responses to embedding arrays
        nlohmann::json batch_embeddings = nlohmann::json::array();
        for (size_t i = 0; i < responses.size(); i++) {
            // Create different embeddings for each response
            std::vector<double> embedding;
            for (size_t j = 0; j < 5; j++) {
                embedding.push_back(0.1 * (i + 1) + 0.1 * j);
            }
            batch_embeddings.push_back(embedding);
        }
        return batch_embeddings;
    }

    // Helper method to handle vector of embeddings
    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::vector<double>>& embeddings) const {
        nlohmann::json batch_embeddings = nlohmann::json::array();
        for (const auto& embedding: embeddings) {
            batch_embeddings.push_back(embedding);
        }
        return batch_embeddings;
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        nlohmann::json large_embeddings = nlohmann::json::array();
        for (size_t i = 0; i < input_count; i++) {
            std::vector<double> embedding;
            for (size_t j = 0; j < 5; j++) {
                // Generate varied embeddings for each input
                embedding.push_back(0.01 * i + 0.1 * j);
            }
            large_embeddings.push_back(embedding);
        }
        return large_embeddings;
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        if (response.is_array() && !response.empty()) {
            // Return string representation of the first embedding
            std::string result = "[";
            for (size_t i = 0; i < response[0].size(); i++) {
                if (i > 0) result += ", ";
                result += duckdb_fmt::format("{:.1f}", response[0][i].get<double>());
            }
            result += "]";
            return result;
        }
        return "[]";
    }

    // Helper to create input struct for embedding tests
    static duckdb::LogicalType CreateEmbeddingInputStruct() {
        duckdb::child_list_t<duckdb::LogicalType> fields = {
                {"text", duckdb::LogicalType::VARCHAR}};
        return duckdb::LogicalType::STRUCT(fields);
    }

    // Helper to setup a 2-argument chunk for embedding testing
    void CreateEmbeddingChunk(duckdb::DataChunk& chunk, size_t cardinality = 1) {
        auto model_struct = CreateModelStruct();
        auto input_struct = CreateEmbeddingInputStruct();

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, input_struct});
        chunk.SetCardinality(cardinality);
    }
};

// Static member definition
const std::vector<std::vector<double>> LLMEmbeddingTest::EXPECTED_EMBEDDINGS = {
        {0.1, 0.2, 0.3, 0.4, 0.5},
        {0.2, 0.3, 0.4, 0.5, 0.6},
        {0.3, 0.4, 0.5, 0.6, 0.7}};

// Test llm_embedding with SQL queries
TEST_F(LLMEmbeddingTest, LLMEmbeddingWithTextInput) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();
    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'text-embedding-3-small'}, {'text': 'This is a test document'}) AS embedding;");
    ASSERT_EQ(results->RowCount(), 1);

    // Check that we got a list back
    auto result_value = results->GetValue(0, 0);
    ASSERT_EQ(result_value.type().id(), duckdb::LogicalTypeId::LIST);
    ASSERT_EQ(result_value.ToSQLString(), FormatExpectedResult(expected_response));
}

TEST_F(LLMEmbeddingTest, LLMEmbeddingWithMultipleTextFields) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();
    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'text-embedding-3-small'}, {'title': 'Document Title', 'content': 'Document content here'}) AS embedding;");
    ASSERT_EQ(results->RowCount(), 1);

    // Check that we got a list back
    auto result_value = results->GetValue(0, 0);
    ASSERT_EQ(result_value.type().id(), duckdb::LogicalTypeId::LIST);
    ASSERT_EQ(result_value.ToSQLString(), FormatExpectedResult(expected_response));
}

TEST_F(LLMEmbeddingTest, ValidateArguments) {
    // Test valid case with exactly 2 arguments
    {
        duckdb::DataChunk chunk;
        auto model_struct = CreateGenericStruct({{"provider", duckdb::LogicalType::VARCHAR}, {"model", duckdb::LogicalType::VARCHAR}});
        auto input_struct = CreateGenericStruct({{"text", duckdb::LogicalType::VARCHAR}});

        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, input_struct});
        chunk.SetCardinality(1);

        EXPECT_NO_THROW(LlmEmbedding::ValidateArguments(chunk));
    }

    // Test invalid cases
    struct InvalidTestCase {
        std::string description;
        duckdb::vector<duckdb::LogicalType> types;
    };

    std::vector<InvalidTestCase> invalid_cases = {
            {"too few arguments (1 argument)", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}})}},
            {"too many arguments (3 arguments)", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}})}},
            {"first argument is not a struct", {duckdb::LogicalType::VARCHAR, CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}})}},
            {"second argument is not a struct", {CreateGenericStruct({{"field", duckdb::LogicalType::VARCHAR}}), duckdb::LogicalType::INTEGER}}};

    for (const auto& test_case: invalid_cases) {
        SCOPED_TRACE("Testing: " + test_case.description);
        duckdb::DataChunk chunk;
        chunk.Initialize(duckdb::Allocator::DefaultAllocator(), test_case.types);
        chunk.SetCardinality(1);

        EXPECT_THROW(LlmEmbedding::ValidateArguments(chunk), std::runtime_error);
    }
}

TEST_F(LLMEmbeddingTest, Operation_TwoArguments_RequiredStructure) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();
    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    CreateEmbeddingChunk(chunk);

    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"text", EXPECTED_TEXT_INPUT}}});

    auto results = LlmEmbedding::Operation(chunk);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].size(), 5);// Expected embedding dimension

    // Verify the embedding values
    for (size_t i = 0; i < results[0].size(); i++) {
        EXPECT_DOUBLE_EQ(results[0][i].GetValue<double>(), expected_response[0][i].get<double>());
    }
}

TEST_F(LLMEmbeddingTest, Operation_BatchProcessing) {
    const std::vector<std::vector<double>> batch_embeddings = {{0.1, 0.2, 0.3, 0.4, 0.5}, {0.2, 0.3, 0.4, 0.5, 0.6}};
    const nlohmann::json expected_response = PrepareExpectedResponseForBatch(batch_embeddings);
    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    CreateEmbeddingChunk(chunk, 2);

    // Set data for batch processing
    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"text", "First document text"}},
                                        {{"text", "Second document text"}}});

    auto results = LlmEmbedding::Operation(chunk);

    EXPECT_EQ(results.size(), 2);

    // Verify first embedding
    EXPECT_EQ(results[0].size(), 5);
    for (size_t i = 0; i < results[0].size(); i++) {
        EXPECT_DOUBLE_EQ(results[0][i].GetValue<double>(), batch_embeddings[0][i]);
    }

    // Verify second embedding
    EXPECT_EQ(results[1].size(), 5);
    for (size_t i = 0; i < results[1].size(); i++) {
        EXPECT_DOUBLE_EQ(results[1][i].GetValue<double>(), batch_embeddings[1][i]);
    }
}

TEST_F(LLMEmbeddingTest, Operation_MultipleInputFields) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();
    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    auto model_struct = CreateModelStruct();
    auto input_struct = CreateInputStruct({"title", "content", "summary"});

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, input_struct});
    chunk.SetCardinality(1);

    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"title", "Document Title"}, {"content", "Main content here"}, {"summary", "Brief summary"}}});

    auto results = LlmEmbedding::Operation(chunk);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].size(), 5);// Expected embedding dimension

    // Verify the exact embedding values match the expected response
    for (size_t i = 0; i < results[0].size(); i++) {
        EXPECT_TRUE(results[0][i].IsNull() == false);
        EXPECT_EQ(results[0][i].type().id(), duckdb::LogicalTypeId::DOUBLE);
        EXPECT_DOUBLE_EQ(results[0][i].GetValue<double>(), expected_response[0][i].get<double>());
    }
}

TEST_F(LLMEmbeddingTest, Operation_InvalidArguments_ThrowsException) {
    // Test with only 1 argument (should fail)
    duckdb::DataChunk chunk;
    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {duckdb::LogicalType::VARCHAR});
    chunk.SetCardinality(1);

    EXPECT_THROW(LlmEmbedding::Operation(chunk), std::runtime_error);
}

TEST_F(LLMEmbeddingTest, Operation_EmptyInput_HandlesGracefully) {
    duckdb::DataChunk chunk;
    CreateEmbeddingChunk(chunk);

    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"text", ""}}});

    // llm_embedding should handle empty input gracefully
    // The exact behavior might depend on the implementation
    EXPECT_NO_THROW(LlmEmbedding::Operation(chunk));
}

TEST_F(LLMEmbeddingTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;

    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);

    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    CreateEmbeddingChunk(chunk, input_count);

    // Set model data
    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});

    // Create large input dataset
    std::vector<std::map<std::string, std::string>> large_input;
    large_input.reserve(input_count);
    for (size_t i = 0; i < input_count; i++) {
        large_input.push_back({{"text", "Document content number " + std::to_string(i)}});
    }

    SetStructStringData(chunk.data[1], large_input);

    const auto results = LlmEmbedding::Operation(chunk);

    EXPECT_EQ(results.size(), input_count);

    // Verify that each result matches the expected embedding values
    for (size_t i = 0; i < results.size(); i++) {
        EXPECT_EQ(results[i].size(), 5);// Expected embedding dimension
        for (size_t j = 0; j < results[i].size(); j++) {
            EXPECT_TRUE(results[i][j].IsNull() == false);
            EXPECT_EQ(results[i][j].type().id(), duckdb::LogicalTypeId::DOUBLE);
            // Verify exact values match expected response
            EXPECT_DOUBLE_EQ(results[i][j].GetValue<double>(), expected_response[i][j].get<double>());
        }
    }
}

TEST_F(LLMEmbeddingTest, Operation_ConcatenatedFields_ProcessesCorrectly) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();
    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    duckdb::DataChunk chunk;
    auto model_struct = CreateModelStruct();
    auto input_struct = CreateInputStruct({"product_name", "product_description", "category"});

    chunk.Initialize(duckdb::Allocator::DefaultAllocator(), {model_struct, input_struct});
    chunk.SetCardinality(1);

    SetStructStringData(chunk.data[0], {{{"model_name", DEFAULT_MODEL}}});
    SetStructStringData(chunk.data[1], {{{"product_name", "Wireless Headphones"},
                                         {"product_description", "High-quality wireless headphones with noise cancellation"},
                                         {"category", "Electronics"}}});

    auto results = LlmEmbedding::Operation(chunk);

    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].size(), 5);// Expected embedding dimension

    // Verify the embedding values are reasonable
    for (const auto& value: results[0]) {
        EXPECT_TRUE(value.IsNull() == false);
        EXPECT_EQ(value.type().id(), duckdb::LogicalTypeId::DOUBLE);
        // Check that the values are in a reasonable range for embeddings
        double val = value.GetValue<double>();
        EXPECT_GE(val, -2.0);
        EXPECT_LE(val, 2.0);
    }
}

}// namespace flockmtl
