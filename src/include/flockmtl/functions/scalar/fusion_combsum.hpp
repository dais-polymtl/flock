#pragma once

#include "flockmtl/functions/scalar/scalar.hpp"

namespace flockmtl {

/**
 * Performs the CombSUM algorithm to merge lists based multiple scoring sources,
 * as proposed by Fox et al. It sums the normalized scores for each entry.
 * Reference: Combination of Multiple Searches. Edward A. Fox and Joseph A. Shaw. NIST, 1993.
 */
class FusionCombSUM : public ScalarFunctionBase {
public:
    static std::vector<double> Operation(duckdb::DataChunk& args);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
};

} // namespace flockmtl
