#include "flock/functions/scalar/llm_embedding.hpp"
#include "fmt/format.h"
#include "llm_function_test_base.hpp"

namespace flock {

class LLMEmbeddingTest : public LLMFunctionTestBase<LlmEmbedding> {
protected:
    // Expected embedding response - typical dimension for embeddings
    static const std::vector<std::vector<double>> EXPECTED_EMBEDDINGS;

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
};

// Static member definition
const std::vector<std::vector<double>> LLMEmbeddingTest::EXPECTED_EMBEDDINGS = {
        {0.1, 0.2, 0.3, 0.4, 0.5},
        {0.2, 0.3, 0.4, 0.5, 0.6},
        {0.3, 0.4, 0.5, 0.6, 0.7}};

// Test llm_embedding with SQL queries - new API
TEST_F(LLMEmbeddingTest, LLMEmbeddingBasicUsage) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();
    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'text-embedding-3-small'}, {'context_columns': [{'data': text}]}) AS embedding FROM unnest(['This is a test document']) as tbl(text);");
    ASSERT_EQ(results->RowCount(), 1);

    // Check that we got a list back
    auto result_value = results->GetValue(0, 0);
    ASSERT_EQ(result_value.type().id(), duckdb::LogicalTypeId::LIST);
}

TEST_F(LLMEmbeddingTest, LLMEmbeddingWithMultipleFields) {
    const nlohmann::json expected_response = GetExpectedJsonResponse();
    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'text-embedding-3-small'}, {'context_columns': [{'data': title}, {'data': content}]}) AS embedding FROM VALUES('Document Title', 'Document content here') as tbl(title, content);");
    ASSERT_EQ(results->RowCount(), 1);

    // Check that we got a list back
    auto result_value = results->GetValue(0, 0);
    ASSERT_EQ(result_value.type().id(), duckdb::LogicalTypeId::LIST);
}

TEST_F(LLMEmbeddingTest, ValidateArguments) {
    TestValidateArguments();
}

TEST_F(LLMEmbeddingTest, Operation_BatchProcessing) {
    const nlohmann::json expected_response = nlohmann::json::array({{0.1, 0.2, 0.3, 0.4, 0.5}, {0.2, 0.3, 0.4, 0.5, 0.6}});
    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'text-embedding-3-small'}, {'context_columns': [{'data': text}]}) AS embedding FROM unnest(['First document text', 'Second document text']) as tbl(text);");
    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 2);

    // Verify both results are lists
    auto result1 = results->GetValue(0, 0);
    auto result2 = results->GetValue(0, 1);
    ASSERT_EQ(result1.type().id(), duckdb::LogicalTypeId::LIST);
    ASSERT_EQ(result2.type().id(), duckdb::LogicalTypeId::LIST);
}

TEST_F(LLMEmbeddingTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 10;
    nlohmann::json expected_response = nlohmann::json::array();
    for (size_t i = 0; i < input_count; i++) {
        std::vector<double> embedding;
        for (size_t j = 0; j < 5; j++) {
            embedding.push_back(0.01 * i + 0.1 * j);
        }
        expected_response.push_back(embedding);
    }

    EXPECT_CALL(*mock_provider, AddEmbeddingRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectEmbeddings(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'text-embedding-3-small'}, {'context_columns': [{'data': content}]}) AS embedding FROM range(" + std::to_string(input_count) + ") AS t(i), unnest(['Document content number ' || i::VARCHAR]) as tbl(content);");
    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), input_count);

    // Verify all results are lists
    for (size_t i = 0; i < input_count; i++) {
        auto result = results->GetValue(0, i);
        ASSERT_EQ(result.type().id(), duckdb::LogicalTypeId::LIST);
    }
}

}// namespace flock
