# Scalar Functions Overview

Scalar functions in FlockMTL operate on data row-by-row, providing powerful operations for text processing, embeddings,
image analysis, and machine learning tasks directly within SQL queries.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Available Functions

- [`llm_complete`](/docs/scalar-functions/llm-complete): Generates text completions based on model and prompt, supports both text and image inputs

- [`llm_filter`](/docs/scalar-functions/llm-filter): Filters rows based on a prompt and returns boolean values, supports both text and image inputs

- [`llm_embedding`](/docs/scalar-functions/llm-embedding): Generates vector embeddings for text data, used for
  similarity search and machine learning tasks (text only)

## 2. Function Characteristics

- Applied row-by-row to table data
- Supports text generation, filtering, and embeddings
- **Multimodal support**: Process both text and image data using the same functions
- **Context columns**: New API design using `context_columns` arrays for flexible data input

## 3. Context Columns API

All scalar functions now use a unified `context_columns` API structure:

```sql
'context_columns': [
  {'data': column_name},                    -- Basic text column
  {'data': column_name, 'name': 'alias'},   -- Text column with custom name
  {'data': image_url, 'type': 'image'}      -- Image column
]
```

Each context column supports three properties:

- **`data`** _(required)_: The SQL column data
- **`name`** _(optional)_: Custom name for referencing in prompts
- **`type`** _(optional)_: Data type - `"tabular"` (default) or `"image"`

## 4. Use Cases

- Text generation and content creation
- Dynamic filtering with visual and textual criteria
- Semantic text and image representation
- Machine learning feature creation
- Multimodal analysis combining text and visual data
- Content moderation for both text and images
