#pragma once
#include "flockmtl/core/common.hpp"

namespace flockmtl {

/**
 * Enum containing the Normalization methods available
 */
enum class NormalizationMethod {
    Max,
    MinMax
};

/**
 * Normalizer class that implements vector normalization strategies.
 */
class Normalizer {
public:
    /**
     * Normalize method which will take an enum value to determine the normalization strategy.
     * This function will also update hit_counts, incrementing the entries for which a non-zero score was provided by 1.
     * @param scores Vector of scores to normalize
     * @param hit_counts Vector which tracks hit counts. Must be the same size as scores and will only increment on hit (modified in-place)
     * @param method Normalization method to use
     * @return The normalized vector
     */
    static std::vector<double> normalize(const std::vector<double>& scores,
                                         std::vector<int>& hit_counts,
                                         const NormalizationMethod method = NormalizationMethod::MinMax);

    static std::vector<double> normalize(const std::vector<double>& scores,
                                         double max_value,
                                         std::vector<int>& hit_counts,
                                         const NormalizationMethod method = NormalizationMethod::MinMax);

    static std::vector<double> normalize(const std::vector<double>& scores,
                                       double min_value,
                                       double max_value,
                                       std::vector<int>& hit_counts,
                                       const NormalizationMethod method = NormalizationMethod::MinMax);

    /**
     * Normalize method which will take an enum value to determine the normalization strategy.
     * @param scores Vector of scores to normalize
     * @param method Normalization method to use
     * @return The normalized vector
     */
    static std::vector<double> normalize(const std::vector<double>& scores, const NormalizationMethod method = NormalizationMethod::MinMax);

    static std::vector<double> normalize(const std::vector<double>& scores,
                                         double max_value,
                                         const NormalizationMethod method = NormalizationMethod::MinMax);

    static std::vector<double> normalize(const std::vector<double>& scores,
                                         double min_value,
                                         double max_value,
                                         const NormalizationMethod method = NormalizationMethod::MinMax);

private:
    /**
     * This function is here to allow two normalize functions to be provided to the user without the need to use pointers or pass addresses for hit_counts
     */
    // static std::vector<double> execute_normalization(const std::vector<double>& scores, const NormalizationMethod method,
    //                                                  std::vector<int>* hit_counts = nullptr);
    //
    // static std::vector<double> execute_normalization(const std::vector<double>& scores, double max_value,
    //                                                  const NormalizationMethod method,
    //                                                  std::vector<int>* hit_counts = nullptr);

    static std::vector<double> execute_normalization(const std::vector<double>& scores, double min_value,
                                                     double max_value, const NormalizationMethod method,
                                                     std::vector<int>* hit_counts = nullptr);

    /**
     * Method to max normalize a vector of scores.
     *
     * Warning: If all entries have the same score, they all get 0.5 (by convention, can be modified)
     * @param scores Vector of scores to normalize
     * @param max_value maximum value present in the entire dataset. Helps prevent limited view on data from applying
     * local normalization instead of global
     * @param hit_counts Vector which tracks hit counts. Must be the same size as scores and will only increment on hit
     * (modified in-place)
     * @return The normalized vector
     */
    static std::vector<double> normalizeMax(const std::vector<double>& scores, double max_value,
                                            std::vector<int>* hit_counts = nullptr);

    /**
     * Method to min-max normalize a vector of scores.
     *
     * Warning: If all entries have the same score, they all get 0.5 (by convention, can be modified)
     * @param scores Vector of scores to normalize
     * @param min_value maximum value present in the entire dataset. Helps prevent limited view on data from applying
     * local normalization instead of global
     * @param max_value minimum value present in the entire dataset. Helps prevent limited view on data from applying
     * local normalization instead of global
     * @param hit_counts Vector which tracks hit counts. Must be the same size as scores and will only increment on hit
     * (modified in-place)
     * @return The normalized vector
     */
    static std::vector<double> normalizeMinMax(const std::vector<double>& scores, double min_value, double max_value,
                                               std::vector<int>* hit_counts = nullptr);
};

} // namespace flockmtl