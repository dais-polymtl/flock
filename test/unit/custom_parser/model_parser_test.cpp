#include "flockmtl/custom_parser/query/model_parser.hpp"
#include "flockmtl/custom_parser/tokenizer.hpp"
#include "gtest/gtest.h"
#include <memory>

using namespace flockmtl;

/**************************************************
 *                 Create Model                  *
 **************************************************/

TEST(ModelParserTest, ParseCreateModelWithoutModelArgs) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider')", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args.size(), 0);
}

TEST(ModelParserTest, ParseCreateModelWithoutModelArgsWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider');", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args.size(), 0);
}

TEST(ModelParserTest, ParseCreateModelWithoutModelArgsWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider') -- Simple model creation", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args.size(), 0);
}

TEST(ModelParserTest, ParseCreateModelWithoutModelArgsWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Create a simple model\nCREATE MODEL ('test_model', 'model_data', 'provider')", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args.size(), 0);
}

TEST(ModelParserTest, ParseCreateModelWithoutModelArgsWithCommentAndSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider'); -- Simple model creation", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args.size(), 0);
}

TEST(ModelParserTest, ParseCreateGlobalModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE GLOBAL MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}})", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
    EXPECT_EQ(create_stmt->catalog, "flockmtl_storage.");
}

TEST(ModelParserTest, ParseCreateGlobalModelWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE GLOBAL MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}});", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
    EXPECT_EQ(create_stmt->catalog, "flockmtl_storage.");
}

TEST(ModelParserTest, ParseCreateGlobalModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE GLOBAL MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}}) -- Global model with args", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
    EXPECT_EQ(create_stmt->catalog, "flockmtl_storage.");
}

TEST(ModelParserTest, ParseCreateLocalModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE LOCAL MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}})", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
    EXPECT_EQ(create_stmt->catalog, "");
}

TEST(ModelParserTest, ParseCreateLocalModelWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE LOCAL MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}});", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
    EXPECT_EQ(create_stmt->catalog, "");
}

TEST(ModelParserTest, ParseCreateLocalModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE LOCAL MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}}) -- Local model with args", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
    EXPECT_EQ(create_stmt->catalog, "");
}

TEST(ModelParserTest, ParseCreateModelWithArgs) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}})", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
}

TEST(ModelParserTest, ParseCreateModelWithArgsWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}});", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
}

TEST(ModelParserTest, ParseCreateModelWithArgsWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}}) -- Model with parameters", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
}

TEST(ModelParserTest, ParseCreateModelWithArgsWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Create model with configuration\nCREATE MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}})", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
}

TEST(ModelParserTest, ParseInvalidCreateModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data')", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseInvalidCreateModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data') -- Invalid model creation", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseStringBatchSizeCreateModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": \"32\", \"model_parameters\": {\"param1\": \"value1\"}})", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseStringBatchSizeCreateModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": \"32\", \"model_parameters\": {\"param1\": \"value1\"}}) -- Invalid batch size", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseCreateModelWithInvalidArgs) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider', {\"invalid_key\": \"value\"})", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseCreateModelWithInvalidArgsWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("CREATE MODEL ('test_model', 'model_data', 'provider', {\"invalid_key\": \"value\"}) -- Invalid args", statement), std::runtime_error);
}

/**************************************************
 *                 Delete Model                  *
 **************************************************/

TEST(ModelParserTest, ParseDeleteModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("DELETE MODEL 'test_model';", statement));
    ASSERT_NE(statement, nullptr);
    const auto delete_stmt = dynamic_cast<DeleteModelStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseDeleteModelWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("DELETE MODEL 'test_model'", statement));
    ASSERT_NE(statement, nullptr);
    const auto delete_stmt = dynamic_cast<DeleteModelStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseDeleteModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("DELETE MODEL 'test_model'; -- Delete the test model", statement));
    ASSERT_NE(statement, nullptr);
    const auto delete_stmt = dynamic_cast<DeleteModelStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseDeleteModelWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Remove the test model\nDELETE MODEL 'test_model'", statement));
    ASSERT_NE(statement, nullptr);
    const auto delete_stmt = dynamic_cast<DeleteModelStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseDeleteModelWithCommentWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("DELETE MODEL 'test_model' -- Delete the test model", statement));
    ASSERT_NE(statement, nullptr);
    const auto delete_stmt = dynamic_cast<DeleteModelStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseInvalidDeleteModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("DELETE MODEL", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseInvalidDeleteModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("DELETE MODEL -- Invalid delete", statement), std::runtime_error);
}

