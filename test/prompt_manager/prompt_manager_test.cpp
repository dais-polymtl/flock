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

// Test cases for PromptManager::ConstructInputTuplesHeader
TEST(PromptManager, ConstructInputTuplesHeader) {
    json tuple = {{{"col1", "val1"}, {"col2", 123}}};

    // XML
    auto xml_header = PromptManager::ConstructInputTuplesHeader(tuple, "xml");
    EXPECT_EQ(xml_header, "<tuple><col>col1</col><col>col2</col></tuple>\n");

    // Markdown
    auto md_header = PromptManager::ConstructInputTuplesHeader(tuple, "markdown");
    EXPECT_EQ(md_header, "|col1|col2|\n|---|---|\n");

    // JSON (should be empty)
    auto json_header = PromptManager::ConstructInputTuplesHeader(tuple, "json");
    EXPECT_EQ(json_header, "");

    // Invalid format
    EXPECT_THROW(PromptManager::ConstructInputTuplesHeader(tuple, "invalid_format"), std::runtime_error);
}

// Test cases for PromptManager::ConstructSingleInputTuple
TEST(PromptManager, ConstructSingleInputTuple) {
    json tuple = {{"col1", "string val"}, {"col2", 456}, {"col3", true}};

    // XML
    auto xml_tuple = PromptManager::ConstructSingleInputTuple(tuple, "xml");
    EXPECT_EQ(xml_tuple, "<tuple><col>\"string val\"</col><col>456</col><col>true</col></tuple>\n");

    // Markdown
    auto md_tuple = PromptManager::ConstructSingleInputTuple(tuple, "markdown");
    EXPECT_EQ(md_tuple, "|\"string val\"|456|true|\n");

    // JSON
    auto json_tuple_str = PromptManager::ConstructSingleInputTuple(tuple, "json");
    EXPECT_EQ(json_tuple_str, tuple.dump() + "\n");

    // Invalid format
    EXPECT_THROW(PromptManager::ConstructSingleInputTuple(tuple, "invalid_format"), std::runtime_error);
}

// Test cases for PromptManager::ConstructNumTuples
TEST(PromptManager, ConstructNumTuples) {
    EXPECT_EQ(PromptManager::ConstructNumTuples(0), "- The Number of Tuples to Generate Responses for: 0\n\n");
    EXPECT_EQ(PromptManager::ConstructNumTuples(5), "- The Number of Tuples to Generate Responses for: 5\n\n");
    EXPECT_EQ(PromptManager::ConstructNumTuples(123), "- The Number of Tuples to Generate Responses for: 123\n\n");
}

// Test cases for PromptManager::ConstructInputTuples
TEST(PromptManager, ConstructInputTuples) {
    json tuples = json::array({{{"colA", "row1A"}, {"colB", 1}},
                               {{"colA", "row2A"}, {"colB", 2}}});

    // XML
    auto xml_expected = std::string("- The Number of Tuples to Generate Responses for: 2\n\n");
    xml_expected += "<tuple><col>colA</col><col>colB</col></tuple>\n";
    xml_expected += "<tuple><col>\"row1A\"</col><col>1</col></tuple>\n";
    xml_expected += "<tuple><col>\"row2A\"</col><col>2</col></tuple>\n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(tuples, "xml"), xml_expected);

    // Markdown
    auto md_expected = std::string("- The Number of Tuples to Generate Responses for: 2\n\n");
    md_expected += "|colA|colB|\n|---|---|\n";
    md_expected += "|\"row1A\"|1|\n";
    md_expected += "|\"row2A\"|2|\n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(tuples, "markdown"), md_expected);

    // JSON
    auto json_expected = std::string("- The Number of Tuples to Generate Responses for: 2\n\n");
    // No header for JSON
    json_expected += tuples[0].dump() + "\n";
    json_expected += tuples[1].dump() + "\n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(tuples, "json"), json_expected);

    // Invalid format
    EXPECT_THROW(PromptManager::ConstructInputTuples(tuples, "invalid_format"), std::runtime_error);
}

// Test case for an empty tuples array
TEST(PromptManager, ConstructInputTuplesEmpty) {
    json empty_tuples = json::array();

    // Empty tuples - XML
    auto xml_expected = std::string("- The Number of Tuples to Generate Responses for: 0\n\n");
    xml_expected += "<tuple>Empty</tuple>\n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(empty_tuples, "xml"), xml_expected);

    // Empty tuples - Markdown
    auto md_expected = std::string("- The Number of Tuples to Generate Responses for: 0\n\n");
    md_expected += "| Empty |\n|---|\n";
    EXPECT_EQ(PromptManager::ConstructInputTuples(empty_tuples, "markdown"), md_expected);

    // Empty tuples - JSON
    auto json_expected = "- The Number of Tuples to Generate Responses for: 0\n\n";
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