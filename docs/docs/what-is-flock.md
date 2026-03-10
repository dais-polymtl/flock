---
sidebar_position: 1
---

# What is Flock?

## Overview

**Flock** enhances DuckDB by integrating semantic functions and robust resource management capabilities, enabling
advanced analytics and language model operations directly within SQL queries. It is distributed as a DuckDB extension
that runs on native platforms and in the browser via DuckDB-WASM.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Key Highlights (v0.4.0 and later)

- **Four LLM Providers**: OpenAI, Azure, Ollama, and Anthropic/Claude, all integrated through a unified SQL API.
- **Multimodal Support**: Text, image, and audio inputs (via transcription) using the same `context_columns` abstraction.
- **LLM Metrics Tracking**: Built-in functions such as `flock_get_metrics()` expose token counts, latency, and call-level
  metrics for all Flock LLM functions.
- **Browser & WASM Support**: Flock can be compiled as a DuckDB-WASM loadable extension to run directly in the browser.
- **Upgraded DuckDB Engine**: Based on DuckDB v1.4.4 for improved performance and stability.
- **Architecture Improvements**: Centralized bind data, RAII-based storage guards, and performance fixes across scalar and
  aggregate functions.
- **Developer Experience**: Interactive build scripts and Copilot agent instructions streamline local development and CI
  for the extension.

## Semantic Functions

Flock offers a suite of semantic functions that allow users to perform various language model operations:

- **Scalar Map Functions**:

    - [`llm_complete`](/docs/scalar-functions/llm-complete): Generates text completions using a specified language
      model.
    - [`llm_filter`](/docs/scalar-functions/llm-filter): Filters data based on language model evaluations, returning
      boolean values.
    - [`llm_embedding`](/docs/scalar-functions/llm-embedding): Generates embeddings for input text, useful for semantic
      similarity tasks.

- **Aggregate Reduce Functions**:
    - [`llm_reduce`](/docs/aggregate-functions/llm-reduce): Aggregates multiple inputs into a single output using a
      language model.
    - [`llm_rerank`](/docs/aggregate-functions/llm-rerank): Reorders query results based on relevance scores from a
      language model.
    - [`llm_first`](/docs/aggregate-functions/llm-first): Selects the top-ranked result after reranking.
    - [`llm_last`](/docs/aggregate-functions/llm-last): Selects the bottom-ranked result after reranking.

This allows users to perform tasks such as text generation, summarization, classification, filtering, fusion, and
embedding generation.

## Image Support

Flock provides comprehensive [**image support**](/docs/image-support) capabilities, allowing you to analyze, describe,
filter, and process images alongside traditional tabular data. This enables powerful multimodal AI applications directly
within your SQL workflows, including:

- Image content analysis and description generation
- Visual filtering and content moderation
- Image similarity search using embeddings
- Combining image and text data in unified queries

## Hybrid Search Functions

Flock also provides functions that support hybrid search. Namely, the following data fusion algorithms to combine scores
of various retrievers:

- [**`fusion_rrf`**](/docs/hybrid-search#fusion_rrf): Implements Reciprocal Rank Fusion (RRF) to combine rankings from
  multiple scoring systems.
- [**`fusion_combsum`**](/docs/hybrid-search#fusion_combsum): Sums normalized scores from different scoring systems.
- [**`fusion_combmnz`**](/docs/hybrid-search#fusion_combmnz): Sums normalized scores and multiplies by the hit count.
- [**`fusion_combmed`**](/docs/hybrid-search#fusion_combmed): Computes the median of normalized scores.
- [**`fusion_combanz`**](/docs/hybrid-search#fusion_combanz): Calculates the average of normalized scores.

These functions enable users to combine the strengths of different scoring methods, such as BM25 and embedding scores,
to produce the best-fit results, and even create end-to-end RAG pipelines.

## Structured Output

Flock provides [**structured output**](/docs/structured-output) capabilities that allow users to obtain predictable,
schema-compliant JSON responses from Large Language Models. This feature works with all Flock LLM functions and supports
OpenAI, Ollama, and Anthropic/Claude providers, ensuring consistent data formats for downstream processing.

## Resource Management

Flock introduces a [**resource management**](/docs/resource-management) framework that treats models (`MODEL`) and
prompts (`PROMPT`) similarly to tables, allowing for organized storage and retrieval.

## System Requirements

Flock is supported by the different operating systems and platforms, such as:

- Linux
- macOS
- Windows
- Modern browsers via DuckDB-WASM

And to ensure stable and reliable performance, it is important to meet only two requirements:

- **DuckDB Setup**: Version **1.4.4 or later**. Flock is compatible with the latest stable release of DuckDB, which can
  be installed from the official
  [DuckDB installation guide](https://duckdb.org/docs/installation/index?version=stable&environment=cli&platform=linux&download_method=direct&architecture=x86_64).
- **Provider API Key**: Flock supports multiple providers such as **OpenAI**, **Azure**, **Ollama**, and
  **Anthropic/Claude**. Configure the provider of your choice to get started.
