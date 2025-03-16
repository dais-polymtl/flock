#include "flockmtl/helper_functions/normalizer.hpp"

namespace flockmtl {

std::vector<double> Normalizer::normalize(const NormalizationMethod method, const std::vector<double>& scores) {
    return execute_normalization(method, scores);
}

std::vector<double> Normalizer::normalize(const NormalizationMethod method, const std::vector<double>& scores, std::vector<int>& hit_counts) {
    return execute_normalization(method, scores, &hit_counts);
}

std::vector<double> Normalizer::execute_normalization(const NormalizationMethod method, const std::vector<double>& scores, std::vector<int>* hit_counts) {
    // make sure we don't have to deal with an empty scores vector in any function
    if (scores.empty()) {
        return {};
    }

    switch (method) {
    case NormalizationMethod::Max:
        return normalizeMax(scores, hit_counts);
    default:
        throw std::invalid_argument("This normalization method is not yet supported.");
    }
}

// Max-normalize a single list of scores, hit_counts keeps track of how many systems returned the entry (search hits)
std::vector<double> Normalizer::normalizeMax(const std::vector<double>& scores, std::vector<int>* hit_counts) {
    // Ensure the num_entry_hits vector has the same size as scores
    if (hit_counts != nullptr && hit_counts->size() != scores.size()) {
        throw std::runtime_error("Max Normalization: number of entries varied unexpectedly");
    }

    // Find max absolute value (to handle both positive/negative scores). We make sure to retrieve the absolute value
    double max_score = std::abs(
        *std::max_element(scores.begin(), scores.end(), [](double a, double b) { return std::abs(a) < std::abs(b); }));

    // Avoid division by zero (if all scores are 0, return zeroes)
    if (max_score == 0.0) {
        return std::vector<double>(scores.size(), 0.0);
    }

    // Normalize scores and increment hit counts for non-zero scores
    // The sign of the score is preserved during normalization because we retrieved the absolute value earlier
    std::vector<double> normalized_scores(scores.size());
    for (size_t i = 0; i < scores.size(); ++i) {
        normalized_scores[i] = scores[i] / max_score;
        // Make sure we don't count a score of 0 as an entry hit
        if (hit_counts != nullptr && scores[i] != 0.0) {
            (*hit_counts)[i]++;
        }
    }
    return normalized_scores;
}

} // namespace flockmtl