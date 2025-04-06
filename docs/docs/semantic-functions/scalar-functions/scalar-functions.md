# Scalar Functions Overview

Scalar functions in FlockMTL operate on data row-by-row, providing powerful operations for text processing, embeddings, and machine learning tasks directly within SQL queries.

# Table of Contents

Below is a quick overview to help you navigate with ease through the documentation. You can click on any of the links to jump to the relevant section.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Available Functions

- [`llm_complete`](/docs/semantic-functions/scalar-functions/llm-complete): Generates text completions based on model and prompt

- [`llm_complete_json`](/docs/semantic-functions/scalar-functions/llm-complete-json): Generates text completions in JSON format for structured output

- [`llm_filter`](/docs/semantic-functions/scalar-functions/llm-filter): Filters rows based on a prompt and returns boolean values

- [`llm_embedding`](/docs/semantic-functions/scalar-functions/llm-embedding): Generates vector embeddings for text data, used for similarity search and machine learning tasks

- [`fusion functions`](/docs/semantic-functions/scalar-functions/fusion-functions): Combines two numerical values into a single, unified relevance score.

## 2. Function Characteristics

- Applied row-by-row to table data
- Supports text generation, filtering, and embeddings

## 3. Use Cases

- Text generation
- Dynamic filtering
- Semantic text representation
- Machine learning feature creation
