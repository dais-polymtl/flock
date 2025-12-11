#include "flock/functions/aggregate/llm_rerank.hpp"
#include "llm_aggregate_function_test_base.hpp"
#include <numeric>

namespace flock {

class LLMRerankTest : public LLMAggregateTestBase<LlmRerank> {
protected:
    static constexpr const char* LLM_RESPONSE = R"({"items":[0, 1, 2]})";
    static constexpr const char* EXPECTED_RESPONSE_SINGLE = R"([{"data":["High-performance running shoes with advanced cushioning"]}])";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE_SINGLE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json::parse(LLM_RESPONSE);
    }

    std::string GetFunctionName() const override {
        return "llm_rerank";
    }

    AggregateFunctionType GetFunctionType() const override {
        return AggregateFunctionType::RERANK;
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        std::vector<int> ranking_indices(responses.size());
        std::iota(ranking_indices.begin(), ranking_indices.end(), 0);
        return nlohmann::json{{"items", ranking_indices}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        std::vector<int> ranking_indices(input_count);
        std::iota(ranking_indices.begin(), ranking_indices.end(), 0);
        return nlohmann::json{{"items", ranking_indices}};
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        return response.dump();
    }
};

// Test 1-tuple case: no LLM call needed, returns the single tuple directly
TEST_F(LLMRerankTest, SingleTupleNoLLMCall) {
    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT llm_rerank("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Rank these products', 'context_columns': [{'data': description}]}"
            ") AS reranked_products FROM VALUES "
            "('High-performance running shoes with advanced cushioning') AS products(description);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);

    nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, 0).GetValue<std::string>());
    EXPECT_EQ(parsed.size(), 1);
    EXPECT_TRUE(parsed[0].contains("data"));
    EXPECT_EQ(parsed[0]["data"].size(), 1);
    EXPECT_EQ(parsed[0]["data"][0], "High-performance running shoes with advanced cushioning");
}

// Test multiple tuples without GROUP BY: LLM is called once
TEST_F(LLMRerankTest, MultipleTuplesWithoutGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT llm_rerank("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Rank these products by relevance', 'context_columns': [{'data': description}]}"
            ") AS reranked_products FROM VALUES "
            "('High-performance running shoes with advanced cushioning'), "
            "('Professional business shoes'), "
            "('Casual sneakers for everyday wear') AS products(description);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
    EXPECT_NO_THROW({
        nlohmann::json parsed = nlohmann::json::parse(results->GetValue(0, 0).GetValue<std::string>());
        EXPECT_EQ(parsed.size(), 1);
        EXPECT_TRUE(parsed[0].contains("data"));
        EXPECT_EQ(parsed[0]["data"].size(), 3);
    });
}

// Test GROUP BY with multiple tuples per group: LLM is called for each group
TEST_F(LLMRerankTest, GroupByWithMultipleTuplesPerGroup) {
    nlohmann::json response_2_items = nlohmann::json{{"items", {1, 0}}};

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(2);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(2)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{response_2_items}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, llm_rerank("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Rank these products by relevance', 'context_columns': [{'data': description}]}"
            ") AS reranked_products FROM VALUES "
            "('footwear', 'Running shoes with cushioning'), "
            "('footwear', 'Business shoes for professionals'), "
            "('electronics', 'Wireless headphones'), "
            "('electronics', 'Smart fitness tracker') "
            "AS products(category, description) GROUP BY category;");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 2);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(1, i).GetValue<std::string>());
            EXPECT_TRUE(parsed[0].contains("data"));
            EXPECT_EQ(parsed[0]["data"].size(), 2);
        });
    }
}

// Test GROUP BY with single tuple per group: no LLM calls needed
TEST_F(LLMRerankTest, GroupByWithSingleTuplePerGroup) {
    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, llm_rerank("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Rank these products by relevance', 'context_columns': [{'data': description}]}"
            ") AS reranked_products FROM VALUES "
            "('footwear', 'Running shoes with cushioning'), "
            "('electronics', 'Wireless headphones'), "
            "('fitness', 'Smart fitness tracker') "
            "AS products(category, description) GROUP BY category;");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 3);
    for (idx_t i = 0; i < results->RowCount(); i++) {
        EXPECT_NO_THROW({
            nlohmann::json parsed = nlohmann::json::parse(results->GetValue(1, i).GetValue<std::string>());
            EXPECT_TRUE(parsed[0].contains("data"));
            EXPECT_EQ(parsed[0]["data"].size(), 1);
        });
    }
}

// Test argument validation
TEST_F(LLMRerankTest, ValidateArguments) {
    TestValidateArguments();
}

// Test operation with invalid arguments
TEST_F(LLMRerankTest, InvalidArguments) {
    TestOperationInvalidArguments();
}

// Test with audio transcription
TEST_F(LLMRerankTest, AudioTranscription) {
    const nlohmann::json expected_transcription1 = nlohmann::json::parse(R"({"text": "First audio candidate"})");
    const nlohmann::json expected_transcription2 = nlohmann::json::parse(R"({"text": "Second audio candidate"})");
    nlohmann::json response_2_items = nlohmann::json{{"items", {1, 0}}};

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription1, expected_transcription2}));

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{response_2_items}));

    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT llm_rerank("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Rank these audio candidates from best to worst', "
            "'context_columns': ["
            "{'data': audio_url, "
            "'type': 'audio', "
            "'transcription_model': 'gpt-4o-transcribe'}"
            "]}) AS result FROM VALUES "
            "('https://example.com/audio1.mp3'), "
            "('https://example.com/audio2.mp3') AS tbl(audio_url);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
}

// Test audio transcription error handling for Ollama
TEST_F(LLMRerankTest, AudioTranscriptionOllamaError) {
    auto con = Config::GetConnection();
    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .WillOnce(::testing::Throw(std::runtime_error("Audio transcription is not currently supported by Ollama.")));

    const auto results = con.Query(
            "SELECT llm_rerank("
            "{'model_name': 'gemma3:4b'}, "
            "{'prompt': 'Rank these audio files', "
            "'context_columns': ["
            "{'data': audio_url, "
            "'type': 'audio', "
            "'transcription_model': 'gemma3:4b'}"
            "]}) AS result FROM VALUES "
            "('https://example.com/audio1.mp3'), "
            "('https://example.com/audio2.mp3') AS tbl(audio_url);");

    ASSERT_TRUE(results->HasError());
}

}// namespace flock
