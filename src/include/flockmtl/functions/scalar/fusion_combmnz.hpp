#pragma once

#include "flockmtl/functions/scalar/scalar.hpp"
#include "flockmtl/helper_functions/normalizer.hpp"

namespace flockmtl {

/**
 * Performs the CombMNZ algorithm to merge lists based multiple scoring sources,
 * as proposed by Fox et al. Compared to CombSUM, this algorithm is less heavily influenced by high scores from any single system.
 * Instead, it heavily rewards entries found by multiple scoring systems (the more, the better).
 * Unfortunately, there is no DOI for this paper.
 * Reference: Combination of Multiple Searches. Edward A. Fox and Joseph A. Shaw. NIST, 1993.
 */
class FusionCombMNZ : public ScalarFunctionBase {
public:
    static std::vector<double> Operation(duckdb::DataChunk& args, NormalizationMethod normalization_method = NormalizationMethod::MinMax);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
};

} // namespace flockmtl
