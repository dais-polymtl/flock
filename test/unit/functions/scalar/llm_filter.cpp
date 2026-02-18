#include "flock/functions/scalar/llm_filter.hpp"
#include "llm_function_test_base.hpp"

namespace flock {

class LLMFilterTest : public LLMFunctionTestBase<LlmFilter> {
protected:
    static constexpr const char* EXPECTED_RESPONSE = "true";
    static constexpr const char* EXPECTED_FALSE_RESPONSE = "false";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json{{"items", {true}}};
    }

    std::string GetFunctionName() const override {
        return "llm_filter";
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        nlohmann::json expected_response = {{"items", {}}};
        for (const auto& response: responses) {
            // Convert string response to boolean for filter responses
            bool filter_result = (response == "True" || response == "true" || response == "1");
            expected_response["items"].push_back(filter_result);
        }
        return expected_response;
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        nlohmann::json expected_response = {{"items", {}}};
        for (size_t i = 0; i < input_count; i++) {
            // Alternate between true and false for testing
            expected_response["items"].push_back(i % 2 == 0);
        }
        return expected_response;
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        if (response.contains("items") && response["items"].is_array() && !response["items"].empty()) {
            bool result = response["items"][0].get<bool>();
            return result ? "true" : "false";
        }
        return response.dump();
    }
};

// Test llm_filter with SQL queries - new API
TEST_F(LLMFilterTest, LLMFilterBasicUsage) {
    const nlohmann::json expected_response = {{"items", {true}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Is this sentiment positive?', 'context_columns': [{'data': text}]}) AS filter_result FROM unnest(['I love this product!']) as tbl(text);");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "true");
}

TEST_F(LLMFilterTest, LLMFilterWithoutContextColumns) {
    const nlohmann::json expected_response = {{"items", {true}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Are you a Robot?'}) AS filter_result;");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "true");
}

TEST_F(LLMFilterTest, LLMFilterWithMultipleRows) {
    const nlohmann::json expected_response = {{"items", {true}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Is this a valid email address?', 'context_columns': [{'data': email}]}) AS filter_result FROM unnest(['test@example.com', 'invalid-email', 'user@domain.org']) as tbl(email);");
    ASSERT_EQ(results->RowCount(), 3);
    // Check first result
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "true");
}

TEST_F(LLMFilterTest, ValidateArguments) {
    TestValidateArguments();
}

TEST_F(LLMFilterTest, Operation_BatchProcessing) {
    const nlohmann::json expected_response = {{"items", {true, false}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Is this review positive?', 'context_columns': [{'data': review}]}) AS result FROM unnest(['Great product!', 'Terrible quality']) as tbl(review);");
    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 2);
    EXPECT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "true");
}

TEST_F(LLMFilterTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 10;

    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'Is this content spam?', 'context_columns': [{'data': content}]}) AS result FROM range(" + std::to_string(input_count) + ") AS t(i), unnest(['Content item ' || i::VARCHAR]) as tbl(content);");
    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), input_count);

    // Verify the first few results match expected
    for (size_t i = 0; i < std::min<size_t>(input_count, size_t(5)); i++) {
        auto result_value = results->GetValue(0, i).GetValue<std::string>();
        auto expected_value = expected_response["items"][i].get<bool>() ? "true" : "false";
        EXPECT_EQ(result_value, expected_value);
    }
}

// Test llm_filter with audio transcription
TEST_F(LLMFilterTest, LLMFilterWithAudioTranscription) {
    const nlohmann::json expected_transcription = "{\"text\": \"This audio contains positive sentiment\"}";
    const nlohmann::json expected_complete_response = {{"items", {true}}};

    // Mock transcription model
    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription}));

    // Mock completion model
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_complete_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT llm_filter("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Is the sentiment in this audio positive?', "
            "'context_columns': ["
            "{'data': audio_url, "
            "'type': 'audio', "
            "'transcription_model': 'gpt-4o-transcribe'}"
            "]}) AS result FROM VALUES ('https://example.com/audio.mp3') AS tbl(audio_url);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
}

// Test llm_filter with audio and text columns
TEST_F(LLMFilterTest, LLMFilterWithAudioAndText) {
    const nlohmann::json expected_transcription = "{\"text\": \"Product review audio\"}";
    const nlohmann::json expected_complete_response = {{"items", {true}}};

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription}));

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_complete_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT llm_filter("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Is this product review positive?', "
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
TEST_F(LLMFilterTest, LLMFilterAudioTranscriptionOllamaError) {
    auto con = Config::GetConnection();

    // Mock transcription model to throw error (simulating Ollama behavior)
    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .WillOnce(::testing::Throw(std::runtime_error("Audio transcription is not currently supported by Ollama.")));

    // Test with Ollama which doesn't support transcription
    const auto results = con.Query(
            "SELECT llm_filter("
            "{'model_name': 'gemma3:4b'}, "
            "{'prompt': 'Is the sentiment positive?', "
            "'context_columns': ["
            "{'data': audio_url, "
            "'type': 'audio', "
            "'transcription_model': 'gemma3:4b'}"
            "]}) AS result FROM VALUES ('https://example.com/audio.mp3') AS tbl(audio_url);");

    // Should fail because Ollama doesn't support transcription
    ASSERT_TRUE(results->HasError());
}

}// namespace flock
