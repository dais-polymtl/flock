#include "flockmtl/functions/scalar/fusion_rrf.hpp"

namespace flockmtl {

void FusionRRF::ValidateArguments(duckdb::DataChunk& args) {
    for (int i = 0; i < static_cast<int>(args.ColumnCount()); i++) {
        if (args.data[i].GetType() != duckdb::LogicalType::DOUBLE) {
            throw std::runtime_error("fusion_rrf: argument must be a double");
        }
    }
}

// performs RRF (Reciprocal Rank Fusion) to merge lists based on some score.
// Different entries with the same RRF score are assigned different, consecutive, rankings arbitrarily
std::vector<int> FusionRRF::Operation(duckdb::DataChunk& args) {
    FusionRRF::ValidateArguments(args);
    // recommended rrf constant is 60
    int rrf_constant = 60;
    int num_different_scores = static_cast<int>(args.ColumnCount());
    int num_entries = static_cast<int>(args.size());

    // the function is sometimes called with a singular entry, often when null values are present in a table
    // in these cases, we return -1 to say that a ranking is impossible/invalid
    if (num_entries == 1) {
        return std::vector<int>(1, -1);
    }

    // we want to keep track of the cumulative RRF score for each entry
    std::vector<std::pair<int, double>> cumulative_scores(num_entries);
    for (int i = 0; i < num_entries; i++) {
        cumulative_scores[i].first = i;
    }

    // for each column, we want a vector of individual input scores
    for (int i = 0; i < num_different_scores; i++) {
        // extract a single column's score values and associated row position
        std::vector<std::pair<int, double>> extracted_column(num_entries);
        for (int j = 0; j < num_entries; j++) {
            // first item is it's position
            extracted_column[j].first = j;
            auto valueWrapper = args.data[i].GetValue(j);
            // null values are set to -inf so that we can ignore them from affecting the calculations later
            if (valueWrapper.IsNull()) {
                extracted_column[j].second = -std::numeric_limits<double>::infinity();
            } else {
                double value = valueWrapper.GetValue<double>();
                extracted_column[j].second = value;
            }
        }
        // sort this vector of scores based on the scores
        std::sort(extracted_column.begin(), extracted_column.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

        // this vector will store the RRF scores for each individual column
        // we will account for cases where the given score is the same and assign the same RRF score
        std::vector<std::pair<int, double>> tmp_scores(num_entries);

        double previous_score = -std::numeric_limits<double>::infinity();
        int cur_rank = 0;
        for (int j = 0; j < num_entries; j++) {
            tmp_scores[j].first = j;        // the first and second values get filled out independently in this loop
            // for values which were null, skip calculating the rrf score, so the value remains zero
            if (extracted_column[j].second == -std::numeric_limits<double>::infinity()) {
                continue;
            }

            // if we see the same score multiple times, we want to assign the same rank
            if (extracted_column[j].second != previous_score) {
                previous_score = extracted_column[j].second;
                cur_rank++;
            }
            int cur_entry_num = extracted_column[j].first;
            // calculate RRF score at correct entry, j is the ranking position since we sorted the list
            tmp_scores[cur_entry_num].second = static_cast<double>(1) / (rrf_constant + cur_rank);
        }

        // add this column's scores to the cumulative scores
        for (int k = 0; k < num_entries; k++) {
            cumulative_scores[k].second += tmp_scores[k].second;
        }
    }

    // sort the scores so we can obtain the rankings
    std::sort(cumulative_scores.begin(), cumulative_scores.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    // return the resulting ranking of all documents
    std::vector<int> results(num_entries);
    for (int i = 0; i < num_entries; i++) {
        const int tmp_index = cumulative_scores[i].first;
        results[tmp_index] = i + 1;         // add 1 because we want rankings to start at 1, not 0
    }

    return results;
}

void FusionRRF::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto results = FusionRRF::Operation(args);

    auto index = 0;
    for (const auto& res : results) {
        result.SetValue(index++, duckdb::Value(res));
    }
}

} // namespace flockmtl