/**************************************************
 *                 Update Model                  *
 **************************************************/

TEST(ModelParserTest, ParseUpdateModelWithoutModelArgs) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider')", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->new_model, "new_model_data");
    EXPECT_EQ(create_stmt->provider_name, "new_provider");
    EXPECT_EQ(create_stmt->new_model_args.size(), 0);
}

TEST(ModelParserTest, ParseUpdateModelWithoutModelArgsWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider');", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->new_model, "new_model_data");
    EXPECT_EQ(create_stmt->provider_name, "new_provider");
    EXPECT_EQ(create_stmt->new_model_args.size(), 0);
}

TEST(ModelParserTest, ParseUpdateModelWithoutModelArgsWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider') -- Update without args", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->new_model, "new_model_data");
    EXPECT_EQ(create_stmt->provider_name, "new_provider");
    EXPECT_EQ(create_stmt->new_model_args.size(), 0);
}

TEST(ModelParserTest, ParseUpdateModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider', {\"tuple_format\": \"xml\", \"batch_size\": 64, \"model_parameters\": {\"param2\": \"value2\"}})", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->new_model, "new_model_data");
    EXPECT_EQ(update_stmt->provider_name, "new_provider");
    EXPECT_EQ(update_stmt->new_model_args["tuple_format"], "xml");
    EXPECT_EQ(update_stmt->new_model_args["batch_size"], 64);
    EXPECT_EQ(update_stmt->new_model_args["model_parameters"].at("param2"), "value2");
}

TEST(ModelParserTest, ParseUpdateModelWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider', {\"tuple_format\": \"xml\", \"batch_size\": 64, \"model_parameters\": {\"param2\": \"value2\"}});", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->new_model, "new_model_data");
    EXPECT_EQ(update_stmt->provider_name, "new_provider");
    EXPECT_EQ(update_stmt->new_model_args["tuple_format"], "xml");
    EXPECT_EQ(update_stmt->new_model_args["batch_size"], 64);
    EXPECT_EQ(update_stmt->new_model_args["model_parameters"].at("param2"), "value2");
}

TEST(ModelParserTest, ParseUpdateModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider', {\"tuple_format\": \"xml\", \"batch_size\": 64, \"model_parameters\": {\"param2\": \"value2\"}}) -- Update with new args", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->new_model, "new_model_data");
    EXPECT_EQ(update_stmt->provider_name, "new_provider");
    EXPECT_EQ(update_stmt->new_model_args["tuple_format"], "xml");
    EXPECT_EQ(update_stmt->new_model_args["batch_size"], 64);
    EXPECT_EQ(update_stmt->new_model_args["model_parameters"].at("param2"), "value2");
}

TEST(ModelParserTest, ParseUpdateModelWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Update model with new configuration\nUPDATE MODEL ('test_model', 'new_model_data', 'new_provider', {\"tuple_format\": \"xml\", \"batch_size\": 64, \"model_parameters\": {\"param2\": \"value2\"}})", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->new_model, "new_model_data");
    EXPECT_EQ(update_stmt->provider_name, "new_provider");
    EXPECT_EQ(update_stmt->new_model_args["tuple_format"], "xml");
    EXPECT_EQ(update_stmt->new_model_args["batch_size"], 64);
    EXPECT_EQ(update_stmt->new_model_args["model_parameters"].at("param2"), "value2");
}

TEST(ModelParserTest, ParseUpdateModelScopeToGlobal) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL 'test_model' TO GLOBAL;", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->catalog, "flockmtl_storage.");
}

TEST(ModelParserTest, ParseUpdateModelScopeToGlobalWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL 'test_model' TO GLOBAL", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->catalog, "flockmtl_storage.");
}

