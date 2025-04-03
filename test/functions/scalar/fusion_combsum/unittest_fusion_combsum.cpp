#include "flockmtl-test/functions/scalar/test_fusion.hpp"
#include <flockmtl/functions/scalar/fusion_combsum.hpp>

using namespace duckdb;

// CombSum takes normalized data as input
TEST_CASE("Unit test for flockmtl::FusionCombSUM with 2 DOUBLES", "[fusion_combsum][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 1 (one row)
    chunk.Initialize(allocator, types, 1);

    // Set the cardinality (number of rows) to 1
    chunk.SetCardinality(1);

    // Populate the DataChunk with test data
    chunk.SetValue(0, 0, 0.5);
    chunk.SetValue(1, 0, 0.2);

    // Call FusionCombSUM with the prepared DataChunk
    const std::vector<double> result = flockmtl::FusionCombSUM::Operation(chunk);

    // Verify the result
    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == 0.7);
}

TEST_CASE("Unit test for flockmtl::FusionCombSUM with 2 rows", "[fusion_combsum][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 5 (five rows)
    chunk.Initialize(allocator, types, 5);

    // Set the cardinality (number of rows) to 5
    chunk.SetCardinality(5);

    // Data we will use to populate the DataChunk
    constexpr std::array<double, 5> bm25_scores = {0.0, 0.4, 0.6, 0.8, 1.0};
    constexpr std::array<double, 5> vs_scores = {0.14, 0.41, 0.0, 1.0, 0.66};

    // Populate the DataChunk with test data
    for (size_t i = 0; i < bm25_scores.size(); ++i) {
        chunk.SetValue(0, i, bm25_scores[i]);
        chunk.SetValue(1, i, vs_scores[i]);
    }

    // Call FusionCombSUM with the prepared DataChunk
    const std::vector<double> result = flockmtl::FusionCombSUM::Operation(chunk);

    // Verify the result
    constexpr std::array<double, 5> expected_results = {0.14, (0.41 + 0.4), 0.6, (0.8 + 1.0), (1.0 + 0.66)};
    REQUIRE(result.size() == expected_results.size());
    for (size_t i = 0; i < expected_results.size(); ++i) {
        REQUIRE(result[i] == expected_results[i]);
    }
}

TEST_CASE("Unit test for flockmtl::FusionCombSUM with 3 rows", "[fusion_combsum][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 5 (five rows)
    chunk.Initialize(allocator, types, 5);

    // Set the cardinality (number of rows) to 5
    chunk.SetCardinality(5);

    // Data we will use to populate the DataChunk
    constexpr std::array<double, 5> bm25_scores = {0.0, 0.4, 0.6, 0.8, 1.0};
    constexpr std::array<double, 5> vs_scores = {0.14, 0.41, 0.0, 1.0, 0.66};
    constexpr std::array<double, 5> random_scores = {0.28, 0.5, 0.1, 0.8, 1.0};

    // Populate the DataChunk with test data
    for (size_t i = 0; i < bm25_scores.size(); ++i) {
        chunk.SetValue(0, i, bm25_scores[i]);
        chunk.SetValue(1, i, vs_scores[i]);
        chunk.SetValue(2, i, random_scores[i]);
    }

    // Call FusionCombSUM with the prepared DataChunk
    const std::vector<double> result = flockmtl::FusionCombSUM::Operation(chunk);

    // Verify the result
    constexpr std::array<double, 5> expected_results = {(0.14 + 0.28), (0.4 + 0.41 + 0.5), (0.6 + 0.1), (0.8 + 1.0 + 0.8), (1.0 + 0.66 + 1.0)};
    REQUIRE(result.size() == expected_results.size());
    for (size_t i = 0; i < expected_results.size(); ++i) {
        REQUIRE(result[i] == expected_results[i]);
    }
}

