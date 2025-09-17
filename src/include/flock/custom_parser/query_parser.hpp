#pragma once

#include "flock/core/common.hpp"
#include "flock/custom_parser/query/model_parser.hpp"
#include "flock/custom_parser/query/prompt_parser.hpp"
#include "flock/custom_parser/query_statements.hpp"
#include "flock/custom_parser/tokenizer.hpp"

#include "fmt/format.h"
#include <memory>
#include <string>
#include <vector>

namespace flock {

class QueryParser {
public:
    std::string ParseQuery(const std::string& query);
    std::string ParsePromptOrModel(Tokenizer tokenizer, const std::string& query);

private:
    std::unique_ptr<QueryStatement> statement;
};

}// namespace flock
