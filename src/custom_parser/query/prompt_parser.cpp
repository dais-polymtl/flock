#include "flockmtl/custom_parser/query/prompt_parser.hpp"

#include "flockmtl/core/common.hpp"
#include "flockmtl/core/config.hpp"

#include <sstream>
#include <stdexcept>

namespace flockmtl {

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
            catalog = "flockmtl_storage.";
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
        auto catalog = value == "GLOBAL" ? "flockmtl_storage." : "";

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
            auto con = Config::GetConnection();
            auto result = con.Query(duckdb_fmt::format(" SELECT prompt_name "
                                                       "   FROM {}flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE"
                                                       "  WHERE prompt_name = '{}';",
                                                       create_stmt.catalog.empty() ? "flockmtl_storage." : "",
                                                       create_stmt.prompt_name));
            if (result->RowCount() != 0) {
                throw std::runtime_error(duckdb_fmt::format("Prompt '{}' already exist.", create_stmt.prompt_name));
            }
            query = duckdb_fmt::format(" INSERT INTO {}flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       " (prompt_name, prompt) "
                                       " VALUES ('{}', '{}'); ",
                                       create_stmt.catalog, create_stmt.prompt_name, create_stmt.prompt);
            break;
        }
        case StatementType::DELETE_PROMPT: {
            const auto& delete_stmt = static_cast<const DeletePromptStatement&>(statement);
            auto con = Config::GetConnection();
            auto result = con.Query(duckdb_fmt::format("  DELETE FROM flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                                       "  WHERE prompt_name = '{}'; ",
                                                       delete_stmt.prompt_name));

            query = duckdb_fmt::format(" DELETE FROM flockmtl_storage.flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       "  WHERE prompt_name = '{}'; ",
                                       delete_stmt.prompt_name);
            break;
        }
        case StatementType::UPDATE_PROMPT: {
            const auto& update_stmt = static_cast<const UpdatePromptStatement&>(statement);
            auto con = Config::GetConnection();
            auto result =
                    con.Query(duckdb_fmt::format(" SELECT version, 'local' AS scope "
                                                 "  FROM flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE"
                                                 " WHERE prompt_name = '{}'"
                                                 " UNION ALL "
                                                 " SELECT version, 'global' AS scope "
                                                 "   FROM flockmtl_storage.flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE"
                                                 "  WHERE prompt_name = '{}' "
                                                 " ORDER BY version DESC;",
                                                 update_stmt.prompt_name, update_stmt.prompt_name));
            if (result->RowCount() == 0) {
                throw std::runtime_error(duckdb_fmt::format("Prompt '{}' doesn't exist.", update_stmt.prompt_name));
            }

            int version = result->GetValue<int>(0, 0) + 1;
            auto catalog = result->GetValue(1, 0).ToString() == "global" ? "flockmtl_storage." : "";
            query = duckdb_fmt::format(" INSERT INTO {}flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       " (prompt_name, prompt, version) "
                                       " VALUES ('{}', '{}', {}); ",
                                       catalog, update_stmt.prompt_name, update_stmt.new_prompt, version);
            break;
        }
        case StatementType::UPDATE_PROMPT_SCOPE: {
            const auto& update_stmt = static_cast<const UpdatePromptScopeStatement&>(statement);
            auto con = Config::GetConnection();
            auto result = con.Query(duckdb_fmt::format(" SELECT prompt_name "
                                                       "   FROM {}flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE"
                                                       "  WHERE prompt_name = '{}';",
                                                       update_stmt.catalog, update_stmt.prompt_name));
            if (result->RowCount() != 0) {
                throw std::runtime_error(
                        duckdb_fmt::format("Model '{}' already exist in {} storage.", update_stmt.prompt_name,
                                           update_stmt.catalog == "flockmtl_storage." ? "global" : "local"));
            }

            con.Query(duckdb_fmt::format("INSERT INTO {}flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                         "(prompt_name, prompt, updated_at, version) "
                                         "SELECT prompt_name, prompt, updated_at, version "
                                         "FROM {}flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                         "WHERE prompt_name = '{}';",
                                         update_stmt.catalog,
                                         update_stmt.catalog == "flockmtl_storage." ? "" : "flockmtl_storage.",
                                         update_stmt.prompt_name));

            query = duckdb_fmt::format("DELETE FROM {}flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       "WHERE prompt_name = '{}'; ",
                                       update_stmt.catalog == "flockmtl_storage." ? "" : "flockmtl_storage.",
                                       update_stmt.prompt_name);
            break;
        }
        case StatementType::GET_PROMPT: {
            const auto& get_stmt = static_cast<const GetPromptStatement&>(statement);
            query = duckdb_fmt::format("SELECT 'global' AS scope, * "
                                       "FROM flockmtl_storage.flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       "WHERE prompt_name = '{}' "
                                       "UNION ALL "
                                       "SELECT 'local' AS scope, * "
                                       "FROM flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                                       "WHERE prompt_name = '{}' "
                                       "ORDER BY version DESC;",
                                       get_stmt.prompt_name, get_stmt.prompt_name);

            break;
        }
        case StatementType::GET_ALL_PROMPT: {
            query = " SELECT 'global' as scope, t1.* "
                    "   FROM flockmtl_storage.flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE AS t1 "
                    "   JOIN (SELECT prompt_name, MAX(version) AS max_version "
                    "   FROM flockmtl_storage.flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                    "  GROUP BY prompt_name) AS t2 "
                    "     ON t1.prompt_name = t2.prompt_name "
                    "    AND t1.version = t2.max_version"
                    " UNION ALL "
                    " SELECT 'local' as scope, t1.* "
                    "   FROM flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE AS t1 "
                    "   JOIN (SELECT prompt_name, MAX(version) AS max_version "
                    "   FROM flockmtl_config.FLOCKMTL_PROMPT_INTERNAL_TABLE "
                    "  GROUP BY prompt_name) AS t2 "
                    "     ON t1.prompt_name = t2.prompt_name "
                    "    AND t1.version = t2.max_version; ";
            break;
        }
        default:
            throw std::runtime_error("Unknown statement type.");
    }

    return query;
}

}// namespace flockmtl
