#pragma once

#include "flockmtl/functions/scalar/scalar.hpp"

namespace flockmtl {

class FusionCombMNZ : public ScalarFunctionBase {
public:
    static void ValidateArguments(duckdb::DataChunk& args);
    static std::vector<std::string> Operation(duckdb::DataChunk& args);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
private:
    static void max_normalize(std::vector<double>& scores, std::vector<int>& hit_counts);
};

} // namespace flockmtl
