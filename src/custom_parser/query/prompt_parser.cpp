#include "flock/custom_parser/query/prompt_parser.hpp"

#include "flock/core/common.hpp"
#include "flock/core/config.hpp"
#include "flock/custom_parser/query_parser.hpp"

#include <sstream>
#include <stdexcept>

namespace flock {

void PromptParser::Parse(const std::string& query, std::unique_ptr<QueryStatement>& statement) {
    Tokenizer tokenizer(query);
    auto token = tokenizer.NextToken();
    auto value = duckdb::StringUtil::Upper(token.value);

    if (token.type == TokenType::KEYWORD) {
        if (value == "CREATE") {
            ParseCreatePrompt(tokenizer, statement);
        } else if (value == "DELETE") {
            ParseDeletePrompt(tokenizer, statement);
        } else if (value == "UPDATE") {
            ParseUpdatePrompt(tokenizer, statement);
        } else if (value == "GET") {
            ParseGetPrompt(tokenizer, statement);
        } else {
            throw std::runtime_error("Unknown keyword: " + token.value);
        }
    } else {
        throw std::runtime_error("Unknown keyword: " + token.value);
    }
}

void PromptParser::ParseCreatePrompt(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement) {
    auto token = tokenizer.NextToken();
    auto value = duckdb::StringUtil::Upper(token.value);

    std::string catalog;
    if (token.type == TokenType::KEYWORD && (value == "GLOBAL" || value == "LOCAL")) {
        if (value == "GLOBAL") {
            catalog = "flock_storage.";
        }
        token = tokenizer.NextToken();
        value = duckdb::StringUtil::Upper(token.value);
    }

    if (token.type != TokenType::KEYWORD || value != "PROMPT") {
        throw std::runtime_error("Unknown keyword: " + token.value);
    }

    token = tokenizer.NextToken();
    if (token.type != TokenType::PARENTHESIS || token.value != "(") {
        throw std::runtime_error("Expected opening parenthesis '(' after 'PROMPT'.");
    }

    token = tokenizer.NextToken();
    if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
        throw std::runtime_error("Expected non-empty string literal for prompt name.");
    }
    const auto prompt_name = token.value;

    token = tokenizer.NextToken();
    if (token.type != TokenType::SYMBOL || token.value != ",") {
        throw std::runtime_error("Expected comma ',' after prompt name.");
    }

    token = tokenizer.NextToken();
    if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
        throw std::runtime_error("Expected non-empty string literal for prompt text.");
    }
    const auto prompt = token.value;

    token = tokenizer.NextToken();
    if (token.type != TokenType::PARENTHESIS || token.value != ")") {
        throw std::runtime_error("Expected closing parenthesis ')' after prompt text.");
    }

    token = tokenizer.NextToken();
    if (token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") {
        auto create_statement = std::make_unique<CreatePromptStatement>();
        create_statement->catalog = catalog;
        create_statement->prompt_name = prompt_name;
        create_statement->prompt = prompt;
        statement = std::move(create_statement);
    } else {
        throw std::runtime_error("Unexpected characters after the closing parenthesis. Only a semicolon is allowed.");
    }
}

void PromptParser::ParseDeletePrompt(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement) {
    auto token = tokenizer.NextToken();
    auto value = duckdb::StringUtil::Upper(token.value);
    if (token.type != TokenType::KEYWORD || value != "PROMPT") {
        throw std::runtime_error("Unknown keyword: " + token.value);
    }

    token = tokenizer.NextToken();
    if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
        throw std::runtime_error("Expected non-empty string literal for prompt name.");
    }
    auto prompt_name = token.value;

    token = tokenizer.NextToken();
    if (token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") {
        auto delete_statement = std::make_unique<DeletePromptStatement>();
        delete_statement->prompt_name = prompt_name;
        statement = std::move(delete_statement);
    } else {
        throw std::runtime_error("Unexpected characters after the closing parenthesis. Only a semicolon is allowed.");
    }
}

