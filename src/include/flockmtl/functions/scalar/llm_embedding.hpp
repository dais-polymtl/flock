#pragma once

#include "flockmtl/functions/scalar/scalar.hpp"

namespace flockmtl {

class LlmEmbedding : public ScalarFunctionBase {
public:
    static void ValidateArguments(duckdb::DataChunk& args);
    static std::vector<duckdb::vector<duckdb::Value>> Operation(duckdb::DataChunk& args, ExecutionMode mode = ExecutionMode::ASYNC);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
};

}// namespace flockmtl
