#include "flockmtl/functions/scalar/fusion_rrf.hpp"

namespace flockmtl {

void FusionRRF::ValidateArguments(duckdb::DataChunk& args) {
    for (int i = 0; i < static_cast<int>(args.ColumnCount()); i++) {
        if (args.data[i].GetType() != duckdb::LogicalType::DOUBLE) {
            throw std::runtime_error("fusion_embedding: argument must be a double");
        }
    }
}

std::vector<int> FusionRRF::Operation(duckdb::DataChunk& args) {
    FusionRRF::ValidateArguments(args);
    // recommended rrf constant is 60
    int rrf_constant = 60;
    int num_different_scores = static_cast<int>(args.ColumnCount());
    int num_entries = static_cast<int>(args.size());

    // we want to keep track of the cumulative RRF score for each entry
    std::vector<std::pair<int, double>> cumulative_scores(num_entries);
    for (int i = 0; i < num_entries; i++) {
        cumulative_scores[i].first = i;
    }

    // for each column, we want a vector of individual RRF scores
    for (int i = 0; i < num_different_scores; i++) {
        // extract a single column's score values and associated row position
        std::vector<std::pair<int, double>> extracted_column(num_entries);
        for (int j = 0; j < num_entries; j++) {
            // first item is it's position
            extracted_column[j].first = j;
            auto valueWrapper = args.data[i].GetValue(j);
            if (!valueWrapper.IsNull()) {
                double value = valueWrapper.GetValue<double>();
                extracted_column[j].second = value;
            }
        }
        // sort this vector of scores based on the scores
        std::sort(extracted_column.begin(), extracted_column.end(), [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

        // this vector will store the RRF scores for each individual column
        // TODO: Fix cases where the same value is there multiple times (ex: all zeroes)
        // TODO: If we're calculating array distance, we need to invert values (higher distance = lower similarity)
        // 1/score should work because only the ranking matters, not the scale
        std::vector<std::pair<int, double>> tmp_scores(num_entries);
        for (int j = 0; j < num_entries; j++) {
            tmp_scores[j].first = j;
            int cur_entry_num = extracted_column[j].first;
            // calculate RRF score at correct entry, j is the ranking position since we sorted the list
            tmp_scores[cur_entry_num].second = static_cast<double>(1) / (rrf_constant +  j + 1);
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
        results.push_back(cumulative_scores[i].first + 1);
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