void PromptParser::ParseUpdatePrompt(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement) {
    auto token = tokenizer.NextToken();
    auto value = duckdb::StringUtil::Upper(token.value);
    if (token.type != TokenType::KEYWORD || value != "PROMPT") {
        throw std::runtime_error("Unknown keyword: " + token.value);
    }

    token = tokenizer.NextToken();
    if (token.type == TokenType::STRING_LITERAL) {
        auto prompt_name = token.value;
        token = tokenizer.NextToken();
        if (token.type != TokenType::KEYWORD || duckdb::StringUtil::Upper(token.value) != "TO") {
            throw std::runtime_error("Expected 'TO' after prompt name.");
        }

        token = tokenizer.NextToken();
        value = duckdb::StringUtil::Upper(token.value);
        if (token.type != TokenType::KEYWORD || (value != "GLOBAL" && value != "LOCAL")) {
            throw std::runtime_error("Expected 'GLOBAL' or 'LOCAL' after 'TO'.");
        }
        auto catalog = value == "GLOBAL" ? "flock_storage." : "";

        token = tokenizer.NextToken();
        if (token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") {
            auto update_statement = std::make_unique<UpdatePromptScopeStatement>();
            update_statement->prompt_name = prompt_name;
            update_statement->catalog = catalog;
            statement = std::move(update_statement);
        } else {
            throw std::runtime_error(
                    "Unexpected characters after the closing parenthesis. Only a semicolon is allowed.");
        }

    } else {
        if (token.type != TokenType::PARENTHESIS || token.value != "(") {
            throw std::runtime_error("Expected opening parenthesis '(' after 'PROMPT'.");
        }

        token = tokenizer.NextToken();
        if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
            throw std::runtime_error("Expected non-empty string literal for prompt name.");
        }
        auto prompt_name = token.value;

        token = tokenizer.NextToken();
        if (token.type != TokenType::SYMBOL || token.value != ",") {
            throw std::runtime_error("Expected comma ',' after prompt name.");
        }

        token = tokenizer.NextToken();
        if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
            throw std::runtime_error("Expected non-empty string literal for new prompt text.");
        }
        auto new_prompt = token.value;

        token = tokenizer.NextToken();
        if (token.type != TokenType::PARENTHESIS || token.value != ")") {
            throw std::runtime_error("Expected closing parenthesis ')' after new prompt text.");
        }

        token = tokenizer.NextToken();
        if (token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") {
            auto update_statement = std::make_unique<UpdatePromptStatement>();
            update_statement->prompt_name = prompt_name;
            update_statement->new_prompt = new_prompt;
            statement = std::move(update_statement);
        } else {
            throw std::runtime_error(
                    "Unexpected characters after the closing parenthesis. Only a semicolon is allowed.");
        }
    }
}

void PromptParser::ParseGetPrompt(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement) {
    auto token = tokenizer.NextToken();
    auto value = duckdb::StringUtil::Upper(token.value);
    if (token.type != TokenType::KEYWORD || (value != "PROMPT" && value != "PROMPTS")) {
        throw std::runtime_error("Unknown keyword: " + token.value);
    }

    token = tokenizer.NextToken();
    if ((token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") && value == "PROMPTS") {
        auto get_all_statement = duckdb::make_uniq<GetAllPromptStatement>();
        statement = std::move(get_all_statement);
    } else {
        if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
            throw std::runtime_error("Expected non-empty string literal for prompt name.");
        }
        auto prompt_name = token.value;

        token = tokenizer.NextToken();
        if (token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") {
            auto get_statement = duckdb::make_uniq<GetPromptStatement>();
            get_statement->prompt_name = prompt_name;
            statement = std::move(get_statement);
        } else {
            throw std::runtime_error("Unexpected characters after the prompt name. Only a semicolon is allowed.");
        }
    }
}

