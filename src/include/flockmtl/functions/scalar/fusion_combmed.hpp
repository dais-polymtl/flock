#pragma once

#include "flockmtl/functions/scalar/scalar.hpp"
#include "flockmtl/helper_functions/normalizer.hpp"

namespace flockmtl {

/**
 * Performs the CombMED algorithm to merge lists based multiple scoring sources,
 * as proposed by Fox et al. It takes the median of normalized scores for each entry.
 * This method is more robust to extreme values, making it less sensitive to outliers.
 * Unfortunately, there is no DOI for this paper.
 * Reference: Combination of Multiple Searches. Edward A. Fox and Joseph A. Shaw. NIST, 1993.
 */
class FusionCombMED : public ScalarFunctionBase {
public:
    static std::vector<std::string> Operation(duckdb::DataChunk& args, NormalizationMethod normalization_method = NormalizationMethod::MinMax);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);
private:
    static double calculateMedian(const std::vector<double>& data);
};

} // namespace flockmtl
