#pragma once

#include "flock/functions/llm_function_bind_data.hpp"
#include "flock/functions/scalar/scalar.hpp"

namespace flock {

class AiClassify : public ScalarFunctionBase {
public:
    static duckdb::unique_ptr<duckdb::FunctionData> Bind(
            duckdb::ClientContext& context,
            duckdb::ScalarFunction& bound_function,
            duckdb::vector<duckdb::unique_ptr<duckdb::Expression>>& arguments);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
};

}// namespace flock
