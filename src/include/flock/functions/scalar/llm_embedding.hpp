#pragma once

#include "flock/functions/scalar/scalar.hpp"

namespace flock {

class LlmEmbedding : public ScalarFunctionBase {
public:
    static void ValidateArguments(duckdb::DataChunk& args);
    static std::vector<duckdb::vector<duckdb::Value>> Operation(duckdb::DataChunk& args);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
};

}// namespace flock
