#pragma once

#include "flock/core/common.hpp"
#include "flock/custom_parser/query_statements.hpp"
#include "flock/custom_parser/tokenizer.hpp"

#include "fmt/format.h"
#include <nlohmann/json.hpp>

namespace flock {

// Specific statements
class CreateModelStatement : public QueryStatement {
public:
    CreateModelStatement() { type = StatementType::CREATE_MODEL; }
    std::string catalog;
    std::string model_name;
    std::string model;
    std::string provider_name;
    nlohmann::json model_args;
};

class DeleteModelStatement : public QueryStatement {
public:
    DeleteModelStatement() { type = StatementType::DELETE_MODEL; }
    std::string model_name;
    std::string provider_name;
};

class UpdateModelScopeStatement : public QueryStatement {
public:
    UpdateModelScopeStatement() { type = StatementType::UPDATE_MODEL_SCOPE; }
    std::string model_name;
    std::string catalog;
};

class UpdateModelStatement : public QueryStatement {
public:
    UpdateModelStatement() { type = StatementType::UPDATE_MODEL; }
    std::string model_name;
    std::string new_model;
    std::string provider_name;
    nlohmann::json new_model_args;
};

class GetModelStatement : public QueryStatement {
public:
    GetModelStatement() { type = StatementType::GET_MODEL; }
    std::string model_name;
    std::string provider_name;
};

class GetAllModelStatement : public QueryStatement {
public:
    GetAllModelStatement() { type = StatementType::GET_ALL_MODEL; }
};

class ModelParser {
public:
    void Parse(const std::string& query, std::unique_ptr<QueryStatement>& statement);
    std::string ToSQL(const QueryStatement& statement) const;

private:
    void ParseCreateModel(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement);
    void ParseDeleteModel(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement);
    void ParseUpdateModel(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement);
    void ParseGetModel(Tokenizer& tokenizer, std::unique_ptr<QueryStatement>& statement);
};

}// namespace flock
