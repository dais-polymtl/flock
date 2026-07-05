---
title: Models
sidebar_position: 1
---

# Models Management

The **Models Management** section provides guidance on how to manage and configure models for **analytics and semantic
analysis tasks** within Flock. These tasks involve processing and analyzing text, embeddings, and other data types
using pre-configured models, either system-defined or user-defined, based on specific use cases. Each database is
configured with its own model management table during the initial load.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Model Configuration

Models are stored in a table with the following structure:

| **Column Name**     | **Description**                                                                                                                                                                                                                                   |
|---------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Model Name**      | Unique identifier for the model                                                                                                                                                                                                                   |
| **Model Type**      | Specific model type (e.g., `gpt-4`, `llama3`)                                                                                                                                                                                                     |
| **Provider**        | Source of the model (e.g., `openai`, `azure`, `ollama`)                                                                                                                                                                                           |
| **Model Arguments** | JSON configuration parameters. For user-defined models: only `tuple_format`, `batch_size`, `model_parameters`, `is_async`, `rate_limit`, and `usage_limit` are allowed. **tuple_format** can be one of: `JSON`, `XML`, or `Markdown`. **batch_size** must be greater than 0. **model_parameters** is a JSON object of provider-specific settings. **is_async** is a boolean (default `true`) that controls whether scalar functions batch completion requests in parallel before collecting responses. **rate_limit** is an optional positive integer for maximum provider requests per minute, scoped per Flock `model_name`. **usage_limit** is an optional JSON object for cumulative token quotas, also scoped per Flock `model_name`. |

### `is_async`

`is_async` applies to `llm_complete` and `llm_filter`. When `true` (the default), all batches are queued in parallel and responses are collected in a single call. When `false`, batches are processed one at a time synchronously.

```sql
-- Default: async batching
CREATE MODEL('async-model', 'gpt-4o', 'openai', {"batch_size": 16});

-- Explicit synchronous batching
CREATE MODEL('sync-model', 'gpt-4o', 'openai', {"batch_size": 16, "is_async": false});
```

You can also pass `is_async` inline when calling a function:

```sql
SELECT llm_complete(
    {'model_name': 'gpt-4o', 'is_async': false},
    {'prompt': 'Summarize', 'context_columns': [{'data': text}]}
) FROM my_table;
```

### `rate_limit`

`rate_limit` sets the maximum number of provider requests per minute for a Flock model, keyed by `model_name`. This helps avoid exceeding provider quotas on free or shared API tiers.

When `rate_limit` is set:

- Flock spaces outgoing provider requests so the configured requests-per-minute cap is not exceeded.
- `batch_size` and `rate_limit` are independent: `batch_size` controls tuples per request; `rate_limit` throttles how many provider requests are sent per minute.
- The limit applies across completions, embeddings, and transcriptions for that model.
- Sync and async scalar execution both respect the same per-model limit.

```sql
-- Limit to 30 requests per minute
CREATE MODEL('limited-gpt4o', 'gpt-4o', 'openai', {"batch_size": 64, "rate_limit": 30});

-- Inline override in a function call
SELECT llm_complete(
    {'model_name': 'gpt-4o', 'rate_limit': 30},
    {'prompt': 'Summarize', 'context_columns': [{'data': text}]}
) FROM my_table;
```

If `rate_limit` is omitted, Flock does not apply request-per-minute throttling.

### `usage_limit`

`usage_limit` sets cumulative token quotas for a Flock model, keyed by `model_name`. Unlike `rate_limit`, which throttles request rate, `usage_limit` tracks provider-reported token usage across calls and fails once a quota is exceeded.

Supported fields (at least one is required):

- `prompt_tokens_limit`: maximum cumulative input/prompt tokens
- `completion_tokens_limit`: maximum cumulative output/completion tokens
- `total_tokens_limit`: maximum cumulative total tokens (prompt + completion)

When a limit is exceeded, Flock raises a usage-limit error. This is separate from context-window overflow errors that trigger batch-size retries.

```sql
-- Cap total tokens for a model
CREATE MODEL('budget-gpt4o', 'gpt-4o', 'openai', {
    "batch_size": 16,
    "usage_limit": {"total_tokens_limit": 50000}
});

-- Inline override in a function call
SELECT llm_complete(
    {
        'model_name': 'gpt-4o',
        'usage_limit': {'prompt_tokens_limit': 10000, 'completion_tokens_limit': 5000}
    },
    {'prompt': 'Summarize', 'context_columns': [{'data': text}]}
) FROM my_table;
```

If `usage_limit` is omitted, Flock does not enforce cumulative token quotas.

## 2. Management Commands

- Retrieve all available models

```sql
GET
MODELS;
```

- Retrieve details of a specific model

```sql
GET
MODEL 'model_name';
```

- Create a new user-defined model

