#include "flock/functions/scalar/llm_complete.hpp"
#include "llm_function_test_base.hpp"

namespace flock {

class LLMCompleteTest : public LLMFunctionTestBase<LlmComplete> {
protected:
    static constexpr const char* EXPECTED_RESPONSE = "FlockMTL enhances DuckDB by integrating semantic functions and robust resource management capabilities, enabling advanced analytics and language model operations directly within SQL queries.";

    std::string GetExpectedResponse() const override {
        return EXPECTED_RESPONSE;
    }

    nlohmann::json GetExpectedJsonResponse() const override {
        return nlohmann::json{{"items", {EXPECTED_RESPONSE}}};
    }

    std::string GetFunctionName() const override {
        return "llm_complete";
    }

    nlohmann::json PrepareExpectedResponseForBatch(const std::vector<std::string>& responses) const override {
        return nlohmann::json{{"items", responses}};
    }

    nlohmann::json PrepareExpectedResponseForLargeInput(size_t input_count) const override {
        nlohmann::json expected_response = {{"items", {}}};
        for (size_t i = 0; i < input_count; i++) {
            expected_response["items"].push_back("response " + std::to_string(i));
        }
        return expected_response;
    }

    nlohmann::json PrepareExpectedResponseRange(size_t start_index, size_t count) const {
        nlohmann::json expected_response = {{"items", {}}};
        for (size_t i = 0; i < count; i++) {
            expected_response["items"].push_back("response " + std::to_string(start_index + i));
        }
        return expected_response;
    }

    std::string FormatExpectedResult(const nlohmann::json& response) const override {
        if (response.contains("items") && response["items"].is_array() && !response["items"].empty()) {
            return response["items"][0].get<std::string>();
        }
        return response.get<std::string>();
    }
};

// Test llm_complete with SQL queries
TEST_F(LLMCompleteTest, LLMCompleteWithoutInputColumns) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'},{'prompt': 'Explain the purpose of FlockMTL.'}) AS flock_purpose;");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
}

TEST_F(LLMCompleteTest, LLMCompleteWithInputColumns) {
    const nlohmann::json expected_response = {{"items", {"The capital of Canada is Ottawa."}}};
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': 'What is the capital of', 'context_columns': [{'data': country}]}) AS flock_capital FROM unnest(['Canada']) as tbl(country);");
    ASSERT_EQ(results->RowCount(), 1);
    ASSERT_EQ(results->GetValue(0, 0).GetValue<std::string>(), expected_response["items"][0]);
}

TEST_F(LLMCompleteTest, ValidateArguments) {
    TestValidateArguments();
}

TEST_F(LLMCompleteTest, Operation_TwoArguments_SimplePrompt) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{GetExpectedJsonResponse()}));

    // Use SQL-based approach - DuckDB's best practice for testing custom functions
    auto con = Config::GetConnection();

    auto query = "SELECT " + GetFunctionName() +
                 "({'model_name': '" + std::string(DEFAULT_MODEL) + "'}, " +
                 "{'prompt': '" + std::string(TEST_PROMPT) + "'}) AS result;";

    const auto results = con.Query(query);

    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
    EXPECT_EQ(results->GetValue(0, 0).GetValue<std::string>(), GetExpectedResponse());
}

TEST_F(LLMCompleteTest, Operation_ThreeArguments_BatchProcessing) {
    const std::vector<std::string> responses = {"response 1", "response 2", "response 3"};
    const nlohmann::json expected_response = PrepareExpectedResponseForBatch(responses);
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_response}));

    // Use SQL approach - DuckDB's best practice
    auto con = Config::GetConnection();

    auto query = "SELECT " + GetFunctionName() +
                 "({'model_name': 'gpt-4o'}, " +
                 "{'prompt': 'Explain the purpose of each product.', " +
                 " 'context_columns': [{'data': product}]}) AS result FROM unnest(['Product 1', 'Product 2', 'Product 3']) as tbl(product);";

    const auto results = con.Query(query);

    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 3);

    auto result_value = results->GetValue(0, 0).GetValue<std::string>();
    EXPECT_EQ(result_value, responses[0]);
}

