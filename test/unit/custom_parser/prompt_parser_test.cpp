#include "flockmtl/custom_parser/query/prompt_parser.hpp"
#include "flockmtl/custom_parser/tokenizer.hpp"
#include "gtest/gtest.h"
#include <memory>

using namespace flockmtl;

/**************************************************
 *                 Create Prompt                  *
 **************************************************/

TEST(PromptParserTest, ParseCreatePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE PROMPT ('test_prompt', 'This is a test prompt.')", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
}

TEST(PromptParserTest, ParseCreatePromptWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE PROMPT ('test_prompt', 'This is a test prompt.');", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
}

TEST(PromptParserTest, ParseCreatePromptWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE PROMPT ('test_prompt', 'This is a test prompt.') -- This is a comment", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
}

TEST(PromptParserTest, ParseCreatePromptWithCommentAndSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE PROMPT ('test_prompt', 'This is a test prompt.'); -- This is a comment", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
}

TEST(PromptParserTest, ParseCreatePromptWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("-- This is a comment before the query\nCREATE PROMPT ('test_prompt', 'This is a test prompt.')", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
}

TEST(PromptParserTest, ParseCreateGlobalPrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE GLOBAL PROMPT ('test_prompt', 'This is a test prompt.')", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
    EXPECT_EQ(create_stmt->catalog, "flockmtl_storage.");
}

TEST(PromptParserTest, ParseCreateGlobalPromptWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE GLOBAL PROMPT ('test_prompt', 'This is a test prompt.');", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
    EXPECT_EQ(create_stmt->catalog, "flockmtl_storage.");
}

TEST(PromptParserTest, ParseCreateGlobalPromptWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE GLOBAL PROMPT ('test_prompt', 'This is a test prompt.') -- Global prompt comment", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
    EXPECT_EQ(create_stmt->catalog, "flockmtl_storage.");
}

TEST(PromptParserTest, ParseCreateLocalPrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE LOCAL PROMPT ('test_prompt', 'This is a test prompt.')", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
    EXPECT_EQ(create_stmt->catalog, "");
}

TEST(PromptParserTest, ParseCreateLocalPromptWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE LOCAL PROMPT ('test_prompt', 'This is a test prompt.');", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
    EXPECT_EQ(create_stmt->catalog, "");
}

TEST(PromptParserTest, ParseCreateLocalPromptWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE LOCAL PROMPT ('test_prompt', 'This is a test prompt.') -- Local prompt comment", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
    EXPECT_EQ(create_stmt->catalog, "");
}

TEST(PromptParserTest, ParseInvalidCreatePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("CREATE PROMPT ('test_prompt')", statement), std::runtime_error);
}

TEST(PromptParserTest, ParseInvalidCreatePromptWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("CREATE PROMPT ('test_prompt') -- Invalid query comment", statement), std::runtime_error);
}

/**************************************************
 *                 Delete Prompt                  *
 **************************************************/

TEST(PromptParserTest, ParseDeletePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("DELETE PROMPT 'test_prompt';", statement));
    ASSERT_NE(statement, nullptr);
    auto delete_stmt = dynamic_cast<DeletePromptStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseDeletePromptWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("DELETE PROMPT 'test_prompt'", statement));
    ASSERT_NE(statement, nullptr);
    auto delete_stmt = dynamic_cast<DeletePromptStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseDeletePromptWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("DELETE PROMPT 'test_prompt'; -- Delete prompt comment", statement));
    ASSERT_NE(statement, nullptr);
    auto delete_stmt = dynamic_cast<DeletePromptStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseDeletePromptWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Delete the test prompt\nDELETE PROMPT 'test_prompt'", statement));
    ASSERT_NE(statement, nullptr);
    auto delete_stmt = dynamic_cast<DeletePromptStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseDeletePromptWithCommentWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("DELETE PROMPT 'test_prompt' -- Delete prompt comment", statement));
    ASSERT_NE(statement, nullptr);
    auto delete_stmt = dynamic_cast<DeletePromptStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseInvalidDeletePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("DELETE PROMPT", statement), std::runtime_error);
}

TEST(PromptParserTest, ParseInvalidDeletePromptWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("DELETE PROMPT -- Invalid delete comment", statement), std::runtime_error);
}

/**************************************************
 *                 Update Prompt                  *
 **************************************************/

TEST(PromptParserTest, ParseUpdatePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT ('test_prompt', 'Updated prompt text.')", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->new_prompt, "Updated prompt text.");
}

TEST(PromptParserTest, ParseUpdatePromptWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT ('test_prompt', 'Updated prompt text.');", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->new_prompt, "Updated prompt text.");
}

TEST(PromptParserTest, ParseUpdatePromptWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT ('test_prompt', 'Updated prompt text.'); -- Update prompt comment", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->new_prompt, "Updated prompt text.");
}

TEST(PromptParserTest, ParseUpdatePromptWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Update the prompt text\nUPDATE PROMPT ('test_prompt', 'Updated prompt text.')", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->new_prompt, "Updated prompt text.");
}

TEST(PromptParserTest, ParseUpdatePromptWithCommentWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT ('test_prompt', 'Updated prompt text.') -- Update prompt comment", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->new_prompt, "Updated prompt text.");
}

