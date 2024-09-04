#pragma once

#include "large_flock/common.hpp"
#include "large_flock/core/parser/lf_tokenizer.hpp"
#include "large_flock/core/parser/query_statements.hpp"

#include <memory>
#include <string>
#include <vector>

namespace large_flock {

namespace core {
// Specific prompt statements
class CreatePromptStatement : public QueryStatement {
public:
    CreatePromptStatement() {
        type = StatementType::CREATE_PROMPT;
    }
    std::string prompt_name;
    std::string prompt;
};

class DeletePromptStatement : public QueryStatement {
public:
    DeletePromptStatement() {
        type = StatementType::DELETE_PROMPT;
    }
    std::string prompt_name;
};

class UpdatePromptStatement : public QueryStatement {
public:
    UpdatePromptStatement() {
        type = StatementType::UPDATE_PROMPT;
    }
    std::string prompt_name;
    std::string new_prompt;
};

class GetPromptStatement : public QueryStatement {
public:
    GetPromptStatement() {
        type = StatementType::GET_PROMPT;
    }
    std::string prompt_name;
};

class GetAllPromptStatement : public QueryStatement {
public:
    GetAllPromptStatement() {
        type = StatementType::GET_ALL_PROMPT;
    }
};

class LfPromptParser {
public:
    void Parse(const std::string &query, std::unique_ptr<QueryStatement> &statement);
    std::string ToSQL(const QueryStatement &statement) const;

private:
    void ParseCreatePrompt(Tokenizer &tokenizer, std::unique_ptr<QueryStatement> &statement);
    void ParseDeletePrompt(Tokenizer &tokenizer, std::unique_ptr<QueryStatement> &statement);
    void ParseUpdatePrompt(Tokenizer &tokenizer, std::unique_ptr<QueryStatement> &statement);
    void ParseGetPrompt(Tokenizer &tokenizer, std::unique_ptr<QueryStatement> &statement);
};

} // namespace core

} // namespace large_flock
