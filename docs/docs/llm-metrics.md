---
title: LLM Metrics & Observability
sidebar_position: 8
---

# LLM Metrics & Observability

Flock includes built-in observability for all LLM functions. You can inspect token usage, API latency, and execution time
for `llm_complete`, `llm_filter`, `llm_embedding`, `llm_reduce`, `llm_rerank`, `llm_first`, and `llm_last` directly from
SQL.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Overview

Metrics are collected at the **database level** and aggregated across scalar and aggregate function calls. This allows
you to answer questions like:

- How many tokens did this query use?
- Which models and providers are being called most often?
- How much time is spent in the LLM API vs. local execution?

All metrics are exposed as JSON via dedicated helper functions.

## Core Functions

Flock registers three scalar functions for metrics:

- **`flock_get_metrics()`** – Returns a compact JSON summary of LLM usage.
- **`flock_get_debug_metrics()`** – Returns a more verbose JSON payload, useful for debugging.
- **`flock_reset_metrics()`** – Resets the in-memory metrics state and returns a confirmation message.

### Basic Usage

```sql
-- Run some LLM queries
SELECT llm_complete(
           {'model_name': 'gpt-4o'},
           {'prompt': 'Summarize this product.'},
           {'product': product_name}
       )
FROM products
LIMIT 10;

-- Inspect aggregated LLM metrics
SELECT flock_get_metrics() AS metrics;
```

Example JSON structure (simplified):

```json
{
  "invocations": [
    {
      "function": "llm_complete",
      "model_name": "gpt-4o",
      "provider": "openai",
      "input_tokens": 1234,
      "output_tokens": 456,
      "api_calls": 10,
      "api_duration_us": 1234567,
      "execution_time_us": 2345678
    }
  ]
}
```

### Resetting Metrics

Use `flock_reset_metrics()` to clear existing metrics before a new experiment or workload:

```sql
SELECT flock_reset_metrics() AS reset_result;
SELECT flock_get_metrics() AS metrics;
```

## Query-Level Workflows

Because metrics are stored at the database level, you can combine computation and inspection in the same script:

```sql
-- 1) Clear previous metrics
SELECT flock_reset_metrics();

-- 2) Run workload
WITH sample AS (
    SELECT *
    FROM (VALUES
        (1, 'Wireless Headphones'),
        (2, 'Gaming Laptop'),
        (3, 'Smart Watch')
    ) AS t(product_id, product_name)
)
SELECT
    product_id,
    llm_complete(
        {'model_name': 'gpt-4o'},
        {'prompt': 'Write a short marketing blurb for {name}.', 'context_columns': [{'data': product_name, 'name': 'name'}]}
    ) AS copy
FROM sample;

-- 3) Inspect metrics
SELECT flock_get_metrics() AS metrics;
```

You can further parse the JSON using DuckDB's `JSON` extension to build dashboards or reports.

## When to Use Metrics

LLM metrics are particularly useful when you:

- Benchmark different providers or models.
- Tune prompts and batch sizes for cost/performance trade-offs.
- Monitor token usage for budgeting and quota management.
- Diagnose slow or unexpectedly expensive queries.

By combining Flock’s LLM metrics with DuckDB’s analytics capabilities, you can build fully in-database observability
for your semantic workloads.

