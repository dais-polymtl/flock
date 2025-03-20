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
    case NormalizationMethod::MinMax:
        return normalizeMinMax(scores, hit_counts);
    case NormalizationMethod::ZScore:
        return normalizeZScore(scores, hit_counts);
    default:
        throw std::invalid_argument("Invalid normalization method.");
    }
}

// Max-normalize a single list of scores, hit_counts keeps track of how many systems returned the entry (search hits)
// Warning: If all entries have the same score, they all get 0.5 (by convention, can be modified if we want to)
std::vector<double> Normalizer::normalizeMax(const std::vector<double>& scores, std::vector<int>* hit_counts) {
    // Ensure the num_entry_hits vector has the same size as scores
    if (hit_counts != nullptr && hit_counts->size() != scores.size()) {
        throw std::runtime_error("Max Normalization: number of entries varied unexpectedly");
    }

    // If all entries have the same score, then we return 0.5 for all values (by convention)
    if (std::adjacent_find(scores.begin(), scores.end(), std::not_equal_to<>()) == scores.end()) {
        return std::vector<double>(scores.size(), 0.5);
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
    for (size_t i = 0; i < scores.size(); i++) {
        normalized_scores[i] = scores[i] / max_score;
        // Make sure we don't count a score of 0 as an entry hit
        if (hit_counts != nullptr && scores[i] != 0.0) {
            (*hit_counts)[i]++;
        }
    }
    return normalized_scores;
}

// Min-max normalize a single list of scores, hit_counts keeps track of how many systems returned the entry (search hits)
// Warning: If all entries have the same score, they all get 0.5 (by convention, can be modified)
std::vector<double> Normalizer::normalizeMinMax(const std::vector<double>& scores, std::vector<int>* hit_counts) {
    // Ensure the hit_counts vector has the same size as scores if provided
    if (hit_counts != nullptr && hit_counts->size() != scores.size()) {
        throw std::runtime_error("Min-Max Normalization: number of entries varied unexpectedly");
    }

    // Find min and max values
    auto [min_it, max_it] = std::minmax_element(scores.begin(), scores.end());
    const double min_score = *min_it;
    const double max_score = *max_it;

    // Calculate range
    const double range = max_score - min_score;

    // If all values are the same, they're all normalized to 0.5
    if (range == 0.0) {
        return std::vector<double>(scores.size(), 0.5);
    }

    // Normalize scores and increment hit counts for non-zero scores
    std::vector<double> normalized_scores(scores.size());
    for (size_t i = 0; i < scores.size(); i++) {
        normalized_scores[i] = (scores[i] - min_score) / range;

        // Make sure we don't count a score of 0 as an entry hit
        if (hit_counts != nullptr && scores[i] != 0.0) {
            (*hit_counts)[i]++;
        }
    }

    return normalized_scores;
}

// Z-score normalize (standardize) a list of scores. Transforms scores to have mean 0 and standard deviation 1
// WARNING: This normalization method will have some negative values. The normalized scores are not on a [0,1] scale.
// Scores of 0 are not ignored (as if the scoring system hasn't found the document). Instead, a poor score is assigned.
// As such, this method is not recommended for use in score-based fusion algorithms.
// If you choose to use it, be wary of entries with a score of 0 (potentially exclude them from the list of scores provided here),
// and you may need to offset all values so that all values are positive. Even then, make sure all your scoring sources lie on the same scale.
// Min-max normalization is recommended.
// hit_counts keeps track of how many systems returned the entry (search hits)
std::vector<double> Normalizer::normalizeZScore(const std::vector<double>& scores, std::vector<int>* hit_counts) {
    // Ensure the hit_counts vector has the same size as scores if provided
    if (hit_counts != nullptr && hit_counts->size() != scores.size()) {
        throw std::runtime_error("Z-Score Normalization: number of entries varied unexpectedly");
    }

    // Need at least two elements for proper standardization
    if (scores.size() < 2) {
        // For single element or empty vectors, return original value or empty vector
        std::vector<double> result = scores;

        // Still increment hit count if needed
        if (hit_counts != nullptr && !scores.empty() && scores[0] != 0.0) {
            (*hit_counts)[0]++;
        }

        return result;
    }

    // Calculate mean
    const double sum = std::accumulate(scores.begin(), scores.end(), 0.0);
    const double mean = sum / scores.size();

    // Calculate standard deviation using a second, less known variance formula: (sum(x^2)/n) - (mean)^2
    // we use std::inner_product to calculate the squared sum of all entries
    const double squared_sum = std::inner_product(scores.begin(), scores.end(), scores.begin(), 0.0);
    double variance = (squared_sum / scores.size()) - (mean * mean);

    // Handle numerical issues that might lead to tiny negative variance
    variance = std::max(variance, 0.0);

    const double std_dev = std::sqrt(variance);

    // If all values are the same, they're all normalized to 0 (convention for z-score normalization, because values are centered around 0)
    if (std_dev == 0.0) {
        return std::vector<double>(scores.size(), 0.0);
    }

    // Normalize scores and increment hit counts for non-zero scores
    std::vector<double> normalized_scores(scores.size());
    for (size_t i = 0; i < scores.size(); i++) {
        // Apply z-score formula: z = (x - mean) / std_dev
        normalized_scores[i] = (scores[i] - mean) / std_dev;

        // Increment hit count for non-zero entries
        if (hit_counts != nullptr && scores[i] != 0.0) {
            (*hit_counts)[i]++;
        }
    }

    return normalized_scores;
}

} // namespace flockmtl