std::string PromptParser::ToSQL(const QueryStatement& statement) const {
    std::string query;

    switch (statement.type) {
        case StatementType::CREATE_PROMPT: {
            const auto& create_stmt = static_cast<const CreatePromptStatement&>(statement);
            query = ExecuteQueryWithStorage([&create_stmt](duckdb::Connection& con) {
                auto result = con.Query(duckdb_fmt::format(" SELECT prompt_name "
                                                           "   FROM flock_storage.flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE"
                                                           "  WHERE prompt_name = '{}'"
                                                           " UNION ALL "
                                                           " SELECT prompt_name "
                                                           "   FROM flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE"
                                                           "  WHERE prompt_name = '{}';",
                                                           create_stmt.prompt_name, create_stmt.prompt_name));

                auto& materialized_result = result->Cast<duckdb::MaterializedQueryResult>();
                if (materialized_result.RowCount() != 0) {
                    throw std::runtime_error(duckdb_fmt::format("Prompt '{}' already exist.", create_stmt.prompt_name));
                }

                auto insert_query = duckdb_fmt::format(" INSERT INTO {}flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                                       " (prompt_name, prompt) "
                                                       " VALUES ('{}', '{}'); ",
                                                       create_stmt.catalog, create_stmt.prompt_name, create_stmt.prompt);
                con.Query(insert_query);

                return std::string("SELECT 'Prompt created successfully' AS status");
            },
                                            false);
            break;
        }
        case StatementType::DELETE_PROMPT: {
            const auto& delete_stmt = static_cast<const DeletePromptStatement&>(statement);
            query = ExecuteSetQuery(
                    duckdb_fmt::format(" DELETE FROM flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       "  WHERE prompt_name = '{}'; "
                                       " DELETE FROM flock_storage.flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       "  WHERE prompt_name = '{}'; ",
                                       delete_stmt.prompt_name, delete_stmt.prompt_name),
                    "Prompt deleted successfully",
                    false);
            break;
        }
        case StatementType::UPDATE_PROMPT: {
            const auto& update_stmt = static_cast<const UpdatePromptStatement&>(statement);
            query = ExecuteQueryWithStorage([&update_stmt](duckdb::Connection& con) {
                auto result = con.Query(duckdb_fmt::format(" SELECT version, 'local' AS scope "
                                                           "  FROM flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE"
                                                           " WHERE prompt_name = '{}'"
                                                           " UNION ALL "
                                                           " SELECT version, 'global' AS scope "
                                                           "   FROM flock_storage.flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE"
                                                           "  WHERE prompt_name = '{}' "
                                                           " ORDER BY version DESC;",
                                                           update_stmt.prompt_name, update_stmt.prompt_name));

                auto& materialized_result = result->Cast<duckdb::MaterializedQueryResult>();
                if (materialized_result.RowCount() == 0) {
                    throw std::runtime_error(duckdb_fmt::format("Prompt '{}' doesn't exist.", update_stmt.prompt_name));
                }

                int version = materialized_result.GetValue<int>(0, 0) + 1;
                auto catalog = materialized_result.GetValue(1, 0).ToString() == "global" ? "flock_storage." : "";

                con.Query(duckdb_fmt::format(" INSERT INTO {}flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                             " (prompt_name, prompt, version) "
                                             " VALUES ('{}', '{}', {}); ",
                                             catalog, update_stmt.prompt_name, update_stmt.new_prompt, version));

                return std::string("SELECT 'Prompt updated successfully' AS status");
            },
                                            false);
            break;
        }
        case StatementType::UPDATE_PROMPT_SCOPE: {
            const auto& update_stmt = static_cast<const UpdatePromptScopeStatement&>(statement);
            query = ExecuteQueryWithStorage([&update_stmt](duckdb::Connection& con) {
                auto result = con.Query(duckdb_fmt::format(" SELECT prompt_name "
                                                           "   FROM {}flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE"
                                                           "  WHERE prompt_name = '{}';",
                                                           update_stmt.catalog, update_stmt.prompt_name));

                auto& materialized_result = result->Cast<duckdb::MaterializedQueryResult>();
                if (materialized_result.RowCount() != 0) {
                    throw std::runtime_error(
                            duckdb_fmt::format("Prompt '{}' already exist in {} storage.", update_stmt.prompt_name,
                                               update_stmt.catalog == "flock_storage." ? "global" : "local"));
                }

                con.Query(duckdb_fmt::format("INSERT INTO {}flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                             "(prompt_name, prompt, updated_at, version) "
                                             "SELECT prompt_name, prompt, updated_at, version "
                                             "FROM {}flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                             "WHERE prompt_name = '{}';",
                                             update_stmt.catalog,
                                             update_stmt.catalog == "flock_storage." ? "" : "flock_storage.",
                                             update_stmt.prompt_name));

                con.Query(duckdb_fmt::format("DELETE FROM {}flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                             "WHERE prompt_name = '{}'; ",
                                             update_stmt.catalog == "flock_storage." ? "" : "flock_storage.",
                                             update_stmt.prompt_name));

                return std::string("SELECT 'Prompt scope updated successfully' AS status");
            },
                                            false);
            break;
        }
        case StatementType::GET_PROMPT: {
            const auto& get_stmt = static_cast<const GetPromptStatement&>(statement);
            query = ExecuteGetQuery(
                    duckdb_fmt::format("SELECT 'global' AS scope, * "
                                       "FROM flock_storage.flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       "WHERE prompt_name = '{}' "
                                       "UNION ALL "
                                       "SELECT 'local' AS scope, * "
                                       "FROM flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       "WHERE prompt_name = '{}' "
                                       "ORDER BY version DESC;",
                                       get_stmt.prompt_name, get_stmt.prompt_name),
                    true);
            break;
        }
        case StatementType::GET_ALL_PROMPT: {
            query = ExecuteGetQuery(
                    " SELECT 'global' as scope, t1.* "
                    "   FROM flock_storage.flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE AS t1 "
                    "   JOIN (SELECT prompt_name, MAX(version) AS max_version "
                    "   FROM flock_storage.flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                    "  GROUP BY prompt_name) AS t2 "
                    "     ON t1.prompt_name = t2.prompt_name "
                    "    AND t1.version = t2.max_version"
                    " UNION ALL "
                    " SELECT 'local' as scope, t1.* "
                    "   FROM flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE AS t1 "
                    "   JOIN (SELECT prompt_name, MAX(version) AS max_version "
                    "   FROM flock_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                    "  GROUP BY prompt_name) AS t2 "
                    "     ON t1.prompt_name = t2.prompt_name "
                    "    AND t1.version = t2.max_version; ",
                    true);
            break;
        }
        default:
            throw std::runtime_error("Unknown statement type.");
    }

    return query;
}

}// namespace flock
