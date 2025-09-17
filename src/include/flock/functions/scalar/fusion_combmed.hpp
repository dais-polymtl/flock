#pragma once

#include "flock/functions/scalar/scalar.hpp"

namespace flock {

/**
 * Performs the CombMED algorithm to merge lists based multiple scoring sources,
 * as proposed by Fox et al. It takes the median of normalized scores for each entry.
 * This method is more robust to extreme values, making it less sensitive to outliers.
 * Reference: Combination of Multiple Searches. Edward A. Fox and Joseph A. Shaw. NIST, 1993.
 */
class FusionCombMED : public ScalarFunctionBase {
public:
    static std::vector<double> Operation(duckdb::DataChunk& args);
    static void Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result);

private:
    static double calculateMedian(const std::vector<double>& scores);
};

}// namespace flock
