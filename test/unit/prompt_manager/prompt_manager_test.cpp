#include "../functions/mock_provider.hpp"
#include "flock/core/config.hpp"
#include "flock/model_manager/model.hpp"
#include "flock/prompt_manager/prompt_manager.hpp"
#include "nlohmann/json.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace flock {
using json = nlohmann::json;

// Test cases for PromptManager::ToString<PromptSection>
TEST(PromptManager, ToString) {
    EXPECT_EQ(PromptManager::ToString(PromptSection::USER_PROMPT), "{{USER_PROMPT}}");
    EXPECT_EQ(PromptManager::ToString(PromptSection::TUPLES), "{{TUPLES}}");
    EXPECT_EQ(PromptManager::ToString(PromptSection::RESPONSE_FORMAT), "{{RESPONSE_FORMAT}}");
    EXPECT_EQ(PromptManager::ToString(PromptSection::INSTRUCTIONS), "{{INSTRUCTIONS}}");
    // Test default/unknown case
    EXPECT_EQ(PromptManager::ToString(static_cast<PromptSection>(999)), "");
}

// Test cases for PromptManager::ReplaceSection with PromptSection enum
TEST(PromptManager, ReplaceSectionEnum) {
    auto prompt_template = "User: {{USER_PROMPT}}, Data: {{TUPLES}}, Format: {{RESPONSE_FORMAT}}";
    auto user_content = "Describe this table";
    auto tuple_content = "<tuple><col>1</col></tuple>";
    auto format_content = "JSON";

    auto result = PromptManager::ReplaceSection(prompt_template, PromptSection::USER_PROMPT, user_content);
    EXPECT_EQ(result, "User: Describe this table, Data: {{TUPLES}}, Format: {{RESPONSE_FORMAT}}");

    result = PromptManager::ReplaceSection(result, PromptSection::TUPLES, tuple_content);
    EXPECT_EQ(result, "User: Describe this table, Data: <tuple><col>1</col></tuple>, Format: {{RESPONSE_FORMAT}}");

    result = PromptManager::ReplaceSection(result, PromptSection::RESPONSE_FORMAT, format_content);
    EXPECT_EQ(result, "User: Describe this table, Data: <tuple><col>1</col></tuple>, Format: JSON");

    // Test replacing non-existent section
    result = PromptManager::ReplaceSection(result, PromptSection::INSTRUCTIONS, "Do nothing");
    EXPECT_EQ(result, "User: Describe this table, Data: <tuple><col>1</col></tuple>, Format: JSON");

    // Test multiple replacements
    auto multi_template = "{{USER_PROMPT}} then {{USER_PROMPT}}";
    result = PromptManager::ReplaceSection(multi_template, PromptSection::USER_PROMPT, "Repeat");
    EXPECT_EQ(result, "Repeat then Repeat");
}

// Test cases for PromptManager::ReplaceSection with string target
TEST(PromptManager, ReplaceSectionString) {
    auto prompt_template = "Replace [this] and [this] but not [that].";
    auto replace_target = "[this]";
    auto replace_content = "THAT";

    auto result = PromptManager::ReplaceSection(prompt_template, replace_target, replace_content);
    EXPECT_EQ(result, "Replace THAT and THAT but not [that].");

    // Test replacing with empty string
    result = PromptManager::ReplaceSection(result, "THAT", "");
    EXPECT_EQ(result, "Replace  and  but not [that].");

    // Test replacing non-existent target
    result = PromptManager::ReplaceSection(result, "[notfound]", "XXX");
    EXPECT_EQ(result, "Replace  and  but not [that].");
}

TEST(PromptManager, ConstructInputTuplesHeader) {
    auto tuple = json::array();
    tuple.push_back({{"name", "Header 1"}});
    tuple.push_back({{"name", "Header 2"}});

    // XML
    auto xml_header = PromptManager::ConstructInputTuplesHeader(tuple, "xml");
    EXPECT_EQ(xml_header, "<header><column>Header 1</column><column>Header 2</column></header>\n");

    // Markdown
    auto md_header = PromptManager::ConstructInputTuplesHeader(tuple, "markdown");
    EXPECT_EQ(md_header, " | COLUMN_Header 1 | COLUMN_Header 2 | \n | -------- | -------- | \n");

    // JSON (should be empty)
    auto json_header = PromptManager::ConstructInputTuplesHeader(tuple, "json");
    EXPECT_EQ(json_header, "");

    // Invalid format
    EXPECT_THROW(PromptManager::ConstructInputTuplesHeader(tuple, "invalid_format"), std::runtime_error);
}

