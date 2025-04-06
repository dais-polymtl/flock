#include "flockmtl/functions/scalar/fusion_rrf.hpp"

namespace flockmtl {

// performs RRF (Reciprocal Rank Fusion) to merge lists based on some score.
// Different entries with the same RRF score are assigned different, consecutive, rankings arbitrarily
std::vector<double> FusionRRF::Operation(duckdb::DataChunk& args) {
    // recommended rrf constant is 60
    int32_t rrf_constant = 60;
    int num_different_scores = static_cast<int>(args.ColumnCount());
    int num_entries = static_cast<int>(args.size());

    // we want to keep track of the cumulative RRF score for each entry
    std::vector<double> cumulative_scores(num_entries);

    // for each column (scoring system), we want a vector of individual input scores
    for (int i = 0; i < num_different_scores; i++) {
        // extract a single column's rankings. There should be no null values
        std::vector<int32_t> extracted_ranks(num_entries);
        for (int j = 0; j < num_entries; j++) {
            auto valueWrapper = args.data[i].GetValue(j);
            if (!valueWrapper.IsNull()) {
                int32_t value = valueWrapper.GetValue<int32_t>();
                extracted_ranks[j] = value;
            }
        }

        // If all entries have the same score or are NULL (0), then this scoring system can be considered useless and
        // should be ignored Or else, all entries would get assigned the best rank possible, even if they are 0
        if (std::all_of(extracted_ranks.begin(), extracted_ranks.end(), [](const auto& rank) { return rank <= 1; })) {
            // if there is only one entry and the rank isn't 0, it's a valid ranking. We don't want to skip.
            if (!(num_entries == 1 && extracted_ranks[0] == 1)) {
                continue;
            }
        }

        // add this column's scores to the cumulative scores
        for (int k = 0; k < num_entries; k++) {
            cumulative_scores[k] += static_cast<double>(1) / (rrf_constant + extracted_ranks[k]);
        }
    }

    return cumulative_scores;
}

void FusionRRF::Execute(duckdb::DataChunk& args, duckdb::ExpressionState& state, duckdb::Vector& result) {
    auto results = FusionRRF::Operation(args);

    auto index = 0;
    for (const auto& res : results) {
        result.SetValue(index++, duckdb::Value(res));
    }
}

} // namespace flockmtl