TEST(PromptParserTest, ParseUpdateScopeToGlobal) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT 'test_prompt' TO GLOBAL;", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->catalog, "flockmtl_storage.");
}

TEST(PromptParserTest, ParseUpdateScopeToGlobalWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT 'test_prompt' TO GLOBAL", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->catalog, "flockmtl_storage.");
}

TEST(PromptParserTest, ParseUpdateScopeToGlobalWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT 'test_prompt' TO GLOBAL; -- Make prompt global", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->catalog, "flockmtl_storage.");
}

TEST(PromptParserTest, ParseUpdateScopeToLocal) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT 'test_prompt' TO LOCAL;", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->catalog, "");
}

TEST(PromptParserTest, ParseUpdateScopeToLocalWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT 'test_prompt' TO LOCAL", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->catalog, "");
}

TEST(PromptParserTest, ParseUpdateScopeToLocalWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT 'test_prompt' TO LOCAL; -- Make prompt local", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->catalog, "");
}

TEST(PromptParserTest, ParseInvalidUpdatePrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("UPDATE PROMPT ('test_prompt')", statement), std::runtime_error);
}

TEST(PromptParserTest, ParseInvalidUpdatePromptWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("UPDATE PROMPT ('test_prompt') -- Invalid update comment", statement), std::runtime_error);
}

/**************************************************
 *                   Get Prompt                   *
 **************************************************/

TEST(PromptParserTest, ParseGetPrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("GET PROMPT 'test_prompt';", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetPromptStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseGetPromptWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("GET PROMPT 'test_prompt'", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetPromptStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseGetPromptWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("GET PROMPT 'test_prompt'; -- Get prompt comment", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetPromptStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseGetPromptWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Retrieve the test prompt\nGET PROMPT 'test_prompt'", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetPromptStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseGetPromptWithCommentWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("GET PROMPT 'test_prompt' -- Get prompt comment", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetPromptStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseInvalidGetPrompt) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("GET PROMPT", statement), std::runtime_error);
}

TEST(PromptParserTest, ParseInvalidGetPromptWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("GET PROMPT -- Invalid get comment", statement), std::runtime_error);
}

/**************************************************
 *                Get All Prompts                 *
 **************************************************/

TEST(PromptParserTest, ParseGetAllPrompts) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("GET PROMPTS;", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllPromptStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

TEST(PromptParserTest, ParseGetAllPromptsWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("GET PROMPTS", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllPromptStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

TEST(PromptParserTest, ParseGetAllPromptsWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("GET PROMPTS; -- Get all prompts comment", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllPromptStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

TEST(PromptParserTest, ParseGetAllPromptsWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Retrieve all prompts\nGET PROMPTS", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllPromptStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

TEST(PromptParserTest, ParseGetAllPromptsWithCommentWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("GET PROMPTS -- Get all prompts comment", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllPromptStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

/**************************************************
 *              Complex Comment Tests             *
 **************************************************/

TEST(PromptParserTest, ParseCreatePromptWithMultipleComments) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Create a new prompt\n-- This is a test prompt\nCREATE PROMPT ('test_prompt', 'This is a test prompt.') -- End comment", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreatePromptStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(create_stmt->prompt, "This is a test prompt.");
}

TEST(PromptParserTest, ParseUpdatePromptWithInlineComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE PROMPT ('test_prompt', 'Updated prompt text.') -- Update with new text", statement));
    ASSERT_NE(statement, nullptr);
    auto update_stmt = dynamic_cast<UpdatePromptStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->prompt_name, "test_prompt");
    EXPECT_EQ(update_stmt->new_prompt, "Updated prompt text.");
}

TEST(PromptParserTest, ParseDeletePromptWithDetailedComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Remove the test prompt from storage\nDELETE PROMPT 'test_prompt'; -- Cleanup operation", statement));
    ASSERT_NE(statement, nullptr);
    auto delete_stmt = dynamic_cast<DeletePromptStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->prompt_name, "test_prompt");
}

TEST(PromptParserTest, ParseGetPromptWithDocumentationComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Retrieve prompt for analysis\n-- This prompt is used in the main application\nGET PROMPT 'test_prompt' -- Get for processing", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetPromptStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->prompt_name, "test_prompt");
}

/**************************************************
 *              Error Handling Tests              *
 **************************************************/

TEST(PromptParserTest, ParseEmptyQuery) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("", statement), std::runtime_error);
}

TEST(PromptParserTest, ParseOnlyComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("-- This is only a comment", statement), std::runtime_error);
}

TEST(PromptParserTest, ParseOnlyWhitespace) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("   \n\t  ", statement), std::runtime_error);
}

TEST(PromptParserTest, ParseInvalidKeyword) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("INVALID COMMAND", statement), std::runtime_error);
}

TEST(PromptParserTest, ParseInvalidKeywordWithComment) {
    std::unique_ptr<QueryStatement> statement;
    PromptParser parser;
    EXPECT_THROW(parser.Parse("INVALID COMMAND -- This should fail", statement), std::runtime_error);
}