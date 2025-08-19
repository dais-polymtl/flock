#include "flockmtl/prompt_manager/prompt_manager.hpp"
#include "nlohmann/json.hpp"
#include <gtest/gtest.h>
#include <string>

namespace flockmtl {
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

}// namespace flockmtl