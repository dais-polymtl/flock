#include "flockmtl/functions/scalar/fusion_combmnz.hpp"
#include "flockmtl/helper_functions/normalizer.hpp"

namespace flockmtl {

void FusionCombMNZ::ValidateArguments(duckdb::DataChunk& args) {
    for (int i = 0; i < static_cast<int>(args.ColumnCount()); i++) {
        if (args.data[i].GetType() != duckdb::LogicalType::DOUBLE) {
            throw std::runtime_error("fusion_combmnz: argument must be a double");
        }
    }
}

// performs CombMNZ to merge lists based on a calculated score.
std::vector<std::string> FusionCombMNZ::Operation(duckdb::DataChunk& args) {
    FusionCombMNZ::ValidateArguments(args);
    int num_different_scores = static_cast<int>(args.ColumnCount());
    int num_entries = static_cast<int>(args.size());

    // the function is sometimes called with a singular entry, often when null values are present in a table
    // in these cases, we return -1 to say that a ranking is impossible/invalid
    if (num_entries == 1) {
        return std::vector<std::string>(1, "-1 (INVALID)");
    }
    if (num_entries == 0) {
        return std::vector<std::string>(1, "");
    }

    // we want to keep track of the cumulative combined score for each entry
    std::vector<std::pair<int, double>> cumulative_scores(num_entries);
    for (int i = 0; i < num_entries; i++) {
        cumulative_scores[i].first = i;
    }

    // we will need to remember how many scoring systems have a "hit" for each entry (ie in how many searches the entry is present)
    std::vector<int> hit_counts(num_entries);

    // for each column (scoring system), we want a vector of individual input scores
    for (int i = 0; i < num_different_scores; i++) {
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

        // we now normalize each scoring system independently, increasing hit counts appropriately
        extracted_scores = Normalizer::normalize(NormalizationMethod::MinMax, extracted_scores, hit_counts);

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
    std::vector<std::string> results(num_entries);
    for (int i = 0; i < num_entries; i++) {
        const int tmp_index = cumulative_scores[i].first;
        // add 1 to rank because we want rankings to start at 1, not 0
        results[tmp_index] = std::to_string(i + 1) + " (" + std::to_string(cumulative_scores[i].second) + ")";
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
