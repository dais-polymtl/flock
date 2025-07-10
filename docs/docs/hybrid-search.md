---
title: Hybrid Search
sidebar_position: 9
---title: Hybrid Search
sidebar_position: 7
---

# Hybrid Search using FlockMTL
## Overview
FlockMTL provides a set of fusion functions that allow you to combine multiple scoring systems into a single, unified relevance score. This is particularly useful in hybrid search scenarios where you want to leverage the strengths of different scoring methods (e.g., BM25 and vector similarity scores) to produce the best-fit result. To achieve that, FlockMTL offers two types of fusion functions: rank-based and score-based.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Rank-Based Fusion Algorithm
The input to the rank-based fusion algorithm is each document's ranking in n scoring systems, from best to worst, where 1 is the best-ranked document. Multiple documents can have the same rank and will be treated as equal.
### `fusion_rrf`
Performs Reciprocal Rank Fusion (RRF). Each document's combined score is calculated using the following formula:
$$
combined\_score = \sum_{n=1}^{N} \frac{1}{60 + \text{ranking}_n}
$$
where `N` represents the number of scoring systems and `ranking_n` represents the rank of the document in that scoring system.

Performs Reciprocal Rank Fusion (RRF) as described by [Cormack et al](https://doi.org/10.1145/1571941.1572114).

* Gordon V. Cormack, Charles L A Clarke, and Stefan Buettcher. 2009. Reciprocal rank fusion outperforms condorcet and individual rank learning methods. In Proceedings of the 32nd international ACM SIGIR conference on Research and development in information retrieval (SIGIR '09). Association for Computing Machinery, New York, NY, USA, 758–759. https://doi.org/10.1145/1571941.1572114

## Score-Based Fusion Algorithms
The input to the score-based fusion algorithms is n normalized sets of scores. The scores must be normalized because different scoring systems often use different scales for their scores. To ensure that all scoring systems are treated equally, they must first be normalized.
### `fusion_combsum`
Sums over all normalized scores for each document. Please note that NULL, NaN, and 0 values are all treated as 0.

### `fusion_combmnz`
Sums over all normalized scores for each document and multiplies that sum by the number of scoring systems which found the document (hit count). A hit constitutes any score greater than 0. Please note that NULL, NaN, and 0 values are all treated as 0. Each document's combined score is calculated using the following formula:
$$
combined\_score = hit\_count * \sum_{n=1}^{N} normalized\_score_n
$$
where $N$ represents the number of scoring systems, $normalized\_score_n$ represents the normalized score of the document in that scoring system, and $hit\_count$ represents the number of non-zero scores that the document has.

### `fusion_combmed`
Takes the median of all scores for each document. Please note that NULL, NaN, and 0 values are all treated as 0 and are considered when calculating the median. If the inputs are `(NULL, NULL, 1.0)`, then the median returned is 0.

### `fusion_combanz`
Calculates the average normalized score for each document. Please note that NULL, NaN, and 0 values are all treated as 0 and are considered when calculating the average. If the inputs are `(NULL, NULL, 1.0)`, then the average returned is $0.\overline{3}$.

A variety of algorithms as described by [fox et al](https://trec.nist.gov/pubs/trec2/papers/txt/23.txt).

## Data pre-processing
### Rank-Based Algorithm
The input to the rank-based algorithm is 1 or more sets of rankings for each document. This ranking can be obtained by using the DENSE_RANK() function, as shown in the following sample query:
```sql
SELECT 
    index_column,
    bm25_score,
    DENSE_RANK() OVER (ORDER BY bm25_score DESC) AS bm25_rank
FROM bm25_scores;
```

### Score-Based Algorithm
The input to score-based algorithms are 1 or more sets of normalized scores for each document. These normalized scores can be obtained directly in SQL. Note that division by 0 (eg. when min = max) results in NaN values. As mentioned above, these are treated by the fusion algorithms as 0. The following sample query performs min-max normalization:
```sql
WITH min_max AS (
    SELECT
        MIN(bm25_score) AS min,
        MAX(bm25_score) AS max
    FROM bm25_scores
)
SELECT 
    index_column,
    bm25_score,
    (bm25_score - min) / (max - min) AS normalized_bm25
FROM bm25_scores, min_max;
```

## 1. Basic Usage Examples
All score-based fusion algorithms use the same syntax. In the following examples, fusion_combsum can be replaced by any of the score-based fusion algorithms presented above.

### 1.1 Simple Example: Combining Two Scores
### Rank-Based Algorithm

```sql
SELECT fusion_rrf(1, 1);
```

**Description**: In this simple example, the `fusion_rrf` function takes two ranks (1 and 1) as inputs and returns the combined score.

**Output**:

```sql
0.03278688524590164
```

### Score-Based Algorithms
```sql
SELECT fusion_combsum(0.4, 0.5);
```

**Output**:

```sql
0.9
```

### 1.2 Example with BM25 and Embedding Scores
### Rank-Based Algorithm

```sql
SELECT
    fusion_rrf(bm25_rank, embedding_rank) AS combined_score
FROM search_results;
```

**Description**: This query combines the BM25 rank (`bm25_rank`) and the embedding rank (`embedding_rank`) for each row in the `search_results` table. The function merges both scores into a single `combined_score`.

### Score-Based Algorithms
```sql
SELECT
    fusion_combsum(bm25_normalized, embedding_normalized) AS combined_score
FROM search_results;
```

**Description**: This query combines the normalized BM25 score (`bm25_normalized`) and the normalized embedding score (`embedding_normalized`) for each row in the `search_results` table. The function merges both scores into a single `combined_score`.

## 2. Advanced Example

```sql
WITH combined_scores AS (
    SELECT
        index_column,
        fusion_combsum(bm25_normalized, embedding_normalized) AS combined_score
    FROM search_results
)
SELECT
    index_column,
    combined_score
FROM combined_scores
ORDER BY combined_score DESC;
```

**Description**: In this example, the query merges BM25 and embedding scores into a `combined_score` for each document and sorts the results by `combined_score` in descending order, prioritizing more relevant documents. The same can be done with `fusion_rrf`

## 3. Input Parameters

The fusion functions (score-based and rank-based) accept **two or more numerical inputs**. While examples below show two inputs for simplicity, the functions are designed to handle any number of inputs.

### 3.1 Numerical Inputs

- **Parameter**: One or more numeric values (e.g., normalized scores from different systems)
- **Description**: Each input represents a score from a distinct retrieval method (e.g., BM25, vector similarity search, etc.). These values are combined by the fusion algorithm.
- **Example**: `0.4, 0.9, 0.7`

> 💡 You can pass any number of numerical inputs—two or more—depending on how many scoring systems you’re combining.