TEST_F(LLMCompleteTest, Operation_InvalidArguments_ThrowsException) {
    // Test with invalid SQL syntax - missing required arguments
    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}) AS result;");
    ASSERT_TRUE(results->HasError()) << "Expected error for missing arguments, but query succeeded";
}

TEST_F(LLMCompleteTest, Operation_EmptyPrompt_HandlesGracefully) {
    // Test with empty prompt using SQL API
    auto con = Config::GetConnection();
    const auto results = con.Query("SELECT " + GetFunctionName() + "({'model_name': 'gpt-4o'}, {'prompt': ''}) AS result;");
    ASSERT_TRUE(results->HasError()) << "Expected error for empty prompt, but query succeeded";
}

TEST_F(LLMCompleteTest, Operation_LargeInputSet_ProcessesCorrectly) {
    constexpr size_t input_count = 100;

    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);
    std::vector<nlohmann::json> batch_responses;
    for (size_t start_index = 0; start_index < input_count; start_index += DEFAULT_BATCH_SIZE) {
        const auto batch_count = std::min<size_t>(DEFAULT_BATCH_SIZE, input_count - start_index);
        batch_responses.push_back(PrepareExpectedResponseRange(start_index, batch_count));
    }
    size_t next_response = 0;

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(static_cast<int>(batch_responses.size()));
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(static_cast<int>(batch_responses.size()))
            .WillRepeatedly(::testing::Invoke([&batch_responses, &next_response](const std::string&) {
                return std::vector<nlohmann::json>{batch_responses[next_response++]};
            }));

    // Use SQL approach - DuckDB's best practice for large datasets
    auto con = Config::GetConnection();

    // Generate a large dataset using DuckDB's range function
    auto query = "SELECT " + GetFunctionName() +
                 "({'model_name': 'gpt-4o', 'is_async': false}, " +
                 "{'prompt': 'Summarize the following text', " +
                 " 'context_columns': [{'data': 'Input text ' || i::VARCHAR}]}) AS result " +
                 "FROM range(" + std::to_string(input_count) + ") AS t(i);";

    const auto results = con.Query(query);

    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), input_count);

    // Verify the first few results match expected
    for (size_t i = 0; i < input_count; i++) {
        auto result_value = results->GetValue(0, i).GetValue<std::string>();
        auto expected_value = expected_response["items"][i].get<std::string>();
        EXPECT_EQ(result_value, expected_value);
    }
}

TEST_F(LLMCompleteTest, Operation_AsyncLargeInputSet_CollectsOnceAndPreservesOrder) {
    constexpr size_t input_count = 100;

    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);
    std::vector<nlohmann::json> batch_responses;
    for (size_t start_index = 0; start_index < input_count; start_index += DEFAULT_BATCH_SIZE) {
        const auto batch_count = std::min<size_t>(DEFAULT_BATCH_SIZE, input_count - start_index);
        batch_responses.push_back(PrepareExpectedResponseRange(start_index, batch_count));
    }

    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(static_cast<int>(batch_responses.size()));
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(1)
            .WillOnce(::testing::Return(batch_responses));

    auto con = Config::GetConnection();
    auto query = "SELECT " + GetFunctionName() +
                 "({'model_name': 'gpt-4o', 'is_async': true}, " +
                 "{'prompt': 'Summarize the following text', " +
                 " 'context_columns': [{'data': 'Input text ' || i::VARCHAR}]}) AS result " +
                 "FROM range(" + std::to_string(input_count) + ") AS t(i);";

    const auto results = con.Query(query);

    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), input_count);

    for (size_t i = 0; i < input_count; i++) {
        auto result_value = results->GetValue(0, i).GetValue<std::string>();
        auto expected_value = expected_response["items"][i].get<std::string>();
        EXPECT_EQ(result_value, expected_value);
    }
}

