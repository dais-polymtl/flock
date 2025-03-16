#pragma once
#include "flockmtl/core/common.hpp"

namespace flockmtl {

/**
 * Enum containing the Normalization methods available
 */
enum class NormalizationMethod {
    Max
};

/**
 * Normalizer class that implements vector normalization strategies.
 */
class Normalizer {
public:
    /**
     * Normalize method which will take an enum value to determine the normalization strategy.
     * This function will also update hit_counts, incrementing the entries for which a non-zero score was provided by 1
     * @param method Normalization method to use
     * @param scores Vector of scores to normalize
     * @param hit_counts Vector which tracks hit counts. Must be the same size as scores and will only increment on hit (modified in-place)
     * @return The normalized vector
     */
    static std::vector<double> normalize(const NormalizationMethod method, const std::vector<double>& scores,
                                         std::vector<int>& hit_counts);

    /**
     * Normalize method which will take an enum value to determine the normalization strategy.
     * @param method Normalization method to use
     * @param scores Vector of scores to normalize
     * @return The normalized vector
     */
    static std::vector<double> normalize(const NormalizationMethod method, const std::vector<double>& scores);

private:
    /**
     * This function is here to allow two normalize functions to be provided to the user without the need to use pointers or pass addresses for hit_counts
     */
    static std::vector<double> execute_normalization(const NormalizationMethod method, const std::vector<double>& scores, std::vector<int>* hit_counts = nullptr);

    /**
     * Method to max-normalize a vector of scores.
     *
     * Warning: If all entries have the same score, they all get the best score possible (1.0)
     * @param scores Vector of scores to normalize
     * @param hit_counts Vector which tracks hit counts. Must be the same size as scores and will only increment on hit (modified in-place)
     * @return The normalized vector
     */
    static std::vector<double> normalizeMax(const std::vector<double>& scores, std::vector<int>* hit_counts = nullptr);
};

} // namespace flockmtl