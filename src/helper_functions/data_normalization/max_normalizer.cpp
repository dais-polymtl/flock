#include "flockmtl/helper_functions/data_normalization/max_normalizer.hpp"

namespace flockmtl {

// Max-normalize a single list of scores, hit_counts keeps track of how many systems returned the entry (search hits)
void MaxNormalizer::normalize(std::vector<double>& scores, std::vector<int>& hit_counts) const {
    if (scores.empty()) {
        return;
    }

    // Ensure the num_entry_hits vector has the same size as scores
    if (hit_counts.size() != scores.size()) {
        throw std::runtime_error("fusion_combmnz: number of entries varied unexpectedly");
    }

    // Find max absolute value (to handle both positive/negative scores). We make sure to retrieve the absolute value
    double max_score = std::abs(
        *std::max_element(scores.begin(), scores.end(), [](double a, double b) { return std::abs(a) < std::abs(b); }));

    // Avoid division by zero (if all scores are 0, leave them unchanged)
    if (max_score == 0.0) {
        return;
    }

    // Normalize scores and increment hit counts for non-zero scores
    // The sign of the score is preserved during normalization because we retrieved the absolute value earlier
    for (int i = 0; i < scores.size(); ++i) {
        // Make sure we don't count a score of 0 as an entry hit
        if (scores[i] != 0.0) {
            scores[i] = scores[i] / max_score;
            hit_counts[i]++;
        }
    }
}

void MaxNormalizer::normalizeStatic(std::vector<double>& scores, std::vector<int>& hit_counts) {
    const MaxNormalizer max_normalizer;
    max_normalizer.normalize(scores, hit_counts);
}

} // namespace flockmtl