#pragma once

#include "flockmtl/functions/scalar/scalar.hpp"

namespace flockmtl {

class LlmFilter : public ScalarFunctionBase {
public:
    static void ValidateArguments(duckdb::DataChunk& args);
    static std::vector<std::string> Operation(duckdb::DataChunk& args, ExecutionMode mode);
    template<ExecutionMode MODE>
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
};

}// namespace flockmtl
