#pragma once

#include "flock/functions/llm_function_bind_data.hpp"
#include "flock/functions/scalar/scalar.hpp"
#include <optional>

namespace flock {

class LlmFilter : public ScalarFunctionBase {
public:
    static duckdb::unique_ptr<duckdb::FunctionData> Bind(
            duckdb::ClientContext& context,
            duckdb::ScalarFunction& bound_function,
            duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments);
    static void ValidateArguments(duckdb::DataChunk& args);
    // A row that was never evaluated has no verdict, so it yields nullopt
    // and reaches SQL as NULL rather than being coerced to a decision.
    static std::vector<std::optional<std::string>> Operation(duckdb::DataChunk& args, LlmFunctionBindData* bind_data);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
};

}// namespace flock
