# Aggregate Functions

Aggregate functions in FlockMTL perform operations on groups of rows, returning a single result for each group. They're particularly useful for summarizing, ranking, and reordering data, often used with the `GROUP BY` clause in SQL queries. Leveraging language models, these functions enable advanced tasks like summarization, ranking, and relevance-based filtering, enhancing data analysis and NLP capabilities.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Available Aggregate Functions

FlockMTL offers several powerful aggregate functions:

1. [**`llm_reduce`**](/docs/aggregate-functions/llm-reduce): Aggregates a group of rows using a language model, typically for summarization or text consolidation.

   - **Example Use Cases**: Summarizing documents, aggregating product descriptions.

2. [**`llm_reduce_json`**](/docs/aggregate-functions/llm-reduce): Aggregates multiple rows into a single JSON output using a language model, ideal for tasks like summarization or consolidating text across multiple features.

   - **Example Use Cases**: Extracting key insights and sentiment from reviews, generating a summary with multiple attributes like themes and tone from survey responses.

3. [**`llm_rerank`**](/docs/aggregate-functions/llm-rerank): Reorders a list of rows based on relevance to a prompt using a sliding window mechanism.
   - **Example Use Cases**: Reranking search results, adjusting document or product rankings.

4. [**`llm_first`**](/docs/aggregate-functions/llm-first): Returns the most relevant item from a group based on a prompt.

   - **Example Use Cases**: Selecting the top-ranked document, finding the most relevant product.

5. [**`llm_last`**](/docs/aggregate-functions/llm-last): Returns the least relevant item from a group based on a prompt.

   - **Example Use Cases**: Finding the least relevant document, selecting the least important product.

## 2. How Aggregate Functions Work

Aggregate functions process groups of rows defined by a `GROUP BY` clause. They apply language models to the grouped data, generating a single result per group. This result can be a summary, a ranking, or another output defined by the prompt.

## 3. When to Use Aggregate Functions

- **Summarization**: Use `llm_reduce` to consolidate multiple rows.
- **Ranking**: Use `llm_first`, `llm_last`, or `llm_rerank` to reorder rows based on relevance.
- **Data Aggregation**: Use these functions to process and summarize grouped data, especially for text-based tasks.
