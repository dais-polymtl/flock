#include "flock/custom_parser/query/model_parser.hpp"

#include "flock/core/common.hpp"
#include "flock/core/config.hpp"
#include "flock/custom_parser/query_parser.hpp"
#include <sstream>
#include <stdexcept>

namespace flock {

void ModelParser::Parse(const std::string& query, std::unique_ptr<QueryStatement>& statement) {
    Tokenizer tokenizer(query);
    auto token = tokenizer.NextToken();
    const auto value = duckdb::StringUtil::Upper(token.value);

    if (token.type == TokenType::KEYWORD) {
        if (value == "CREATE") {
            ParseCreateModel(tokenizer, statement);
        } else if (value == "DELETE") {
            ParseDeleteModel(tokenizer, statement);
        } else if (value == "UPDATE") {
            ParseUpdateModel(tokenizer, statement);
        } else if (value == "GET") {
            ParseGetModel(tokenizer, statement);
        } else {
            throw std::runtime_error("Unknown keyword: " + token.value);
        }
    } else {
        throw std::runtime_error("Expected a keyword at the beginning of the query.");
    }
}

void ModelParser::ParseCreateModel(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement) {
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

    if (token.type != TokenType::KEYWORD || value != "MODEL") {
        throw std::runtime_error("Expected 'MODEL' after 'CREATE'.");
    }

    token = tokenizer.NextToken();
    if (token.type != TokenType::PARENTHESIS || token.value != "(") {
        throw std::runtime_error("Expected opening parenthesis '(' after 'MODEL'.");
    }

    token = tokenizer.NextToken();
    if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
        throw std::runtime_error("Expected non-empty string literal for model name.");
    }
    auto model_name = token.value;

    token = tokenizer.NextToken();
    if (token.type != TokenType::SYMBOL || token.value != ",") {
        throw std::runtime_error("Expected comma ',' after model name.");
    }

    token = tokenizer.NextToken();
    if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
        throw std::runtime_error("Expected non-empty string literal for model.");
    }
    auto model = token.value;

    token = tokenizer.NextToken();
    if (token.type != TokenType::SYMBOL || token.value != ",") {
        throw std::runtime_error("Expected comma ',' after model.");
    }

    token = tokenizer.NextToken();
    if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
        throw std::runtime_error("Expected non-empty string literal for provider_name.");
    }
    std::string provider_name = token.value;

    token = tokenizer.NextToken();
    nlohmann::json model_args = nlohmann::json::object();
    // The JSON argument is optional. If present, extract tuple_format, batch_size, and model_parameters (all optional).
    if (token.type == TokenType::SYMBOL || token.value == ",") {
        token = tokenizer.NextToken();
        try {
            nlohmann::json input_args = nlohmann::json::parse(token.value);
            // Only allow tuple_format, batch_size, model_parameters
            for (auto it = input_args.begin(); it != input_args.end(); ++it) {
                const std::string& key = it.key();
                if (key == "tuple_format" || key == "batch_size" || key == "model_parameters") {
                    const auto& param_val = it.value();
                    if (key == "batch_size") {
                        if (!param_val.is_number_integer()) {
                            throw std::runtime_error("Expected 'batch_size' to be an integer.");
                        }
                        model_args[key] = param_val.get<int>();
                    } else {
                        model_args[key] = it.value();
                    }
                } else {
                    throw std::runtime_error("Unknown model_args parameter: '" + key + "'. Only tuple_format, batch_size, and model_parameters are allowed.");
                }
            }
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Failed to parse model_args JSON: ") + e.what());
        }
        token = tokenizer.NextToken();
        if (token.type != TokenType::PARENTHESIS) {
            throw std::runtime_error("Expected closing parenthesis ')' after model_args.");
        }
    } else if (token.type == TokenType::PARENTHESIS && token.value == ")") {
    } else {
        throw std::runtime_error("Expected closing parenthesis ')' or JSON after provider_name.");
    }

    token = tokenizer.NextToken();
    if (token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") {
        auto create_statement = std::make_unique<CreateModelStatement>();
        create_statement->catalog = catalog;
        create_statement->model_name = model_name;
        create_statement->model = model;
        create_statement->provider_name = provider_name;
        create_statement->model_args = model_args;
        statement = std::move(create_statement);
    } else {
        throw std::runtime_error("Unexpected characters after the closing parenthesis. Only a semicolon is allowed.");
    }
}