TEST(PromptManager, ConstructInputTuplesHeaderEmpty) {
    auto tuple = json::array();
    tuple.push_back({{"data", {}}});
    tuple.push_back({{"data", {}}});

    // XML
    auto xml_header = PromptManager::ConstructInputTuplesHeader(tuple, "xml");
    EXPECT_EQ(xml_header, "<header><column>COLUMN 1</column><column>COLUMN 2</column></header>\n");

    // Markdown
    auto md_header = PromptManager::ConstructInputTuplesHeader(tuple, "markdown");
    EXPECT_EQ(md_header, " | COLUMN 1 | COLUMN 2 | \n | -------- | -------- | \n");

    // JSON (should be empty)
    auto json_header = PromptManager::ConstructInputTuplesHeader(tuple, "json");
    EXPECT_EQ(json_header, "");

    // Invalid format
    EXPECT_THROW(PromptManager::ConstructInputTuplesHeader(tuple, "invalid_format"), std::runtime_error);
}

// Test cases for PromptManager::ConstructNumTuples
TEST(PromptManager, ConstructNumTuples) {
    EXPECT_EQ(PromptManager::ConstructNumTuples(0), "- The Number of Tuples to Generate Responses for: 0\n\n");
    EXPECT_EQ(PromptManager::ConstructNumTuples(5), "- The Number of Tuples to Generate Responses for: 5\n\n");
    EXPECT_EQ(PromptManager::ConstructNumTuples(123), "- The Number of Tuples to Generate Responses for: 123\n\n");
}

// Test cases for PromptManager::ConstructInputTuples
TEST(PromptManager, ConstructInputTuples) {
    auto tuples = json::array();
    tuples.push_back({{"data", {"row1A", "row2A"}}});
    tuples.push_back({{"data", {"1", "2"}}});

    // XML
    auto xml_expected = std::string("- The Number of Tuples to Generate Responses for: 2\n\n");
    xml_expected += "<header><column>COLUMN 1</column><column>COLUMN 2</column></header>\n";
    xml_expected += "<row><column>row1A</column><column>1</column></row>\n";
    xml_expected += "<row><column>row2A</column><column>2</column></row>\n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(tuples, "xml"), xml_expected);

    // Markdown
    auto md_expected = std::string("- The Number of Tuples to Generate Responses for: 2\n\n");
    md_expected += " | COLUMN 1 | COLUMN 2 | \n | -------- | -------- | \n";
    md_expected += " | \"row1A\" | \"1\" | \n";
    md_expected += " | \"row2A\" | \"2\" | \n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(tuples, "markdown"), md_expected);

    // JSON
    auto json_expected = std::string("- The Number of Tuples to Generate Responses for: 2\n\n");
    auto expected_tuples_json = nlohmann::json::object();
    auto column_idx = 1u;
    for (const auto& column: tuples) {
        auto column_name = column.contains("name") ? column["name"].get<std::string>() : "COLUMN " + std::to_string(column_idx++);
        expected_tuples_json[column_name] = column["data"];
    }

    json_expected += expected_tuples_json.dump(4);
    json_expected += "\n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(tuples, "json"), json_expected);

    // Invalid format
    EXPECT_THROW(PromptManager::ConstructInputTuples(tuples, "invalid_format"), std::runtime_error);
}

// Test case for an empty tuples array
TEST(PromptManager, ConstructInputTuplesEmpty) {
    const json empty_tuples = json::array();

    // Empty tuples - XML
    auto xml_expected = std::string("- The Number of Tuples to Generate Responses for: 0\n\n");
    xml_expected += "<header></header>\n<row></row>\n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(empty_tuples, "xml"), xml_expected);

    // Empty tuples - Markdown
    auto md_expected = std::string("- The Number of Tuples to Generate Responses for: 0\n\n");
    md_expected += " | Empty | \n | ----- | \n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(empty_tuples, "markdown"), md_expected);

    // Empty tuples - JSON
    auto json_expected = "- The Number of Tuples to Generate Responses for: 0\n\n{}\n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(empty_tuples, "json"), json_expected);
}

TEST(PromptManager, CreatePromptDetailsLiteralPrompt) {
    const json prompt_json = {{"prompt", "test_prompt"}};
    const auto [prompt_name, prompt, version] = PromptManager::CreatePromptDetails(prompt_json);
    EXPECT_EQ(prompt, "test_prompt");
    EXPECT_EQ(prompt_name, "");
    EXPECT_EQ(version, -1);
}

TEST(PromptManager, CreatePromptDetailsUnvalidArgs) {
    const json prompt_json = {{"invalid_key", "test_prompt"}};
    EXPECT_THROW(PromptManager::CreatePromptDetails(prompt_json), std::runtime_error);
}

