#pragma once

#include "flockmtl/functions/scalar/scalar.hpp"

namespace flockmtl {

/**
 * Performs the CombMED algorithm to merge lists based multiple scoring sources,
 * as proposed by Fox et al. It takes the median of normalized scores for each entry.
 * This method is more robust to extreme values, making it less sensitive to outliers.
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
class FusionCombMED : public ScalarFunctionBase {
public:
    static void ValidateArguments(duckdb::DataChunk& args);
    static std::vector<std::string> Operation(duckdb::DataChunk& args);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
private:
    static double calculateMedian(const std::vector<double>& data);
};

} // namespace flockmtl