TEST_F(LLMCompleteTest, Operation_AsyncRetriesWithSmallerBatchOnTokenOverflow) {
    constexpr size_t input_count = 100;
    constexpr size_t retry_batch_size = DEFAULT_BATCH_SIZE / 2;

    const nlohmann::json expected_response = PrepareExpectedResponseForLargeInput(input_count);
    std::vector<nlohmann::json> retry_batch_responses;
    for (size_t start_index = 0; start_index < input_count; start_index += retry_batch_size) {
        const auto batch_count = std::min<size_t>(retry_batch_size, input_count - start_index);
        retry_batch_responses.push_back(PrepareExpectedResponseRange(start_index, batch_count));
    }

    const auto first_attempt_batches = (input_count + DEFAULT_BATCH_SIZE - 1) / DEFAULT_BATCH_SIZE;
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .Times(static_cast<int>(first_attempt_batches + retry_batch_responses.size()));
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(2)
            .WillOnce(::testing::Throw(ExceededMaxOutputTokensError()))
            .WillOnce(::testing::Return(retry_batch_responses));

    auto con = Config::GetConnection();
    auto query = "SELECT " + GetFunctionName() +
                 "({'model_name': 'gpt-4o', 'is_async': true}, " +
                 "{'prompt': 'Summarize the following text', " +
                 " 'context_columns': [{'data': 'Input text ' || i::VARCHAR}]}) AS result " +
                 "FROM range(" + std::to_string(input_count) + ") AS t(i);";

    const auto results = con.Query(query);

    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), input_count);
    for (size_t i = 0; i < input_count; i++) {
        auto result_value = results->GetValue(0, i).GetValue<std::string>();
        auto expected_value = expected_response["items"][i].get<std::string>();
        EXPECT_EQ(result_value, expected_value);
    }
}

TEST_F(LLMCompleteTest, Operation_SyncNullsRowAndContinuesAfterTokenOverflowExhaustion) {
    const nlohmann::json success_response = {{"items", {"response 2"}}};

    {
        ::testing::InSequence sequence;
        EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, 1, ::testing::_, ::testing::_))
                .Times(3);
        EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
                .Times(3)
                .WillOnce(::testing::Throw(ExceededMaxOutputTokensError()))
                .WillOnce(::testing::Throw(ExceededMaxOutputTokensError()))
                .WillOnce(::testing::Return(std::vector<nlohmann::json>{success_response}));
    }

    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o', 'batch_size': 1, 'is_async': false}, "
                                            "{'prompt': 'Summarize', 'context_columns': [{'data': content}]}"
                                            ") AS result FROM unnest(['row-0', 'row-1', 'row-2']) AS tbl(content);");

    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 3);
    EXPECT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "null");
    EXPECT_EQ(results->GetValue(0, 1).GetValue<std::string>(), "null");
    EXPECT_EQ(results->GetValue(0, 2).GetValue<std::string>(), "response 2");
}

TEST_F(LLMCompleteTest, Operation_SyncHalvesBatchAndRetriesBeforeSuccess) {
    nlohmann::json first_half_response = {{"items", {"response 0", "response 1"}}};
    nlohmann::json second_half_response = {{"items", {"response 2", "response 3"}}};

    {
        ::testing::InSequence sequence;
        EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, 4, ::testing::_, ::testing::_))
                .Times(1);
        EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
                .WillOnce(::testing::Throw(ExceededMaxOutputTokensError()));
        EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, 2, ::testing::_, ::testing::_))
                .Times(1);
        EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
                .WillOnce(::testing::Return(std::vector<nlohmann::json>{first_half_response}));
        EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, 2, ::testing::_, ::testing::_))
                .Times(1);
        EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
                .WillOnce(::testing::Return(std::vector<nlohmann::json>{second_half_response}));
    }

    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o', 'batch_size': 4, 'is_async': false}, "
                                            "{'prompt': 'Summarize', 'context_columns': [{'data': content}]}"
                                            ") AS result FROM unnest(['row-0', 'row-1', 'row-2', 'row-3']) AS tbl(content);");

    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 4);
    for (size_t i = 0; i < 4; i++) {
        EXPECT_EQ(results->GetValue(0, i).GetValue<std::string>(), "response " + std::to_string(i));
    }
}

