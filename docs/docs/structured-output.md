---
title: Structured Output
sidebar_position: 8
---title: Structured Output
sidebar_position: 6
---

# Structured Output in Flock

Flock enables users to obtain structured JSON responses from Large Language Models (LLMs) by defining JSON schemas
within SQL queries. This ensures LLM outputs conform to specific data structures, making them more reliable and easier
to process.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Overview

Structured output allows you to define the exact format and structure of the JSON response you expect from an LLM.
Instead of receiving free-form text, you can specify a JSON schema that the model must follow.

**Compatibility**: Works with all Flock LLM functions - `llm_complete`, `llm_filter`, `llm_reduce`, `llm_rerank`,
`llm_first`, `llm_last`

:::note Prerequisites
To extract values from structured JSON responses using dot notation (e.g., `response.category`), you need to load the
JSON extension:

```sql
LOAD JSON;
```

:::

## OpenAI Structured Output

OpenAI uses the `response_format` field with `type: "json_schema"` and `strict: true` to enforce schema compliance. For
more details check [OpenAI Structured Output](https://platform.openai.com/docs/guides/structured-outputs?api-mode=chat).

### Syntax

```sql
{
  'model_name': 'your-model-name',
  'model_parameters': '{
    "response_format": {
      "type": "json_schema",
      "json_schema": {
        "name": "response_schema",
        "schema": {
          "type": "object",
          "properties": {
            "field_name": { "type": "string" }
          },
          "required": ["field_name"],
          "additionalProperties": false
        }
      },
      "strict": true
    }
  }'
}
```

### Basic Example

```sql
SELECT llm_complete(
  {
    'model_name': 'gpt-4o',
    'model_parameters': '{
      "response_format": {
        "type": "json_schema",
        "json_schema": {
          "name": "product_categorization",
          "schema": {
            "type": "object",
            "properties": {
              "category": { "type": "string" },
              "confidence": { "type": "number" }
            },
            "required": ["category", "confidence"],
            "additionalProperties": false
          }
        },
        "strict": true
      }
    }'
  },
  { 'prompt': 'Categorize this product and provide a confidence score.' },
  { 'product_name': 'Wireless Bluetooth Headphones' }
) AS response;
```

### Advanced Example

```sql
WITH product_analysis AS (
  SELECT
    product_id,
    product_name,
    llm_complete(
      {
        'model_name': 'gpt-4o',
        'model_parameters': '{
          "response_format": {
            "type": "json_schema",
            "json_schema": {
              "name": "product_analysis",
              "schema": {
                "type": "object",
                "properties": {
                  "category": { "type": "string" },
                  "price_range": { "type": "string" },
                  "features": {
                    "type": "array",
                    "items": { "type": "string" }
                  },
                  "target_audience": { "type": "string" },
                  "rating_prediction": { "type": "number" }
                },
                "required": ["category", "price_range", "features", "target_audience", "rating_prediction"],
                "additionalProperties": false
              }
            },
            "strict": true
          }
        }'
      },
      { 'prompt_name': 'product-analysis', 'version': 1 },
      { 'product_name': product_name, 'product_description': product_description }
    ) AS analysis
  FROM products
)
SELECT 
  product_id,
  product_name,
  analysis.category::VARCHAR,
  analysis.price_range::VARCHAR,
  analysis.features,
  analysis.target_audience::VARCHAR,
  analysis.rating_prediction::DOUBLE
FROM product_analysis;
```

## Ollama Structured Output

Ollama uses the `format` field with an object schema definition to structure responses.

### Syntax

```sql
{
  'model_name': 'your-model-name',
  'model_parameters': '{
    "format": {
      "type": "object",
      "properties": {
        "field_name": { "type": "string" }
      },
      "required": ["field_name"],
      "additionalProperties": false
    }
  }'
}
```

### Basic Example

```sql
SELECT llm_complete(
  {
    'model_name': 'llama3.1',
    'model_parameters': '{
      "format": {
        "type": "object",
        "properties": {
          "summary": { "type": "string" },
          "sentiment": { "type": "string" }
        },
        "required": ["summary", "sentiment"],
        "additionalProperties": false
      }
    }'
  },
  { 'prompt': 'Summarize this review and determine its sentiment.' },
  { 'review_text': review_text }
) AS response
FROM customer_reviews;
```

### Advanced Example

```sql
WITH content_generation AS (
  SELECT
    topic_id,
    topic_name,
    llm_complete(
      {
        'model_name': 'ollama-writer',
        'model_parameters': '{
          "format": {
            "type": "object",
            "properties": {
              "title": { "type": "string" },
              "content": { "type": "string" },
              "tags": {
                "type": "array",
                "items": { "type": "string" }
              },
              "word_count": { "type": "number" },
              "difficulty_level": { "type": "string" }
            },
            "required": ["title", "content", "tags", "word_count", "difficulty_level"],
            "additionalProperties": false
          }
        }'
      },
      { 'prompt_name': 'content-generator', 'version': 2 },
      { 'topic': topic_name, 'target_audience': target_audience }
    ) AS generated_content
  FROM content_topics
)
SELECT 
  topic_id,
  topic_name,
  generated_content.title::VARCHAR,
  generated_content.content::VARCHAR,
  generated_content.tags,
  generated_content.word_count::INTEGER,
  generated_content.difficulty_level::VARCHAR
FROM content_generation;
```

## Aggregate Functions

Structured output works with aggregate functions like `llm_reduce`:

```sql
SELECT llm_reduce(
  {
    'model_name': 'gpt-4o',
    'model_parameters': '{
      "response_format": {
        "type": "json_schema",
        "json_schema": {
          "name": "review_aggregation",
          "schema": {
            "type": "object",
            "properties": {
              "overall_summary": { "type": "string" },
              "key_themes": {
                "type": "array",
                "items": { "type": "string" }
              },
              "sentiment_distribution": {
                "type": "object",
                "properties": {
                  "positive": { "type": "number" },
                  "negative": { "type": "number" },
                  "neutral": { "type": "number" }
                }
              }
            },
            "required": ["overall_summary", "key_themes", "sentiment_distribution"],
            "additionalProperties": false
          }
        },
        "strict": true
      }
    }'
  },
  { 'prompt_name': 'review-aggregation' },
  { 'review_text': review_text }
) AS aggregated_analysis
FROM customer_reviews
GROUP BY product_id;
```

## Extracting Values

Use dot notation to extract values from structured responses:

```sql
-- Basic extraction
SELECT response.category FROM table_name;

-- With type casting
SELECT response.category::VARCHAR FROM table_name;

-- Common type casts
SELECT 
  response.text_field::VARCHAR,
  response.number_field::DOUBLE,
  response.integer_field::INTEGER,
  response.boolean_field::BOOLEAN
FROM table_name;
```

## Common Schema Patterns

### Classification

```json
{
  "type": "object",
  "properties": {
    "category": { "type": "string" },
    "confidence": { "type": "number" },
    "reasoning": { "type": "string" }
  },
  "required": ["category", "confidence"],
  "additionalProperties": false
}
```

### Analysis

```json
{
  "type": "object",
  "properties": {
    "summary": { "type": "string" },
    "key_points": {
      "type": "array",
      "items": { "type": "string" }
    },
    "sentiment": { "type": "string" },
    "topics": {
      "type": "array",
      "items": { "type": "string" }
    }
  },
  "required": ["summary", "sentiment"],
  "additionalProperties": false
}
```

### Entity Extraction

```json
{
  "type": "object",
  "properties": {
    "entities": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "name": { "type": "string" },
          "type": { "type": "string" },
          "confidence": { "type": "number" }
        }
      }
    }
  },
  "required": ["entities"],
  "additionalProperties": false
}
```
