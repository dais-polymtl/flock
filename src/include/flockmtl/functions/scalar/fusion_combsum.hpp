#pragma once

#include "flockmtl/functions/scalar/scalar.hpp"
#include "flockmtl/helper_functions/normalizer.hpp"

namespace flockmtl {

/**
 * Performs the CombSUM algorithm to merge lists based multiple scoring sources,
 * as proposed by Fox et al. It sums the normalized scores for each entry.
 * Unfortunately, there is no DOI for this paper.
 *
 * Authors   = Edward A. Fox and Joseph A. Shaw
 *
 * Title     = Combination of Multiple Searches
 *
 * Publisher = National Institute of Standards and Technology (NIST)
 *
 * Year      = 1993
 */
class FusionCombSUM : public ScalarFunctionBase {
public:
    static void ValidateArguments(duckdb::DataChunk& args);
    static std::vector<std::string> Operation(duckdb::DataChunk& args, NormalizationMethod normalization_method = NormalizationMethod::MinMax);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
};

} // namespace flockmtl
