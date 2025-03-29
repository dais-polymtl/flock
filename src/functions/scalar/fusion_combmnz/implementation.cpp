#include "flockmtl/functions/scalar/fusion_combmnz.hpp"

namespace flockmtl {

// performs CombMNZ to merge lists based on a calculated score.
std::vector<double> FusionCombMNZ::Operation(duckdb::DataChunk& args, const NormalizationMethod normalization_method) {
    int num_columns = static_cast<int>(args.ColumnCount());
    int num_entries = static_cast<int>(args.size());

    // we want to keep track of the cumulative combined score for each entry
    std::vector<std::pair<int, double>> cumulative_scores(num_entries);
    for (int i = 0; i < num_entries; i++) {
        cumulative_scores[i].first = i;
    }

    // we will need to remember how many scoring systems have a "hit" for each entry (ie in how many searches the entry is present)
    std::vector<int> hit_counts(num_entries);

    // for each column (scoring system), we want a vector of individual input scores
    // we increment i by 3 because each scoring system is followed by its min and max value
    // we check for num_columns - 2 because each scoring system takes 3 columns, we want to stop without reaching min and max columns
    for (int i = 0; i < num_columns - 2; i += 3) {
        // extract a single column's score values. Initializing this way ensures 0 for null values
        std::vector<double> extracted_scores(num_entries);
        for (int j = 0; j < num_entries; j++) {
            auto valueWrapper = args.data[i].GetValue(j);
            // null values are left as 0, treated as if the entry is not present in that scoring system's results
            if (!valueWrapper.IsNull()) {
                extracted_scores[j] = valueWrapper.GetValue<double>();
            }
        }

        // If all entries have the same score, then this scoring system can be considered useless and should be ignored
        if (std::adjacent_find(extracted_scores.begin(), extracted_scores.end(), std::not_equal_to<>()) == extracted_scores.end()) {
            continue;
        }

        // before normalizing, we need the minimum and maximum values for the column
        double min_value = args.data[i+1].GetValue(0).GetValue<double>();
        double max_value = args.data[i+2].GetValue(0).GetValue<double>();

        // we now normalize each scoring system independently, increasing hit counts appropriately
        extracted_scores = Normalizer::normalize(extracted_scores, min_value, max_value, hit_counts, normalization_method);

        // add this column's scores to the cumulative scores
        for (int k = 0; k < num_entries; k++) {
            cumulative_scores[k].second += extracted_scores[k];
        }
    }

    // multiply each score by the number of systems which returned it
    for (int i = 0; i < num_entries; i++) {
        cumulative_scores[i].second *= hit_counts[i];
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

void FusionCombMNZ::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto results = FusionCombMNZ::Operation(args);

    auto index = 0;
    for (const auto& res : results) {
        result.SetValue(index++, duckdb::Value(res));
    }
}

} // namespace flockmtl
