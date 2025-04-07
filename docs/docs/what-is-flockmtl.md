---
sidebar_position: 1
---

# What is FlockMTL?

## Overview

**FlockMTL** enhances DuckDB by integrating semantic functions and robust resource management capabilities, enabling advanced analytics and language model operations directly within SQL queries.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Semantic Functions

FlockMTL offers a suite of semantic functions that allow users to perform various language model operations:

- **Scalar Map Functions**:
    - [`llm_complete`](/docs/scalar-functions/llm-complete): Generates text completions using a specified language model.
    - [`llm_complete_json`](/docs/scalar-functions/llm-complete-json): Produces JSON-formatted text completions.
    - [`llm_filter`](/docs/scalar-functions/llm-filter): Filters data based on language model evaluations, returning boolean values.
    - [`llm_embedding`](/docs/scalar-functions/llm-embedding): Generates embeddings for input text, useful for semantic similarity tasks.

- **Aggregate Reduce Functions**:
    - [`llm_reduce`](/docs/aggregate-functions/llm-reduce): Aggregates multiple inputs into a single output using a language model.
    - [`llm_reduce_json`](/docs/aggregate-functions/llm-reduce-json): Similar to `llm_reduce`, but outputs JSON-formatted results.
    - [`llm_rerank`](/docs/aggregate-functions/llm-rerank): Reorders query results based on relevance scores from a language model.
    - [`llm_first`](/docs/aggregate-functions/llm-first): Selects the top-ranked result after reranking.
    - [`llm_last`](/docs/aggregate-functions/llm-last): Selects the bottom-ranked result after reranking.


This allows users to perform tasks such as text generation, summarization, classification, filtering, fusion, and embedding generation.

## Hybrid Search Functions

FlockMTL also provides functions that support hybrid search. Namely, the following data fusion algorithms to combine scores of various retrievers:

- [**`fusion_rrf`**](/docs/hybrid-search#fusion_rrf): Implements Reciprocal Rank Fusion (RRF) to combine rankings from multiple scoring systems.
- [**`fusion_combsum`**](/docs/hybrid-search#fusion_combsum): Sums normalized scores from different scoring systems.
- [**`fusion_combmnz`**](/docs/hybrid-search#fusion_combmnz): Sums normalized scores and multiplies by the hit count.
- [**`fusion_combmed`**](/docs/hybrid-search#fusion_combmed): Computes the median of normalized scores.
- [**`fusion_combanz`**](/docs/hybrid-search#fusion_combanz): Calculates the average of normalized scores.

These functions enable users to combine the strengths of different scoring methods, such as BM25 and embedding scores, to produce the best-fit results, and even create end-to-end RAG pipelines.

We believe that relational DBMSs and LLMs are a match made in heaven. We are leaning on the tradition of declarative interfaces to unburden users from lower-level implementation details. Users can query both structured and unstructred datasets while combining analytics and semantic analysis directly within SQL.

## Resource Management

FlockMTL introduces a [**resource management**](/docs/resource-management) framework that treats models (`MODEL`) and prompts (`PROMPT`) similarly to tables, allowing for organized storage and retrieval.

## System Requirements

FlockMTL is supported by the different operating systems and platforms, such as:
- Linux
- macOS
- Windows

And to ensure stable and reliable performance, it is important to meet only two requirements:
- **DuckDB Setup**: Version 1.1.1 or later. FlockMTL is compatible with the latest stable release of DuckDB, which can be installed from the official [DuckDB installation guide](https://duckdb.org/docs/installation/index?version=stable&environment=cli&platform=linux&download_method=direct&architecture=x86_64).
- **Provider API Key**: FlockMTL supports multiple providers such as **OpenAI**, **Azure**, and **Ollama**. Configure the provider of your choice to get started.