void ModelParser::ParseDeleteModel(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement) {
    auto token = tokenizer.NextToken();
    auto value = duckdb::StringUtil::Upper(token.value);
    if (token.type != TokenType::KEYWORD || value != "MODEL") {
        throw std::runtime_error("Unknown keyword: " + token.value);
    }

    token = tokenizer.NextToken();
    if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
        throw std::runtime_error("Expected non-empty string literal for model name.");
    }
    auto model_name = token.value;

    token = tokenizer.NextToken();
    if (token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") {
        auto delete_statement = std::make_unique<DeleteModelStatement>();
        delete_statement->model_name = model_name;
        statement = std::move(delete_statement);
    } else {
        throw std::runtime_error("Unexpected characters after the closing parenthesis. Only a semicolon is allowed.");
    }
}

void ModelParser::ParseUpdateModel(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement) {
    auto token = tokenizer.NextToken();
    auto value = duckdb::StringUtil::Upper(token.value);
    if (token.type != TokenType::KEYWORD || value != "MODEL") {
        throw std::runtime_error("Expected 'MODEL' after 'UPDATE'.");
    }

    token = tokenizer.NextToken();
    if (token.type == TokenType::STRING_LITERAL) {
        auto model_name = token.value;
        token = tokenizer.NextToken();
        if (token.type != TokenType::KEYWORD || duckdb::StringUtil::Upper(token.value) != "TO") {
            throw std::runtime_error("Expected 'TO' after model name.");
        }

        token = tokenizer.NextToken();
        value = duckdb::StringUtil::Upper(token.value);
        if (token.type != TokenType::KEYWORD || (value != "GLOBAL" && value != "LOCAL")) {
            throw std::runtime_error("Expected 'GLOBAL' or 'LOCAL' after 'TO'.");
        }
        auto catalog = value == "GLOBAL" ? "flock_storage." : "";

        token = tokenizer.NextToken();
        if (token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") {
            auto update_statement = std::make_unique<UpdateModelScopeStatement>();
            update_statement->model_name = model_name;
            update_statement->catalog = catalog;
            statement = std::move(update_statement);
        } else {
            throw std::runtime_error(
                    "Unexpected characters after the closing parenthesis. Only a semicolon is allowed.");
        }

    } else {
        if (token.type != TokenType::PARENTHESIS || token.value != "(") {
            throw std::runtime_error("Expected opening parenthesis '(' after 'MODEL'.");
        }

        token = tokenizer.NextToken();
        if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
            throw std::runtime_error("Expected non-empty string literal for model name.");
        }
        auto model_name = token.value;

        token = tokenizer.NextToken();
        if (token.type != TokenType::SYMBOL || token.value != ",") {
            throw std::runtime_error("Expected comma ',' after model name.");
        }

        token = tokenizer.NextToken();
        if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
            throw std::runtime_error("Expected non-empty string literal for model.");
        }
        auto new_model = token.value;

        token = tokenizer.NextToken();
        if (token.type != TokenType::SYMBOL || token.value != ",") {
            throw std::runtime_error("Expected comma ',' after model.");
        }

        token = tokenizer.NextToken();
        if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
            throw std::runtime_error("Expected non-empty string literal for provider_name.");
        }
        auto provider_name = token.value;

        token = tokenizer.NextToken();
        nlohmann::json new_model_args = nlohmann::json::object();
        if (token.type == TokenType::SYMBOL || token.value == ",") {
            token = tokenizer.NextToken();
            try {
                nlohmann::json input_args = nlohmann::json::parse(token.value);
                // Only allow tuple_format, batch_size, model_parameters
                for (auto it = input_args.begin(); it != input_args.end(); ++it) {
                    const std::string& key = it.key();
                    if (key == "tuple_format" || key == "batch_size" || key == "model_parameters") {
                        const auto& param_val = it.value();
                        if (key == "batch_size") {
                            if (!param_val.is_number_integer()) {
                                throw std::runtime_error("Expected 'batch_size' to be an integer.");
                            }
                            new_model_args[key] = param_val.get<int>();
                        } else {
                            new_model_args[key] = it.value();
                        }
                    } else {
                        throw std::runtime_error("Unknown model_args parameter: '" + key + "'. Only tuple_format, batch_size, and model_parameters are allowed.");
                    }
                }
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("Failed to parse model_args JSON: ") + e.what());
            }
            token = tokenizer.NextToken();
            if (token.type != TokenType::PARENTHESIS) {
                throw std::runtime_error("Expected closing parenthesis ')' after model_args.");
            }
        } else if (token.type == TokenType::PARENTHESIS) {
            // No model_args provided, just closing parenthesis
        } else {
            throw std::runtime_error("Expected closing parenthesis ')' or JSON after provider_name.");
        }

        token = tokenizer.NextToken();
        if (token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") {
            auto update_statement = std::make_unique<UpdateModelStatement>();
            update_statement->new_model = new_model;
            update_statement->model_name = model_name;
            update_statement->provider_name = provider_name;
            update_statement->new_model_args = new_model_args;
            statement = std::move(update_statement);
        } else {
            throw std::runtime_error(
                    "Unexpected characters after the closing parenthesis. Only a semicolon is allowed.");
        }
    }
}

