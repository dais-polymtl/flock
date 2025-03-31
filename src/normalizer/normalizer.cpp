#include "flockmtl/helper_functions/normalizer.hpp"

namespace flockmtl {

// public
std::vector<double> Normalizer::normalize(const std::vector<double>& scores, std::vector<int>& hit_counts,
                                          const NormalizationMethod method) {
    return execute_normalization(scores, std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::quiet_NaN(), method, &hit_counts);
}

std::vector<double> Normalizer::normalize(const std::vector<double>& scores, double max_value,
                                          std::vector<int>& hit_counts, const NormalizationMethod method) {
    return execute_normalization(scores, std::numeric_limits<double>::quiet_NaN(), max_value, method, &hit_counts);
}

std::vector<double> Normalizer::normalize(const std::vector<double>& scores, double min_value, double max_value,
                                          std::vector<int>& hit_counts, const NormalizationMethod method) {
    return execute_normalization(scores, min_value, max_value, method, &hit_counts);
}

std::vector<double> Normalizer::normalize(const std::vector<double>& scores, const NormalizationMethod method) {
    return execute_normalization(scores, std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<double>::quiet_NaN(), method);
}

std::vector<double> Normalizer::normalize(const std::vector<double>& scores, double max_value,
                                          const NormalizationMethod method) {
    return execute_normalization(scores, std::numeric_limits<double>::quiet_NaN(), max_value, method);
}
std::vector<double> Normalizer::normalize(const std::vector<double>& scores, double min_value, double max_value,
                                          const NormalizationMethod method) {
    return execute_normalization(scores, min_value, max_value, method);
}

// private
std::vector<double> Normalizer::execute_normalization(const std::vector<double>& scores, double min_value,
                                                      double max_value, const NormalizationMethod method,
                                                      std::vector<int>* hit_counts) {
    // make sure we don't have to deal with an empty scores vector in any function
    if (scores.empty()) {
        return {};
    }

    switch (method) {
    case NormalizationMethod::Max:
        return normalizeMax(scores, min_value, hit_counts);
    case NormalizationMethod::MinMax:
        return normalizeMinMax(scores, min_value, max_value, hit_counts);
    default:
        throw std::invalid_argument("Invalid normalization method.");
    }
}

// Max-normalize a single list of scores, hit_counts keeps track of how many systems returned the entry (search hits)
// Warning: If all entries have the same score, they all get 0.5 (by convention, can be modified if we want to)
std::vector<double> Normalizer::normalizeMax(const std::vector<double>& scores, double max_value,
                                            std::vector<int>* hit_counts) {
    // Ensure the num_entry_hits vector has the same size as scores
    if (hit_counts != nullptr && hit_counts->size() != scores.size()) {
        throw std::runtime_error("Max Normalization: number of entries varied unexpectedly");
    }

    // If all entries have the same score, then we return 0.5 for all values (by convention)
    if (std::adjacent_find(scores.begin(), scores.end(), std::not_equal_to<>()) == scores.end()) {
        return std::vector<double>(scores.size(), 0.5);
    }

    // If no max value was passed, find max absolute value (to handle both positive/negative scores). We make sure to retrieve the absolute value
    if (std::isnan(max_value)) {
        max_value = std::abs(*std::max_element(scores.begin(), scores.end(),
                                               [](double a, double b) { return std::abs(a) < std::abs(b); }));
    }


    // Avoid division by zero (if all scores are 0, return zeroes)
    if (max_value == 0.0) {
        return std::vector<double>(scores.size(), 0.0);
    }

    // Normalize scores and increment hit counts for non-zero scores
    // The sign of the score is preserved during normalization because we retrieved the absolute value earlier
    std::vector<double> normalized_scores(scores.size());
    for (size_t i = 0; i < scores.size(); i++) {
        normalized_scores[i] = scores[i] / max_value;
        // Make sure we don't count a negative score or 0 as an entry hit
        if (hit_counts != nullptr && scores[i] > 0.0) {
            (*hit_counts)[i]++;
        }
    }
    return normalized_scores;
}

// Min-max normalize a single list of scores, hit_counts keeps track of how many systems returned the entry (search hits)
// Warning: If all entries have the same score, they all get 0.5 (by convention, can be modified)
std::vector<double> Normalizer::normalizeMinMax(const std::vector<double>& scores, double min_value, double max_value,
                                               std::vector<int>* hit_counts) {
    // Ensure the hit_counts vector has the same size as scores if provided
    if (hit_counts != nullptr && hit_counts->size() != scores.size()) {
        throw std::runtime_error("Min-Max Normalization: number of entries varied unexpectedly");
    }

    // If they weren't provided, find min and max values. No need to take care of sign because we divide by range.
    if (std::isnan(min_value) && std::isnan(max_value)) {
        auto [min_it, max_it] = std::minmax_element(scores.begin(), scores.end());
        min_value = *min_it;
        max_value = *max_it;
    }

    if (std::isnan(max_value)) {
        max_value = *std::max_element(scores.begin(), scores.end());
    }

    if (std::isnan(min_value)) {
        min_value = *std::min_element(scores.begin(), scores.end());
    }

    // Calculate range
    const double range = max_value - min_value;

    // If all values are the same, they're all normalized to 0.5
    if (range == 0.0) {
        return std::vector<double>(scores.size(), 0.5);
    }

    // Normalize scores and increment hit counts for non-zero scores
    std::vector<double> normalized_scores(scores.size());
    for (size_t i = 0; i < scores.size(); i++) {
        // sometimes, we get passed a score of 0 originating from NULL values which don't reflect the minimum score
        // As such, any value less than the passed minimum value is assigned 0 for normalization
        if (scores[i] < min_value) {
            if (scores[i] > 0.0) {
                std::cerr << "Warning: Min-Max Normalization: Positive value found smaller than provided min_value: " << scores[i] << std::endl;
            }
            normalized_scores[i] = 0.0;
        } else {
            normalized_scores[i] = (scores[i] - min_value) / range;
        }

        // Make sure we don't count a negative score or 0 as an entry hit
        if (hit_counts != nullptr && scores[i] > 0.0) {
            (*hit_counts)[i]++;
        }
    }

    return normalized_scores;
}

} // namespace flockmtl