TEST_CASE("Unit test for flockmtl::FusionCombSUM with some NULL values", "[fusion_combsum][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 5 (five rows)
    chunk.Initialize(allocator, types, 5);

    // Set the cardinality (number of rows) to 5
    chunk.SetCardinality(5);

    // Data we will use to populate the DataChunk. -1 means null. NaN is the result of division by 0.
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr std::array<double, 5> bm25_scores = {-1, 0.4, -1, 0.8, 1.0};
    constexpr std::array<double, 5> vs_scores = {0.14, 0.41, 0.0, 1.0, nan};

    // Populate the DataChunk with test data
    for (size_t i = 0; i < bm25_scores.size(); ++i) {
        if (bm25_scores[i] == -1) {
            chunk.SetValue(0, i, duckdb::Value(duckdb::LogicalType::DOUBLE));
        } else {
            chunk.SetValue(0, i, bm25_scores[i]);
        }

        if (vs_scores[i] == -1) {
            chunk.SetValue(1, i, duckdb::Value(duckdb::LogicalType::DOUBLE));
        } else {
            chunk.SetValue(1, i, vs_scores[i]);
        }
    }

    // Call FusionCombSUM with the prepared DataChunk
    const std::vector<double> result = flockmtl::FusionCombSUM::Operation(chunk);

    // Verify the result
    constexpr std::array<double, 5> expected_results = {0.14, (0.41 + 0.4), 0.0, (0.8 + 1.0), 1.0};
    REQUIRE(result.size() == expected_results.size());
    for (size_t i = 0; i < expected_results.size(); ++i) {
        REQUIRE(result[i] == expected_results[i]);
    }
}

TEST_CASE("Unit test for flockmtl::FusionCombSUM with entire NULL column", "[fusion_combsum][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 5 (five rows)
    chunk.Initialize(allocator, types, 5);

    // Set the cardinality (number of rows) to 5
    chunk.SetCardinality(5);

    // Data we will use to populate the DataChunk
    constexpr std::array<double, 5> vs_scores = {0.046906565486452, 0.43271756518423, 0.75739696774139, 0.520761702528056, 0.58813126808257};

    // Populate the DataChunk with test data
    for (size_t i = 0; i < vs_scores.size(); ++i) {
        chunk.SetValue(0, i, std::numeric_limits<double>::quiet_NaN());   // Result of division by 0 in DuckDB
        chunk.SetValue(1, i, vs_scores[i]);
    }

    // Call FusionCombSUM with the prepared DataChunk
    const std::vector<double> result = flockmtl::FusionCombSUM::Operation(chunk);

    // Verify the result
    constexpr std::array<double, 5> expected_results = {0.046906565486452, 0.43271756518423, 0.75739696774139, 0.520761702528056, 0.58813126808257};
    REQUIRE(result.size() == expected_results.size());
    for (size_t i = 0; i < expected_results.size(); ++i) {
        REQUIRE(result[i] == expected_results[i]);
    }
}

TEST_CASE("Unit test for flockmtl::FusionCombSUM with only NULL values", "[fusion_combsum][flockmtl]") {
    // Define the column types (2 DOUBLE columns)
    const duckdb::vector<duckdb::LogicalType> types = {duckdb::LogicalType::DOUBLE, duckdb::LogicalType::DOUBLE};

    // Create a DataChunk and initialize it with the default allocator
    duckdb::DataChunk chunk;
    auto &allocator = duckdb::Allocator::DefaultAllocator();
    // Initialize with capacity 5 (five rows)
    chunk.Initialize(allocator, types, 5);

    // Set the cardinality (number of rows) to 5
    chunk.SetCardinality(5);

    // Populate the DataChunk with test data
    for (size_t i = 0; i < 5; ++i) {
        chunk.SetValue(0, i, std::numeric_limits<double>::quiet_NaN());   // Result of division by 0 in DuckDB
        chunk.SetValue(1, i, std::numeric_limits<double>::quiet_NaN());   // Result of division by 0 in DuckDB
    }

    // Call FusionCombSUM with the prepared DataChunk
    const std::vector<double> result = flockmtl::FusionCombSUM::Operation(chunk);

    for (size_t i = 0; i < 5; ++i) {
        REQUIRE(result[i] == 0.0);
    }
}