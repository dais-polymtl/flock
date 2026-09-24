#include "flock/core/config.hpp"
#include "flock/custom_parser/query/model_parser.hpp"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace flock {
// Test CreateModelStatement
TEST(ModelStatementTest, CreateModelStatement_DefaultInitialization) {
    CreateModelStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::CREATE_MODEL);
    EXPECT_TRUE(stmt.catalog.empty());
    EXPECT_TRUE(stmt.model_name.empty());
    EXPECT_TRUE(stmt.model.empty());
    EXPECT_TRUE(stmt.provider_name.empty());
    EXPECT_TRUE(stmt.model_args.is_null() || stmt.model_args.empty());
}

// Test DeleteModelStatement
TEST(ModelStatementTest, DeleteModelStatement_DefaultInitialization) {
    DeleteModelStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::DELETE_MODEL);
    EXPECT_TRUE(stmt.model_name.empty());
    EXPECT_TRUE(stmt.provider_name.empty());
}

// Test UpdateModelScopeStatement
TEST(ModelStatementTest, UpdateModelScopeStatement_DefaultInitialization) {
    UpdateModelScopeStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::UPDATE_MODEL_SCOPE);
    EXPECT_TRUE(stmt.model_name.empty());
    EXPECT_TRUE(stmt.catalog.empty());
}

// Test UpdateModelStatement
TEST(ModelStatementTest, UpdateModelStatement_DefaultInitialization) {
    UpdateModelStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::UPDATE_MODEL);
    EXPECT_TRUE(stmt.model_name.empty());
    EXPECT_TRUE(stmt.new_model.empty());
    EXPECT_TRUE(stmt.provider_name.empty());
    EXPECT_TRUE(stmt.new_model_args.is_null() || stmt.new_model_args.empty());
}

// Test GetModelStatement
TEST(ModelStatementTest, GetModelStatement_DefaultInitialization) {
    GetModelStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::GET_MODEL);
    EXPECT_TRUE(stmt.model_name.empty());
    EXPECT_TRUE(stmt.provider_name.empty());
}

// Test GetAllModelStatement
TEST(ModelStatementTest, GetAllModelStatement_DefaultInitialization) {
    GetAllModelStatement stmt;
    EXPECT_EQ(stmt.type, StatementType::GET_ALL_MODEL);
}

TEST(ModelStatementTest, TypeSafeRejectsInapplicableModelArgs) {
    auto con = Config::GetConnection();

    const auto parameters = con.Query(
            "CREATE MODEL('jev-rejects-parameters', 'jev-latest', 'typesafe', "
            "{\"model_parameters\": {\"temperature\": 0}});");
    ASSERT_TRUE(parameters->HasError());
    EXPECT_NE(parameters->GetError().find("model_parameters"), std::string::npos);

    const auto format = con.Query("CREATE MODEL('jev-rejects-format', 'jev-latest', 'typesafe', "
                                  "{\"tuple_format\": \"JSON\"});");
    ASSERT_TRUE(format->HasError());
    EXPECT_NE(format->GetError().find("tuple_format"), std::string::npos);

    // The models table outlives a test run.
    con.Query("DELETE MODEL 'jev-good-args';");
    const auto accepted = con.Query("CREATE MODEL('jev-good-args', 'jev-latest', 'typesafe', "
                                    "{\"threshold\": 0.8, \"max_batch_size\": 64});");
    EXPECT_FALSE(accepted->HasError()) << accepted->GetError();

    // The same settings remain valid for a generative provider.
    con.Query("DELETE MODEL 'gpt-good-args';");
    const auto generative = con.Query("CREATE MODEL('gpt-good-args', 'gpt-4o', 'openai', "
                                      "{\"model_parameters\": {\"temperature\": 0}});");
    EXPECT_FALSE(generative->HasError()) << generative->GetError();
}

}// namespace flock