// Test with empty JSON object
TEST(PromptManager, CreatePromptDetailsEmptyJson) {
    const json empty_json = json::object();
    EXPECT_THROW(PromptManager::CreatePromptDetails(empty_json), std::runtime_error);
}

// Test with prompt_name and a specific version
TEST(PromptManager, CreatePromptDetailsWithExplicitVersion) {
    const json prompt_json = {
            {"prompt_name", "product_summary"},
            {"version", "4"}};

    const auto [prompt_name, prompt, version] = PromptManager::CreatePromptDetails(prompt_json);
    EXPECT_EQ(prompt_name, "product_summary");
    EXPECT_EQ(prompt, "Summarize the product with a persuasive tone suitable for a sales page.");
    EXPECT_EQ(version, 4);
}

// Test with a non-existent prompt name
TEST(PromptManager, CreatePromptDetailsNonExistentPrompt) {
    const json prompt_json = {{"prompt_name", "non_existent_prompt"}};
    EXPECT_THROW(PromptManager::CreatePromptDetails(prompt_json), std::runtime_error);
}

// Test with a non-existent version of existing prompt
TEST(PromptManager, CreatePromptDetailsNonExistentVersion) {
    const json prompt_json = {
            {"prompt_name", "product_summary"},
            {"version", "999"}};

    EXPECT_THROW(PromptManager::CreatePromptDetails(prompt_json), std::runtime_error);
}

// Test with too many fields in the JSON for a prompt_name case
TEST(PromptManager, CreatePromptDetailsTooManyFieldsWithPromptName) {
    const json prompt_json = {
            {"prompt_name", "product_summary"},
            {"extra_field", "value"},
            {"another_field", "value"}};

    EXPECT_THROW(PromptManager::CreatePromptDetails(prompt_json), std::runtime_error);
}

// Test with too many fields in the JSON for prompt_name with a version case
TEST(PromptManager, CreatePromptDetailsTooManyFieldsWithVersion) {
    const json prompt_json = {
            {"prompt_name", "product_summary"},
            {"version", "5"},
            {"extra_field", "value"}};

    EXPECT_THROW(PromptManager::CreatePromptDetails(prompt_json), std::runtime_error);
}

// Test with multiple fields in a prompt-only case
TEST(PromptManager, CreatePromptDetailsMultipleFieldsPromptOnly) {
    const json prompt_json = {
            {"prompt", "test_prompt"},
            {"extra_field", "this should be ignored"}};
    EXPECT_THROW(PromptManager::CreatePromptDetails(prompt_json), std::runtime_error);
}

TEST(PromptManager, CreatePromptDetailsOnlyPromptName) {
    const json prompt_json = {{"prompt_name", "product_summary"}};
    const auto [prompt_name, prompt, version] = PromptManager::CreatePromptDetails(prompt_json);
    EXPECT_EQ(prompt_name, "product_summary");
    EXPECT_EQ(prompt, "Generate a summary with a focus on technical specifications.");
    EXPECT_EQ(version, 6);
}

// Test fixture for TranscribeAudioColumn tests
class TranscribeAudioColumnTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto con = Config::GetConnection();
        con.Query(" CREATE SECRET ("
                  "       TYPE OPENAI,"
                  "    API_KEY 'your-api-key');");
        con.Query("  CREATE SECRET ("
                  "       TYPE OLLAMA,"
                  "    API_URL '127.0.0.1:11434');");

        mock_provider = std::make_shared<MockProvider>(ModelDetails{});
        Model::SetMockProvider(mock_provider);
    }

    void TearDown() override {
        Model::ResetMockProvider();
        mock_provider = nullptr;
    }

    std::shared_ptr<MockProvider> mock_provider;
};

// Test TranscribeAudioColumn with named column
TEST_F(TranscribeAudioColumnTest, TranscribeAudioColumnWithName) {
    json audio_column = {
            {"name", "audio_review"},
            {"type", "audio"},
            {"transcription_model", "gpt-4o-transcribe"},
            {"data", {"https://example.com/audio1.mp3", "https://example.com/audio2.mp3"}}};

    json expected_transcription1 = "{\"text\": \"This is the first transcription\"}";
    json expected_transcription2 = "{\"text\": \"This is the second transcription\"}";

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription1, expected_transcription2}));

    auto result = PromptManager::TranscribeAudioColumn(audio_column);

    EXPECT_TRUE(result.contains("name"));
    EXPECT_EQ(result["name"], "transcription_of_audio_review");
    EXPECT_TRUE(result.contains("data"));
    EXPECT_TRUE(result["data"].is_array());
    EXPECT_EQ(result["data"].size(), 2);
    EXPECT_EQ(result["data"][0], expected_transcription1);
    EXPECT_EQ(result["data"][1], expected_transcription2);
}

