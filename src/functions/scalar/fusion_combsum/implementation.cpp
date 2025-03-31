#include "flockmtl/functions/scalar/fusion_combsum.hpp"

namespace flockmtl {

// performs CombSUM to merge lists based on a calculated score.
std::vector<double> FusionCombSUM::Operation(duckdb::DataChunk& args, const NormalizationMethod normalization_method) {
    int num_different_scores = static_cast<int>(args.ColumnCount());
    int num_entries = static_cast<int>(args.size());

    // we want to keep track of the cumulative combined score for each entry
    std::vector<std::pair<int, double>> cumulative_scores(num_entries);
    for (int i = 0; i < num_entries; i++) {
        cumulative_scores[i].first = i;
    }

    // for each column (scoring system), we want a vector of individual input scores
    for (int i = 0; i < num_different_scores; i++) {
        // extract a single column's score values. Initializing this way ensures 0 for null values
        std::vector<double> extracted_scores(num_entries);
        for (int j = 0; j < num_entries; j++) {
            auto valueWrapper = args.data[i].GetValue(j);
            // null values are left as 0, treated as if the entry is not present in that scoring system's results
            if (!valueWrapper.IsNull()) {
                double value = valueWrapper.GetValue<double>();
                if (!std::isnan(value)) {
                    extracted_scores[j] = valueWrapper.GetValue<double>();
                }
            }
        }

        // If all entries have the same score, then this scoring system can be considered useless and should be ignored
        if (std::adjacent_find(extracted_scores.begin(), extracted_scores.end(), std::not_equal_to<>()) == extracted_scores.end()) {
            continue;
        }

        // add this column's scores to the cumulative scores
        for (int k = 0; k < num_entries; k++) {
            cumulative_scores[k].second += extracted_scores[k];
        }
    }

    // sort the scores so we can obtain the rankings
    std::sort(cumulative_scores.begin(), cumulative_scores.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    // return the resulting ranking of all documents
    std::vector<double> results(num_entries);
    for (int i = 0; i < num_entries; i++) {
        const int tmp_index = cumulative_scores[i].first;
        // add 1 to rank because we want rankings to start at 1, not 0
        results[tmp_index] = cumulative_scores[i].second;
    }

    return results;
}

void FusionCombSUM::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto results = FusionCombSUM::Operation(args);

    auto index = 0;
    for (const auto& res : results) {
        result.SetValue(index++, duckdb::Value(res));
    }
}

} // namespace flockmtl