TEST_F(LLMCompleteTest, Operation_AsyncReturnsNullWhenTokenOverflowCannotRetry) {
    EXPECT_CALL(*mock_provider, AddCompletionRequest(::testing::_, 1, ::testing::_, ::testing::_))
            .Times(2);
    EXPECT_CALL(*mock_provider, CollectCompletions(::testing::_))
            .Times(1)
            .WillOnce(::testing::Throw(ExceededMaxOutputTokensError()));

    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT " + GetFunctionName() + "("
                                            "{'model_name': 'gpt-4o', 'batch_size': 1, 'is_async': true}, "
                                            "{'prompt': 'Summarize', 'context_columns': [{'data': content}]}"
                                            ") AS result FROM unnest(['row-0', 'row-1']) AS tbl(content);");

    ASSERT_TRUE(!results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 2);
    EXPECT_EQ(results->GetValue(0, 0).GetValue<std::string>(), "null");
    EXPECT_EQ(results->GetValue(0, 1).GetValue<std::string>(), "null");
}

// Test llm_complete with audio transcription
TEST_F(LLMCompleteTest, LLMCompleteWithAudioTranscription) {
    const nlohmann::json expected_transcription = "{\"text\": \"This is a transcribed audio\"}";
    const nlohmann::json expected_complete_response = {{"items", {"Based on the transcription: This is a transcribed audio"}}};

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
            "SELECT llm_complete("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Summarize this audio', "
            "'context_columns': ["
            "{'data': audio_url, "
            "'type': 'audio', "
            "'transcription_model': 'gpt-4o-transcribe'}"
            "]}) AS result FROM VALUES ('https://example.com/audio.mp3') AS tbl(audio_url);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
}

// Test llm_complete with audio and text columns
TEST_F(LLMCompleteTest, LLMCompleteWithAudioAndText) {
    const nlohmann::json expected_transcription = "{\"text\": \"Product audio description\"}";
    const nlohmann::json expected_complete_response = {{"items", {"Combined response"}}};

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
            "SELECT llm_complete("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Describe this product', "
            "'context_columns': ["
            "{'data': product, 'name': 'product'}, "
            "{'data': audio_url, "
            "'type': 'audio', "
            "'transcription_model': 'gpt-4o-transcribe'}"
            "]}) AS result FROM VALUES ('Wireless Headphones', 'https://example.com/audio.mp3') AS tbl(product, audio_url);");

    ASSERT_FALSE(results->HasError()) << "Query failed: " << results->GetError();
    ASSERT_EQ(results->RowCount(), 1);
}

// Test audio transcription error handling
TEST_F(LLMCompleteTest, LLMCompleteAudioTranscriptionError) {
    auto con = Config::GetConnection();
    // Mock transcription model to throw error (simulating Ollama behavior)
    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .WillOnce(::testing::Throw(std::runtime_error("Audio transcription is not currently supported by Ollama.")));

    // Test with Ollama which doesn't support transcription
    const auto results = con.Query(
            "SELECT llm_complete("
            "{'model_name': 'gemma3:4b'}, "
            "{'prompt': 'Summarize this audio', "
            "'context_columns': ["
            "{'data': audio_url, "
            "'type': 'audio', "
            "'transcription_model': 'gemma3:4b'}"
            "]}) AS result FROM VALUES ('https://example.com/audio.mp3') AS tbl(audio_url);");

    // Should fail because Ollama doesn't support transcription
    ASSERT_TRUE(results->HasError());
}

// Test audio transcription with missing transcription_model
TEST_F(LLMCompleteTest, LLMCompleteAudioMissingTranscriptionModel) {
    auto con = Config::GetConnection();
    const auto results = con.Query(
            "SELECT llm_complete("
            "{'model_name': 'gpt-4o'}, "
            "{'prompt': 'Summarize this audio', "
            "'context_columns': ["
            "{'data': audio_url, "
            "'type': 'audio'}"
            "]}) AS result FROM VALUES ('https://example.com/audio.mp3') AS tbl(audio_url);");

    // Should fail because transcription_model is required for audio type
    ASSERT_TRUE(results->HasError());
}

}// namespace flock