TEST(ModelParserTest, ParseUpdateModelScopeToGlobalWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL 'test_model' TO GLOBAL; -- Make model global", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->catalog, "flockmtl_storage.");
}

TEST(ModelParserTest, ParseUpdateModelScopeToLocal) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL 'test_model' TO LOCAL;", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->catalog, "");
}

TEST(ModelParserTest, ParseUpdateModelScopeToLocalWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL 'test_model' TO LOCAL", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->catalog, "");
}

TEST(ModelParserTest, ParseUpdateModelScopeToLocalWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL 'test_model' TO LOCAL; -- Make model local", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelScopeStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->catalog, "");
}

TEST(ModelParserTest, ParseUpdateModelWithArgs) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider', {\"tuple_format\": \"xml\", \"batch_size\": 64, \"model_parameters\": {\"param2\": \"value2\"}})", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->new_model, "new_model_data");
    EXPECT_EQ(update_stmt->provider_name, "new_provider");
    EXPECT_EQ(update_stmt->new_model_args["tuple_format"], "xml");
    EXPECT_EQ(update_stmt->new_model_args["batch_size"], 64);
    EXPECT_EQ(update_stmt->new_model_args["model_parameters"].at("param2"), "value2");
}

TEST(ModelParserTest, ParseUpdateModelWithArgsWithSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider', {\"tuple_format\": \"xml\", \"batch_size\": 64, \"model_parameters\": {\"param2\": \"value2\"}});", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->new_model, "new_model_data");
    EXPECT_EQ(update_stmt->provider_name, "new_provider");
    EXPECT_EQ(update_stmt->new_model_args["tuple_format"], "xml");
    EXPECT_EQ(update_stmt->new_model_args["batch_size"], 64);
    EXPECT_EQ(update_stmt->new_model_args["model_parameters"].at("param2"), "value2");
}

TEST(ModelParserTest, ParseUpdateModelWithArgsWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider', {\"tuple_format\": \"xml\", \"batch_size\": 64, \"model_parameters\": {\"param2\": \"value2\"}}) -- Update with args", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->new_model, "new_model_data");
    EXPECT_EQ(update_stmt->provider_name, "new_provider");
    EXPECT_EQ(update_stmt->new_model_args["tuple_format"], "xml");
    EXPECT_EQ(update_stmt->new_model_args["batch_size"], 64);
    EXPECT_EQ(update_stmt->new_model_args["model_parameters"].at("param2"), "value2");
}

TEST(ModelParserTest, ParseInvalidUpdateModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data')", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseInvalidUpdateModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data') -- Invalid update", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseStringBatchSizeUpdateModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider', {\"tuple_format\": \"xml\", \"batch_size\": \"64\", \"model_parameters\": {\"param2\": \"value2\"}})", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseStringBatchSizeUpdateModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider', {\"tuple_format\": \"xml\", \"batch_size\": \"64\", \"model_parameters\": {\"param2\": \"value2\"}}) -- Invalid batch size", statement), std::runtime_error);
}

/**************************************************
 *                   Get Model                   *
 **************************************************/

