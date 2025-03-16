#pragma once

#include "flockmtl/functions/scalar/scalar.hpp"

namespace flockmtl {

/**
 * Performs the CombMNZ algorithm to merge lists based multiple scoring sources,
 * as proposed by Fox et al. Unfortunately, there is no DOI for this paper.
 *
 * Authors   = Edward A. Fox and Joseph A. Shaw
 *
 * Title     = Combination of Multiple Searches
 *
 * Publisher = National Institute of Standards and Technology (NIST)
 *
 * Year      = 1993
 */
class FusionCombMNZ : public ScalarFunctionBase {
public:
    static void ValidateArguments(duckdb::DataChunk& args);
    static std::vector<std::string> Operation(duckdb::DataChunk& args);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
private:
    static void max_normalize(std::vector<double>& scores, std::vector<int>& hit_counts);
};

} // namespace flockmtl
