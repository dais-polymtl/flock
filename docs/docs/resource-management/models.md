---
title: Models
sidebar_position: 1
---

# Models Management

The **Models Management** section provides guidance on how to manage and configure models for **analytics and semantic
analysis tasks** within FlockMTL. These tasks involve processing and analyzing text, embeddings, and other data types
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
| **Model Arguments** | JSON configuration parameters. For user-defined models: only `tuple_format`, `batch_size`, and `model_parameters` (which itself is a JSON object for all model params) are allowed. **tuple_format** can be one of: `JSON`, `XML`, or `Markdown`. |

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
-- User-defined model (only tuple_format, batch_size, and model_parameters allowed in JSON)
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
        }
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
        }
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
    {'model_name': 'gpt-4'},
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