TEST(ModelParserTest, ParseGetModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("GET MODEL 'test_model';", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetModelStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseGetModelWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("GET MODEL 'test_model'", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetModelStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseGetModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("GET MODEL 'test_model'; -- Get the test model", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetModelStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseGetModelWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Retrieve the test model\nGET MODEL 'test_model'", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetModelStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseGetModelWithCommentWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("GET MODEL 'test_model' -- Get the test model", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetModelStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseInvalidGetModel) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("GET MODEL", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseInvalidGetModelWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("GET MODEL -- Invalid get", statement), std::runtime_error);
}

/**************************************************
 *                Get All Models                 *
 **************************************************/

TEST(ModelParserTest, ParseGetAllModels) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("GET MODELS;", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllModelStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

TEST(ModelParserTest, ParseGetAllModelsWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("GET MODELS", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllModelStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

TEST(ModelParserTest, ParseGetAllModelsWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("GET MODELS; -- Get all models", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllModelStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

TEST(ModelParserTest, ParseGetAllModelsWithCommentBefore) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Retrieve all models\nGET MODELS", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllModelStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

TEST(ModelParserTest, ParseGetAllModelsWithCommentWithoutSemicolon) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("GET MODELS -- Get all models", statement));
    ASSERT_NE(statement, nullptr);
    auto get_all_stmt = dynamic_cast<GetAllModelStatement*>(statement.get());
    ASSERT_NE(get_all_stmt, nullptr);
}

/**************************************************
 *              Complex Comment Tests             *
 **************************************************/

TEST(ModelParserTest, ParseCreateModelWithMultipleComments) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Create a new model\n-- This is a test model with configuration\nCREATE MODEL ('test_model', 'model_data', 'provider', {\"tuple_format\": \"json\", \"batch_size\": 32, \"model_parameters\": {\"param1\": \"value1\"}}) -- End comment", statement));
    ASSERT_NE(statement, nullptr);
    auto create_stmt = dynamic_cast<CreateModelStatement*>(statement.get());
    ASSERT_NE(create_stmt, nullptr);
    EXPECT_EQ(create_stmt->model_name, "test_model");
    EXPECT_EQ(create_stmt->model, "model_data");
    EXPECT_EQ(create_stmt->provider_name, "provider");
    EXPECT_EQ(create_stmt->model_args["tuple_format"], "json");
    EXPECT_EQ(create_stmt->model_args["batch_size"], 32);
    EXPECT_EQ(create_stmt->model_args["model_parameters"].at("param1"), "value1");
}

TEST(ModelParserTest, ParseUpdateModelWithInlineComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("UPDATE MODEL ('test_model', 'new_model_data', 'new_provider', {\"tuple_format\": \"xml\", \"batch_size\": 64, \"model_parameters\": {\"param2\": \"value2\"}}) -- Update with new configuration", statement));
    ASSERT_NE(statement, nullptr);
    const auto update_stmt = dynamic_cast<UpdateModelStatement*>(statement.get());
    ASSERT_NE(update_stmt, nullptr);
    EXPECT_EQ(update_stmt->model_name, "test_model");
    EXPECT_EQ(update_stmt->new_model, "new_model_data");
    EXPECT_EQ(update_stmt->provider_name, "new_provider");
    EXPECT_EQ(update_stmt->new_model_args["tuple_format"], "xml");
    EXPECT_EQ(update_stmt->new_model_args["batch_size"], 64);
    EXPECT_EQ(update_stmt->new_model_args["model_parameters"].at("param2"), "value2");
}

TEST(ModelParserTest, ParseDeleteModelWithDetailedComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Remove the test model from storage\nDELETE MODEL 'test_model'; -- Cleanup operation", statement));
    ASSERT_NE(statement, nullptr);
    const auto delete_stmt = dynamic_cast<DeleteModelStatement*>(statement.get());
    ASSERT_NE(delete_stmt, nullptr);
    EXPECT_EQ(delete_stmt->model_name, "test_model");
}

TEST(ModelParserTest, ParseGetModelWithDocumentationComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_NO_THROW(parser.Parse("-- Retrieve model for analysis\n-- This model is used in the main application\nGET MODEL 'test_model' -- Get for processing", statement));
    ASSERT_NE(statement, nullptr);
    auto get_stmt = dynamic_cast<GetModelStatement*>(statement.get());
    ASSERT_NE(get_stmt, nullptr);
    EXPECT_EQ(get_stmt->model_name, "test_model");
}

/**************************************************
 *              Error Handling Tests              *
 **************************************************/

TEST(ModelParserTest, ParseEmptyQuery) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseOnlyComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("-- This is only a comment", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseOnlyWhitespace) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("   \n\t  ", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseInvalidKeyword) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("INVALID COMMAND", statement), std::runtime_error);
}

TEST(ModelParserTest, ParseInvalidKeywordWithComment) {
    std::unique_ptr<QueryStatement> statement;
    ModelParser parser;
    EXPECT_THROW(parser.Parse("INVALID COMMAND -- This should fail", statement), std::runtime_error);
}