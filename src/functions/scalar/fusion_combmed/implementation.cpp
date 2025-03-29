#include "flockmtl/functions/scalar/fusion_combmed.hpp"

namespace flockmtl {

// performs CombMED to merge lists based on a calculated score.
std::vector<std::string> FusionCombMED::Operation(duckdb::DataChunk& args, const NormalizationMethod normalization_method) {
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

    // we want to keep track of all scores for each entry (in a vector)
    std::vector<std::pair<int, std::vector<double>>> cumulative_scores(num_entries);
    for (int i = 0; i < num_entries; i++) {
        cumulative_scores[i].first = i;
    }

    // for each column (scoring system), we want a vector of individual input scores so we can normalize them
    for (int i = 0; i < num_different_scores; i++) {
        // extract a single column's score values. We will keep track of which values were NULL and not extract them.
        // This will allow normalizing and median calculations since extracted_scores has no NaN values in it.
        // This means extracted_scores is NOT the same size as the input size since NaN values are removed.
        // We will have to manually keep track of which were removed and where to maintain ordering.
        std::vector<double> extracted_scores(num_entries);
        std::vector<bool> score_is_nan(num_entries);
        int nan_count = 0;
        for (int j = 0; j < num_entries; j++) {
            auto valueWrapper = args.data[i].GetValue(j);
            // null values are marked as NaN, we will ignore them during calculations later
            if (!valueWrapper.IsNull()) {
                extracted_scores[j - nan_count] = valueWrapper.GetValue<double>();  // make sure all valid values follow
                score_is_nan[j] = false;
            } else {
                score_is_nan[j] = true;
                nan_count++;
            }
        }

        // truncate the extracted scores in case we had NaN values and need to remove the trailing zeroes
        extracted_scores.resize(num_entries - nan_count);

        // If all entries have the same score, then this scoring system can be considered useless and should be ignored
        if (std::adjacent_find(extracted_scores.begin(), extracted_scores.end(), std::not_equal_to<>()) == extracted_scores.end()) {
            continue;
        }

        // we now normalize each scoring system independently
        extracted_scores = Normalizer::normalize(extracted_scores, normalization_method);

        // add this column's scores to the scores for this entry while making sure to avoid NaN values
        size_t score_index = 0;
        for (size_t k = 0; k < num_entries; k++) {
            if (!score_is_nan[k]) {
                cumulative_scores[k].second.push_back(extracted_scores[score_index]);
                score_index++;   // only increment the extracted scores index for non-NaN entries, separately from k
            }
        }
    }

    // Now that all scores are normalized and extracted, we can calculate the median score for each entry
    std::vector<std::pair<int, double>> median_scores(num_entries);
    for (int i = 0; i < num_entries; i++) {
        median_scores[i].first = i;
        median_scores[i].second = FusionCombMED::calculateMedian(cumulative_scores[i].second);
    }

    // sort the medians so we can obtain the rankings
    std::sort(median_scores.begin(), median_scores.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    // return the resulting ranking of all documents
    std::vector<std::string> results(num_entries);
    for (size_t i = 0; i < num_entries; i++) {
        const int tmp_index = median_scores[i].first;
        // add 1 to rank because we want rankings to start at 1, not 0
        results[tmp_index] = std::to_string(i + 1) + " (" + std::to_string(median_scores[i].second) + ")";
    }

    return results;
}

void FusionCombMED::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto results = FusionCombMED::Operation(args);

    auto index = 0;
    for (const auto& res : results) {
        result.SetValue(index++, duckdb::Value(res));
    }
}

double FusionCombMED::calculateMedian(const std::vector<double>& data) {
    if (data.empty()) {
        return 0.0;
    }

    // Create a copy to avoid modifying the original vector
    std::vector<double> sorted_data = data;
    const size_t size = sorted_data.size();
    const size_t mid = size / 2;

    // Sort the copy
    std::sort(sorted_data.begin(), sorted_data.end());

    if (size % 2 == 0) {
        // For even-sized vectors, average the two middle elements
        return (sorted_data[mid - 1] + sorted_data[mid]) / 2.0;
    } else {
        // For odd-sized vectors, return the middle element
        return sorted_data[mid];
    }
}

} // namespace flockmtl