// Test TranscribeAudioColumn without name
TEST_F(TranscribeAudioColumnTest, TranscribeAudioColumnWithoutName) {
    json audio_column = {
            {"type", "audio"},
            {"transcription_model", "gpt-4o-transcribe"},
            {"data", {"https://example.com/audio.mp3"}}};

    json expected_transcription = "{\"text\": \"Transcribed audio content\"}";

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription}));

    auto result = PromptManager::TranscribeAudioColumn(audio_column);

    EXPECT_TRUE(result.contains("name"));
    EXPECT_EQ(result["name"], "transcription");
    EXPECT_TRUE(result.contains("data"));
    EXPECT_TRUE(result["data"].is_array());
    EXPECT_EQ(result["data"].size(), 1);
    EXPECT_EQ(result["data"][0], expected_transcription);
}

// Test TranscribeAudioColumn with empty name
TEST_F(TranscribeAudioColumnTest, TranscribeAudioColumnWithEmptyName) {
    json audio_column = {
            {"name", ""},
            {"type", "audio"},
            {"transcription_model", "gpt-4o-transcribe"},
            {"data", {"https://example.com/audio.mp3"}}};

    json expected_transcription = "{\"text\": \"Transcribed content\"}";

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription}));

    auto result = PromptManager::TranscribeAudioColumn(audio_column);

    EXPECT_TRUE(result.contains("name"));
    EXPECT_EQ(result["name"], "transcription");
    EXPECT_TRUE(result.contains("data"));
    EXPECT_EQ(result["data"].size(), 1);
}

// Test TranscribeAudioColumn with single audio file
TEST_F(TranscribeAudioColumnTest, TranscribeAudioColumnSingleFile) {
    json audio_column = {
            {"name", "podcast"},
            {"type", "audio"},
            {"transcription_model", "gpt-4o-transcribe"},
            {"data", {"https://example.com/podcast.mp3"}}};

    json expected_transcription = "{\"text\": \"Podcast transcription\"}";

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription}));

    auto result = PromptManager::TranscribeAudioColumn(audio_column);

    EXPECT_EQ(result["name"], "transcription_of_podcast");
    EXPECT_EQ(result["data"].size(), 1);
    EXPECT_EQ(result["data"][0], expected_transcription);
}

// Test TranscribeAudioColumn with multiple audio files
TEST_F(TranscribeAudioColumnTest, TranscribeAudioColumnMultipleFiles) {
    json audio_column = {
            {"name", "interviews"},
            {"type", "audio"},
            {"transcription_model", "gpt-4o-transcribe"},
            {"data", {"https://example.com/interview1.mp3", "https://example.com/interview2.mp3", "https://example.com/interview3.mp3"}}};

    json expected_transcription1 = "{\"text\": \"First interview\"}";
    json expected_transcription2 = "{\"text\": \"Second interview\"}";
    json expected_transcription3 = "{\"text\": \"Third interview\"}";

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription1, expected_transcription2, expected_transcription3}));

    auto result = PromptManager::TranscribeAudioColumn(audio_column);

    EXPECT_EQ(result["name"], "transcription_of_interviews");
    EXPECT_EQ(result["data"].size(), 3);
    EXPECT_EQ(result["data"][0], expected_transcription1);
    EXPECT_EQ(result["data"][1], expected_transcription2);
    EXPECT_EQ(result["data"][2], expected_transcription3);
}

// Test TranscribeAudioColumn output format (JSON array)
TEST_F(TranscribeAudioColumnTest, TranscribeAudioColumnOutputFormat) {
    json audio_column = {
            {"name", "test_audio"},
            {"type", "audio"},
            {"transcription_model", "gpt-4o-transcribe"},
            {"data", {"https://example.com/audio.mp3"}}};

    json expected_transcription = "{\"text\": \"Test transcription\"}";

    EXPECT_CALL(*mock_provider, AddTranscriptionRequest(::testing::_))
            .Times(1);
    EXPECT_CALL(*mock_provider, CollectTranscriptions("multipart/form-data"))
            .WillOnce(::testing::Return(std::vector<nlohmann::json>{expected_transcription}));

    auto result = PromptManager::TranscribeAudioColumn(audio_column);

    // Verify the result is a proper JSON object with name and data fields
    EXPECT_TRUE(result.is_object());
    EXPECT_TRUE(result.contains("name"));
    EXPECT_TRUE(result.contains("data"));
    EXPECT_TRUE(result["data"].is_array());

    // Verify data contains the transcription results
    EXPECT_EQ(result["data"][0], expected_transcription);
}

}// namespace flock