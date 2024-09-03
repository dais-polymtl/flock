#pragma once

#include "large_flock/common.hpp"

#include <memory>
#include <string>
#include <vector>

namespace large_flock {
namespace core {
// Enum to represent different statement types
enum class StatementType {
    CREATE_MODEL,
    DELETE_MODEL,
    UPDATE_MODEL,
    GET_MODEL,
	GET_ALL_MODEL,
    CREATE_PROMPT,
    DELETE_PROMPT,
    UPDATE_PROMPT,
    GET_PROMPT,
	GET_ALL_PROMPT,
};

// Abstract base class for statements
class QueryStatement {
public:
    virtual ~QueryStatement() = default;
    StatementType type;
};

// Statement for 'CREATE DUCK' command
class CreateDuckStatement : public QueryStatement {
public:
    std::string duck_string;
};

} // namespace core

} // namespace large_flock