#include "flock/functions/scalar/fusion_combsum.hpp"

namespace flock {

// performs CombSUM to merge lists based on a calculated score.
std::vector<double> FusionCombSUM::Operation(duckdb::DataChunk& args) {
    int num_different_scores = static_cast<int>(args.ColumnCount());
    int num_entries = static_cast<int>(args.size());

    // we want to keep track of the cumulative combined score for each entry
    std::vector<double> cumulative_scores(num_entries);

    // for each column (scoring system), we want a vector of individual input scores
    for (int i = 0; i < num_different_scores; i++) {
        // extract a single column's score values. Initializing this way ensures 0 for null values
        std::vector<double> extracted_scores(num_entries);
        for (int j = 0; j < num_entries; j++) {
            auto valueWrapper = args.data[i].GetValue(j);
            // null values are left as 0, treated as if the entry is not present in that scoring system's results
            if (!valueWrapper.IsNull()) {
                if (const double value = valueWrapper.GetValue<double>(); !std::isnan(value)) {
                    extracted_scores[j] = value;
                }
            }
        }

        // add this column's scores to the cumulative scores
        for (int k = 0; k < num_entries; k++) {
            cumulative_scores[k] += extracted_scores[k];
        }
    }

    return cumulative_scores;
}

void FusionCombSUM::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto results = FusionCombSUM::Operation(args);

    auto index = 0;
    for (const auto& res: results) {
        result.SetValue(index++, duckdb::Value(res));
    }
}

}// namespace flock