```sql
-- User-defined model (only tuple_format, batch_size, model_parameters, is_async, rate_limit, and usage_limit allowed in JSON)
-- tuple_format can be "JSON", "XML", or "Markdown"
CREATE
MODEL(
    'model_name',
    'model',
    'provider',
    {
        "tuple_format": "JSON",
        "batch_size": 8,
        "model_parameters": {
            "temperature": 0.2,
            "top_p": 0.95
        },
        "is_async": true
    }
);
CREATE
MODEL(
    'model_name',
    'model',
    'provider',
    {
        "tuple_format": "XML",
        "batch_size": 8,
        "model_parameters": {
            "n": 3,
            "frequency_penalty": 0.1
        }
    }
);
CREATE
MODEL(
    'model_name',
    'model',
    'provider',
    {
        "tuple_format": "Markdown",
        "batch_size": 8
    }
);
```

- Modify an existing user-defined model

```sql
-- Update user-defined model (same JSON rules as CREATE)
-- tuple_format can be "JSON", "XML", or "Markdown"
UPDATE MODEL(
    'model_name',
    'model',
    'provider'
);
UPDATE MODEL(
    'model_name',
    'model',
    'provider',
    {
        "tuple_format": "JSON",
        "batch_size": 8,
        "model_parameters": {
            "temperature": 0.2,
            "top_p": 0.95
        },
        "is_async": true
    }
);
UPDATE MODEL(
    'model_name',
    'model',
    'provider',
    {
        "tuple_format": "XML",
        "batch_size": 8,
        "model_parameters": {
            "n": 3,
            "frequency_penalty": 0.1
        }
    }
);
UPDATE MODEL(
    'model_name',
    'model',
    'provider',
    {
    "tuple_format": "Markdown",
    "batch_size": 8
    }
);
```

- Remove a user-defined model

```sql
DELETE
MODEL 'model_name';
```

## 3. Global and Local Models

Model creation is database specific. If you want it to be available irrespective of the database, use the GLOBAL
keyword. LOCAL is the default if not specified.

### Create Models

- Create a global model:

```sql
CREATE GLOBAL MODEL
('model_name', 'model_type', 'provider'
);
CREATE GLOBAL MODEL
(
    'model_name', 'model_type', 'provider', {
    "tuple_format"
    :
    "JSON",
    "batch_size": 8,
    "model_parameters": {
            "temperature": 0.2,
    "top_p": 0.95
        }
    }
);
CREATE GLOBAL MODEL
(
    'model_name', 'model_type', 'provider',
    {
        "tuple_format": "XML",
        "batch_size": 8
    }
);
CREATE GLOBAL MODEL
(
    'model_name', 'model_type', 'provider',
    {
        "tuple_format": "Markdown"
    }
);
```

- Create a local model (default if no type is specified):

```sql
CREATE LOCAL MODEL('model_name', 'model_type', 'provider');
CREATE LOCAL MODEL(
    'model_name', 'model_type', 'provider',
    {
        "tuple_format": "JSON",
        "batch_size": 8,
        "model_parameters": {
            "temperature": 0.2
        }
    }
);
CREATE LOCAL MODEL
(
    'model_name', 'model_type', 'provider',
    {
        "tuple_format": "XML",
        "batch_size": 8,
        "model_parameters": {
            "n": 3
        }
    }
);
CREATE LOCAL MODEL
(
    'model_name', 'model_type', 'provider',
    {
        "tuple_format": "Markdown",
        "batch_size": 8,
        "model_parameters": {
            "top_p": 0.95
        }
    }
);
CREATE
MODEL(
    'model_name',
    'model_type',
    'provider'
);
CREATE
MODEL(
    'model_name',
    'model_type',
    'provider',
    {
        "tuple_format": "JSON",
        "batch_size": 8,
        "model_parameters": {
            "temperature": 0.2
        }
    }
);
CREATE
MODEL(
    'model_name',
    'model_type',
    'provider',
    {
        "tuple_format": "XML",
        "batch_size": 8,
        "model_parameters": {
            "n": 3
        }
    }
);
CREATE
MODEL(
    'model_name',
    'model_type',
    'provider',
    {
        "tuple_format": "Markdown",
        "batch_size": 8,
        "model_parameters": {
            "top_p": 0.95
        }
    }
);
```

- Toggle a model's state between global and local:

```sql
UPDATE MODEL 'model_name' TO GLOBAL;
UPDATE MODEL 'model_name' TO LOCAL;
```

All other queries remain the same for both global and local models.

## 4. SQL Query Examples

### Semantic Text Completion

```sql
SELECT llm_complete(
    {'model_name': 'gpt-4o'},
    {'prompt_name': 'product-description'},
    {'input_text': product_description}
) AS generated_description
FROM products;
```

### Semantic Search

```sql
SELECT llm_complete(
    {'model_name': 'semantic_search_model'},
    {'prompt_name': 'search-query'},
    {'search_query': query}
) AS search_results
FROM search_data;
```
