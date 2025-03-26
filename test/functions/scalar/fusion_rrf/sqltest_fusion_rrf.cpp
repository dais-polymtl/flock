#include "flockmtl-test/functions/scalar/fusion_rrf/test_fusion_rrf.hpp"

#include <iostream>

using namespace duckdb;

TEST_CASE("End-to-End SQL test for fusion_rrf with 2 DOUBLES", "[fusion_rrf]") {
    // Initialize an in-memory DuckDB instance
    auto db = make_uniq<DuckDB>(nullptr);
    auto con = make_uniq<Connection>(*db);

    // Run the function and check the output
    auto result = con->Query("SELECT fusion_rrf(1::DOUBLE, 2::DOUBLE)");

    // Ensure query executed successfully
    REQUIRE(!result->HasError());

    // Check expected value, -1 because there is only one row so no ranking is possible
    REQUIRE(result->GetValue<double>(0, 0) == -1);
}

TEST_CASE("Test fusion_rrf with multiple rows", "[fusion_rrf]") {
    // Initialize DuckDB in-memory instance
    auto db = make_uniq<DuckDB>(nullptr);
    auto con = make_uniq<Connection>(*db);

    // Create tables for BM25 and vector similarity scores
    unique_ptr<MaterializedQueryResult> result;
    result = con->Query("CREATE TABLE bm25_scores (index_column INTEGER, bm25_score DOUBLE)");
    REQUIRE(!result->HasError());
    result = con->Query("CREATE TABLE vs_scores (index_column INTEGER, vs_score DOUBLE)");
    REQUIRE(!result->HasError());

    // insert keyword search scores
    result = con->Query(R"(
        INSERT INTO bm25_scores VALUES
        (1, 10.0), (2, 20.0), (3, 30.0), (4, 40.0), (5, 50.0)
    )");
    REQUIRE(!result->HasError());

    // insert vector similarity scores
    result = con->Query(R"(
        INSERT INTO vs_scores VALUES
        (1, 30.0), (2, 10.0), (3, 2.0), (4, 200.0), (5, 15.0)
    )");
    REQUIRE(!result->HasError());

    result = con->Query(R"(
        SELECT
            COALESCE(b.index_column, e.index_column) AS index_column,
            fusion_rrf(
                b.bm25_score::DOUBLE,
                e.vs_score::DOUBLE
            ) AS ranking
        FROM bm25_scores b
        FULL OUTER JOIN vs_scores e
        ON b.index_column = e.index_column
        ORDER BY index_column;
    )");
    REQUIRE(!result->HasError());

    // check that values are as expected
    std::vector<int> expected_values = {3, 5, 4, 1, 2};
    for (int i = 0; i < 5; i++) {
        REQUIRE(result->GetValue(0, i) == i + 1);
        REQUIRE(result->GetValue(1, i) == expected_values[i]);
    }
}

TEST_CASE("Test fusion_rrf with NULL values", "[fusion_rrf]") {
    // Initialize an in-memory DuckDB instance
    auto db = make_uniq<DuckDB>(nullptr);
    auto con = make_uniq<Connection>(*db);

    // Create tables for BM25 and vector similarity scores
    unique_ptr<MaterializedQueryResult> result;
    result = con->Query("CREATE TABLE bm25_scores (index_column INTEGER, bm25_score DOUBLE)");
    REQUIRE(!result->HasError());
    result = con->Query("CREATE TABLE vs_scores (index_column INTEGER, vs_score DOUBLE)");
    REQUIRE(!result->HasError());

    // insert keyword search scores
    result = con->Query(R"(
        INSERT INTO bm25_scores VALUES
        (1, NULL), (2, NULL), (3, NULL), (4, NULL), (5, NULL)
    )");
    REQUIRE(!result->HasError());

    // insert vector similarity scores
    result = con->Query(R"(
        INSERT INTO vs_scores VALUES
        (1, 21.046906565486452), (2, 20.43271756518423), (3, 21.75739696774139), (4, 20.520761702528056), (5, 21.58813126808257)
    )");
    REQUIRE(!result->HasError());

    result = con->Query(R"(
        SELECT
            COALESCE(b.index_column, e.index_column) AS index_column,
            fusion_rrf(
                b.bm25_score::DOUBLE,
                e.vs_score::DOUBLE
            ) AS ranking
        FROM bm25_scores b
        FULL OUTER JOIN vs_scores e
        ON b.index_column = e.index_column
        ORDER BY index_column;
    )");
    REQUIRE(!result->HasError());

    // check that values are as expected
    std::vector<int> expected_values = {3, 5, 1, 4, 2};
    for (int i = 0; i < 5; i++) {
        REQUIRE(result->GetValue(0, i) == i + 1);
        REQUIRE(result->GetValue(1, i) == expected_values[i]);
    }
}