# Aggregate Functions

Aggregate functions in FlockMTL perform operations on groups of rows, returning a single result for each group. They're particularly useful for summarizing, ranking, and reordering data, often used with the `GROUP BY` clause in SQL queries. Leveraging language models, these functions enable advanced tasks like summarization, ranking, and relevance-based filtering, enhancing data analysis and NLP capabilities. **All aggregate functions support both text and image data** through the unified context columns API.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Available Aggregate Functions

FlockMTL offers several powerful aggregate functions:

1. [**`llm_reduce`**](/docs/aggregate-functions/llm-reduce): Aggregates a group of rows using a language model, typically for summarization or text consolidation.

   - **Example Use Cases**: Summarizing documents, aggregating product descriptions, analyzing collections of images.

2. [**`llm_rerank`**](/docs/aggregate-functions/llm-rerank): Reorders a list of rows based on relevance to a prompt using a sliding window mechanism.

   - **Example Use Cases**: Reranking search results, adjusting document or product rankings, prioritizing images by visual appeal.

3. [**`llm_first`**](/docs/aggregate-functions/llm-first): Returns the most relevant item from a group based on a prompt.

   - **Example Use Cases**: Selecting the top-ranked document, finding the most relevant product, choosing the best image from a collection.

4. [**`llm_last`**](/docs/aggregate-functions/llm-last): Returns the least relevant item from a group based on a prompt.

   - **Example Use Cases**: Finding the least relevant document, selecting the least important product, identifying low-quality images.

## 2. Context Columns API

All aggregate functions use the unified `context_columns` API structure:

```sql
'context_columns': [
  {'data': column_name},                    -- Basic text column
  {'data': column_name, 'name': 'alias'},   -- Text column with custom name
  {'data': image_url, 'type': 'image'}      -- Image column
]
```

This enables multimodal processing where you can combine text and image data in a single aggregate operation.

## 3. How Aggregate Functions Work

Aggregate functions process groups of rows defined by a `GROUP BY` clause. They apply language models to the grouped data, generating a single result per group. This result can be a summary, a ranking, or another output defined by the prompt. The functions can process:

- **Text data**: Traditional textual content
- **Image data**: Visual content from URLs or file paths
- **Mixed data**: Combinations of text and images in the same operation

## 4. When to Use Aggregate Functions

- **Summarization**: Use `llm_reduce` to consolidate multiple rows, including visual summaries of image collections
- **Ranking**: Use `llm_first`, `llm_last`, or `llm_rerank` to reorder rows based on relevance, including visual criteria
- **Data Aggregation**: Use these functions to process and summarize grouped data for both text and visual content
- **Multimodal Analysis**: Combine text and image data to make more informed decisions