void ModelParser::ParseGetModel(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement) {
    auto token = tokenizer.NextToken();
    auto value = duckdb::StringUtil::Upper(token.value);
    if (token.type != TokenType::KEYWORD || (value != "MODEL" && value != "MODELS")) {
        throw std::runtime_error("Expected 'MODEL' after 'GET'.");
    }

    token = tokenizer.NextToken();
    if ((token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") && value == "MODELS") {
        auto get_all_statement = std::make_unique<GetAllModelStatement>();
        statement = std::move(get_all_statement);
    } else {
        if (token.type != TokenType::STRING_LITERAL || token.value.empty()) {
            throw std::runtime_error("Expected non-empty string literal for model name.");
        }
        auto model_name = token.value;

        token = tokenizer.NextToken();
        if (token.type == TokenType::END_OF_FILE || token.type == TokenType::SYMBOL || token.value == ";") {
            auto get_statement = std::make_unique<GetModelStatement>();
            get_statement->model_name = model_name;
            statement = std::move(get_statement);
        } else {
            throw std::runtime_error("Unexpected characters after the model name. Only a semicolon is allowed.");
        }
    }
}

std::string ModelParser::ToSQL(const QueryStatement& statement) const {
    std::string query;

    switch (statement.type) {
        case StatementType::CREATE_MODEL: {
            const auto& create_stmt = static_cast<const CreateModelStatement&>(statement);
            query = ExecuteQueryWithStorage([&create_stmt](duckdb::Connection& con) {
                auto result = con.Query(duckdb_fmt::format(
                        " SELECT model_name"
                        "  FROM flock_storage.flock_config.FLOCKMTL_MODEL_DEFAULT_INTERNAL_TABLE"
                        " WHERE model_name = '{}'"
                        " UNION ALL "
                        " SELECT model_name "
                        "   FROM flock_storage.flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE"
                        "  WHERE model_name = '{}'"
                        " UNION ALL "
                        " SELECT model_name "
                        "   FROM flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE"
                        "  WHERE model_name = '{}';",
                        create_stmt.model_name, create_stmt.model_name, create_stmt.model_name));

                auto& materialized_result = result->Cast<duckdb::MaterializedQueryResult>();
                if (materialized_result.RowCount() != 0) {
                    throw std::runtime_error(duckdb_fmt::format("Model '{}' already exist.", create_stmt.model_name));
                }

                // Insert the new model
                auto insert_query = duckdb_fmt::format(" INSERT INTO "
                                                       " {}flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
                                                       " (model_name, model, provider_name, model_args) "
                                                       " VALUES ('{}', '{}', '{}', '{}');",
                                                       create_stmt.catalog, create_stmt.model_name, create_stmt.model,
                                                       create_stmt.provider_name, create_stmt.model_args.dump());
                con.Query(insert_query);

                return std::string("SELECT 'Model created successfully' AS status");
            },
                                            false);
            break;
        }
        case StatementType::DELETE_MODEL: {
            const auto& delete_stmt = static_cast<const DeleteModelStatement&>(statement);
            query = ExecuteSetQuery(
                    duckdb_fmt::format(" DELETE FROM flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
                                       "  WHERE model_name = '{}'; "
                                       " DELETE FROM "
                                       " flock_storage.flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
                                       "  WHERE model_name = '{}';",
                                       delete_stmt.model_name, delete_stmt.model_name),
                    "Model deleted successfully",
                    false);
            break;
        }
        case StatementType::UPDATE_MODEL: {
            const auto& update_stmt = static_cast<const UpdateModelStatement&>(statement);
            query = ExecuteQueryWithStorage([&update_stmt](duckdb::Connection& con) {
                // Get the location of the model_name if local or global
                auto result = con.Query(
                        duckdb_fmt::format(" SELECT model_name, 'global' AS scope "
                                           "   FROM flock_storage.flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE"
                                           "  WHERE model_name = '{}'"
                                           " UNION ALL "
                                           " SELECT model_name, 'local' AS scope "
                                           "   FROM flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE"
                                           "  WHERE model_name = '{}';",
                                           update_stmt.model_name, update_stmt.model_name, update_stmt.model_name));

                auto& materialized_result = result->Cast<duckdb::MaterializedQueryResult>();
                if (materialized_result.RowCount() == 0) {
                    throw std::runtime_error(duckdb_fmt::format("Model '{}' doesn't exist.", update_stmt.model_name));
                }

                auto catalog = materialized_result.GetValue(1, 0).ToString() == "global" ? "flock_storage." : "";

                con.Query(duckdb_fmt::format(" UPDATE {}flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
                                             "    SET model = '{}', provider_name = '{}', "
                                             " model_args = '{}' WHERE model_name = '{}'; ",
                                             catalog, update_stmt.new_model, update_stmt.provider_name,
                                             update_stmt.new_model_args.dump(), update_stmt.model_name));

                return std::string("SELECT 'Model updated successfully' AS status");
            },
                                            false);
            break;
        }
        case StatementType::UPDATE_MODEL_SCOPE: {
            const auto& update_stmt = static_cast<const UpdateModelScopeStatement&>(statement);
            query = ExecuteQueryWithStorage([&update_stmt](duckdb::Connection& con) {
                auto result = con.Query(duckdb_fmt::format(" SELECT model_name "
                                                           "   FROM {}flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE"
                                                           "  WHERE model_name = '{}';",
                                                           update_stmt.catalog, update_stmt.model_name));

                auto& materialized_result = result->Cast<duckdb::MaterializedQueryResult>();
                if (materialized_result.RowCount() != 0) {
                    throw std::runtime_error(
                            duckdb_fmt::format("Model '{}' already exist in {} storage.", update_stmt.model_name,
                                               update_stmt.catalog == "flock_storage." ? "global" : "local"));
                }

                con.Query(duckdb_fmt::format("INSERT INTO {}flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
                                             "(model_name, model, provider_name, model_args) "
                                             "SELECT model_name, model, provider_name, model_args "
                                             "FROM {}flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
                                             "WHERE model_name = '{}'; ",
                                             update_stmt.catalog,
                                             update_stmt.catalog == "flock_storage." ? "" : "flock_storage.",
                                             update_stmt.model_name));

                con.Query(duckdb_fmt::format("DELETE FROM {}flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
                                             "WHERE model_name = '{}'; ",
                                             update_stmt.catalog == "flock_storage." ? "" : "flock_storage.",
                                             update_stmt.model_name));

                return std::string("SELECT 'Model scope updated successfully' AS status");
            },
                                            false);
            break;
        }
        case StatementType::GET_MODEL: {
            const auto& get_stmt = static_cast<const GetModelStatement&>(statement);
            query = ExecuteGetQuery(
                    duckdb_fmt::format("SELECT 'global' AS scope, * "
                                       "FROM flock_storage.flock_config.FLOCKMTL_MODEL_DEFAULT_INTERNAL_TABLE "
                                       "WHERE model_name = '{}' "
                                       "UNION ALL "
                                       "SELECT 'global' AS scope, * "
                                       "FROM flock_storage.flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
                                       "WHERE model_name = '{}'"
                                       "UNION ALL "
                                       "SELECT 'local' AS scope, * "
                                       "FROM flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE "
                                       "WHERE model_name = '{}';",
                                       get_stmt.model_name, get_stmt.model_name, get_stmt.model_name, get_stmt.model_name),
                    true);
            break;
        }

        case StatementType::GET_ALL_MODEL: {
            query = ExecuteGetQuery(
                    " SELECT 'global' AS scope, * "
                    " FROM flock_storage.flock_config.FLOCKMTL_MODEL_DEFAULT_INTERNAL_TABLE"
                    " UNION ALL "
                    " SELECT 'global' AS scope, * "
                    " FROM flock_storage.flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE"
                    " UNION ALL "
                    " SELECT 'local' AS scope, * "
                    " FROM flock_config.FLOCKMTL_MODEL_USER_DEFINED_INTERNAL_TABLE;",
                    true);
            break;
        }
        default:
            throw std::runtime_error("Unknown statement type.");
    }

    return query;
}

}// namespace flock
