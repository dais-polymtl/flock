#include "flock/functions/aggregate/llm_first_or_last.hpp"
#include "llm_aggregate_function_test_base.hpp"

namespace flock {

class LLMLastTest : public LLMAggregateTestBase<LlmFirstOrLast> {
protected:
    static constexpr const char* LLM_RESPONSE = R"({"items":[2]})";
    static constexpr const char* EXPECTED_RESPONSE_SINGLE = R"([{"data":["Smart fitness tracker with heart rate monitoring"]}])";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE_SINGLE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json::parse(LLM_RESPONSE);
    }

    std::string GetFunctionName() const override {
        return "llm_last";
    }

    AggregateFunctionType GetFunctionType() const override {
        return AggregateFunctionType::LAST;
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        return nlohmann::json{{"items", {static_cast<int>(responses.size() - 1)}}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        return nlohmann::json{{"items", {static_cast<int>(input_count - 1)}}};
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        return response.dump();
    }
};

// Test 1-tuple case: no LLM call needed, returns the single tuple directly
TEST_F(LLMLastTest, SingleTupleNoLLMCall) {
    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT llm_last("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Select the last product', 'context_columns': [{'data': description}]}"
            ") AS last_product FROM VALUES "
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
TEST_F(LLMLastTest, MultipleTuplesWithoutGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT llm_last("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'What is the least relevant product?', 'context_columns': [{'data': description}]}"
            ") AS last_product FROM VALUES "
            "('High-performance running shoes with advanced cushioning'), "
            "('Wireless noise-cancelling headphones for immersive audio'), "
            "('Smart fitness tracker with heart rate monitoring') AS products(description);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
}

// Test GROUP BY with multiple tuples per group: LLM is called for each group
TEST_F(LLMLastTest, GroupByWithMultipleTuplesPerGroup) {
    nlohmann::json response_index_1 = nlohmann::json{{"items", {1}}};

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(2);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(2)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{response_index_1}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, llm_last("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Select the least relevant product', 'context_columns': [{'data': description}]}"
            ") AS last_product FROM VALUES "
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
        });
    }
}

// Test GROUP BY with single tuple per group: no LLM calls needed
TEST_F(LLMLastTest, GroupByWithSingleTuplePerGroup) {
    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, llm_last("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Select the least relevant product', 'context_columns': [{'data': description}]}"
            ") AS last_product FROM VALUES "
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
TEST_F(LLMLastTest, ValidateArguments) {
    TestValidateArguments();
}

// Test operation with invalid arguments
TEST_F(LLMLastTest, InvalidArguments) {
    TestOperationInvalidArguments();
}

// Test with audio transcription
TEST_F(LLMLastTest, AudioTranscription) {
    const nlohmann::json expected_transcription1 = nlohmann::json::parse(R"({"text": "First audio candidate"})");
    const nlohmann::json expected_transcription2 = nlohmann::json::parse(R"({"text": "Last audio candidate"})");
    nlohmann::json response_index_1 = nlohmann::json{{"items", {1}}};

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription1, expected_transcription2}));

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{response_index_1}));

    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT llm_last("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Select the worst audio candidate', "
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
TEST_F(LLMLastTest, AudioTranscriptionOllamaError) {
    auto con = Config::GetConnection();
    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .WillOnce(::testing::Throw(std::runtime_error("Audio transcription is not currently supported by Ollama.")));

    const auto results = con.Query(
            "SELECT llm_last("
            "{'model_name': 'gemma3:4b'}, "
            "{'prompt': 'Select the worst audio', "
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
