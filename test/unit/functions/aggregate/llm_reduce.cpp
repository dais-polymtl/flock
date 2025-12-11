#include "flock/functions/aggregate/llm_reduce.hpp"
#include "llm_aggregate_function_test_base.hpp"

namespace flock {

class LLMReduceTest : public LLMAggregateTestBase<LlmReduce> {
protected:
    static constexpr const char* EXPECTED_RESPONSE = "A comprehensive summary of products.";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json{{"items", {EXPECTED_RESPONSE}}};
    }

    std::string GetFunctionName() const override {
        return "llm_reduce";
    }

    AggregateFunctionType GetFunctionType() const override {
        return AggregateFunctionType::REDUCE;
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        return nlohmann::json{{"items", {responses[0]}}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        return nlohmann::json{{"items", {"Summary of " + std::to_string(input_count) + " items processed"}}};
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        if (response.contains("items")) {
            return response["items"][0].get<std::string>();
        }
        return response.get<std::string>();
    }
};

// Test single tuple: LLM is still called for reduce (to summarize)
TEST_F(LLMReduceTest, SingleTupleWithLLMCall) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT llm_reduce("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Summarize the following product descriptions', 'context_columns': [{'data': description}]}"
            ") AS product_summary FROM VALUES "
            "('High-performance running shoes with advanced cushioning') AS products(description);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
}

// Test multiple tuples without GROUP BY: LLM is called once
TEST_F(LLMReduceTest, MultipleTuplesWithoutGroupBy) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT llm_reduce("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Summarize the following product descriptions', 'context_columns': [{'data': description}]}"
            ") AS product_summary FROM VALUES "
            "('High-performance running shoes with advanced cushioning'), "
            "('Wireless noise-cancelling headphones for immersive audio'), "
            "('Smart fitness tracker with heart rate monitoring') AS products(description);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
}

// Test GROUP BY with multiple tuples per group: LLM is called for each group
TEST_F(LLMReduceTest, GroupByWithMultipleTuplesPerGroup) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(2);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(2)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, llm_reduce("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Summarize the following product descriptions', 'context_columns': [{'data': description}]}"
            ") AS description_summary FROM VALUES "
            "('footwear', 'Running shoes with cushioning'), "
            "('footwear', 'Business shoes for professionals'), "
            "('electronics', 'Wireless headphones'), "
            "('electronics', 'Smart fitness tracker') "
            "AS products(category, description) GROUP BY category;");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 2);
    ASSERT_EQ(results->GetValue(1, 0).GetValue<std::string>(), GetExpectedResponse());
    ASSERT_EQ(results->GetValue(1, 1).GetValue<std::string>(), GetExpectedResponse());
}

// Test GROUP BY with single tuple per group: LLM is still called (reduce always calls LLM)
TEST_F(LLMReduceTest, GroupByWithSingleTuplePerGroup) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(3);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(3)
            .WillRepeatedly(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();

    const auto results = con.Query(
            "SELECT category, llm_reduce("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Summarize the following product descriptions', 'context_columns': [{'data': description}]}"
            ") AS description_summary FROM VALUES "
            "('electronics', 'Running shoes with advanced cushioning'), "
            "('audio', 'Wireless noise-cancelling headphones'), "
            "('fitness', 'Smart fitness tracker with heart rate monitoring') "
            "AS products(category, description) GROUP BY category;");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 3);
    ASSERT_EQ(results->GetValue(1, 0).GetValue<std::string>(), GetExpectedResponse());
    ASSERT_EQ(results->GetValue(1, 1).GetValue<std::string>(), GetExpectedResponse());
    ASSERT_EQ(results->GetValue(1, 2).GetValue<std::string>(), GetExpectedResponse());
}

// Test argument validation
TEST_F(LLMReduceTest, ValidateArguments) {
    TestValidateArguments();
}

// Test operation with invalid arguments
TEST_F(LLMReduceTest, InvalidArguments) {
    TestOperationInvalidArguments();
}

// Test with audio transcription
TEST_F(LLMReduceTest, AudioTranscription) {
    const nlohmann::json expected_transcription = nlohmann::json::parse(R"({"text": "This is a transcribed audio summary"})");

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription}));

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT llm_reduce("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Summarize the following audio content', "
            "'context_columns': ["
            "{'data': audio_url, "
            "'type': 'audio', "
            "'transcription_model': 'gpt-4o-transcribe'}"
            "]}) AS result FROM VALUES ('https://example.com/audio.mp3') AS tbl(audio_url);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
}

// Test with audio and text columns
TEST_F(LLMReduceTest, AudioAndTextColumns) {
    const nlohmann::json expected_transcription = nlohmann::json::parse(R"({"text": "Product audio review"})");

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription}));

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT llm_reduce("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Summarize the product reviews', "
            "'context_columns': ["
            "{'data': text_review, 'name': 'text_review'}, "
            "{'data': audio_url, "
            "'type': 'audio', "
            "'transcription_model': 'gpt-4o-transcribe'}"
            "]}) AS result FROM VALUES ('Great product', 'https://example.com/audio.mp3') AS tbl(text_review, audio_url);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
}

// Test audio transcription error handling for Ollama
TEST_F(LLMReduceTest, AudioTranscriptionOllamaError) {
    auto con = Config::GetConnection();
    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .WillOnce(::testing::Throw(std::runtime_error("Audio transcription is not currently supported by Ollama.")));

    const auto results = con.Query(
            "SELECT llm_reduce("
            "{'model_name': 'gemma3:4b'}, "
            "{'prompt': 'Summarize this audio', "
            "'context_columns': ["
            "{'data': audio_url, "
            "'type': 'audio', "
            "'transcription_model': 'gemma3:4b'}"
            "]}) AS result FROM VALUES ('https://example.com/audio.mp3') AS tbl(audio_url);");

    ASSERT_TRUE(results->HasError());
}

}// namespace flock
