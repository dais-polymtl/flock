# Scalar Functions Overview

Scalar functions in FlockMTL operate on data row-by-row, providing powerful operations for text processing, embeddings, and machine learning tasks directly within SQL queries.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Available Functions

- [`llm_complete`](/docs/scalar-functions/llm-complete): Generates text completions based on model and prompt

- [`llm_complete_json`](/docs/scalar-functions/llm-complete-json): Generates text completions in JSON format for structured output

- [`llm_filter`](/docs/scalar-functions/llm-filter): Filters rows based on a prompt and returns boolean values

- [`llm_embedding`](/docs/scalar-functions/llm-embedding): Generates vector embeddings for text data, used for similarity search and machine learning tasks

## 2. Function Characteristics

- Applied row-by-row to table data
- Supports text generation, filtering, and embeddings

## 3. Use Cases

- Text generation
- Dynamic filtering
- Semantic text representation
- Machine learning feature creation
