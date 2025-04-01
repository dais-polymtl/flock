#include "flockmtl/functions/scalar/fusion_combmed.hpp"

namespace flockmtl {

// performs CombMED to merge lists based on a calculated score.
std::vector<double> FusionCombMED::Operation(duckdb::DataChunk& args) {
    int num_different_scores = static_cast<int>(args.ColumnCount());
    int num_entries = static_cast<int>(args.size());

    // we want to keep track of all scores for each entry (in a vector)
    std::vector<std::vector<double>> cumulative_scores(num_entries);

    // for each column (scoring system), we want a vector of individual input scores so we can normalize them
    for (int i = 0; i < num_different_scores; i++) {
        // extract a single column's score values.
        std::vector<double> extracted_scores(num_entries);
        for (int j = 0; j < num_entries; j++) {
            auto valueWrapper = args.data[i].GetValue(j);
            // if there is a value, it is extracted. if there is no value (NULL/NaN), it stays at 0
            if (!valueWrapper.IsNull()) {
                if (const double value = valueWrapper.GetValue<double>(); !std::isnan(value)) {
                    extracted_scores[j] = valueWrapper.GetValue<double>();
                }
            }
        }

        // We make sure not to skip columns where all entries are the same.
        // If a retrieval system has all 0, it means the document wasn't found, which we want to keep in mind.

        // add this column's scores to the scores for this entry
        for (size_t k = 0; k < num_entries; k++) {
            cumulative_scores[k].push_back(extracted_scores[k]);
        }
    }

    // Now that all scores are normalized and extracted, we can calculate the median score for each entry
    std::vector<double> median_scores(num_entries);
    for (int i = 0; i < num_entries; i++) {
        median_scores[i] = FusionCombMED::calculateMedian(cumulative_scores[i]);
    }

    return median_scores;
}

void FusionCombMED::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto results = FusionCombMED::Operation(args);

    auto index = 0;
    for (const auto& res : results) {
        result.SetValue(index++, duckdb::Value(res));
    }
}

double FusionCombMED::calculateMedian(const std::vector<double>& scores) {
    if (scores.empty()) {
        return 0.0;
    }

    // Create a copy to avoid modifying the original vector
    std::vector<double> sorted_scores = scores;
    const size_t size = sorted_scores.size();
    const size_t mid = size / 2;

    // Sort the copy
    std::sort(sorted_scores.begin(), sorted_scores.end());

    if (size % 2 == 0) {
        // For even-sized vectors, average the two middle elements
        return (sorted_scores[mid - 1] + sorted_scores[mid]) / 2.0;
    } else {
        // For odd-sized vectors, return the middle element
        return sorted_scores[mid];
    }
}

} // namespace flockmtl
