---
title: Model Parameters
sidebar_position: 9
---

# Model Parameters in Flock

Flock allows you to configure model behavior through the `model_parameters` field in LLM function calls. This provides
fine-grained control over how models generate responses, enabling you to optimize performance for specific use cases.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Overview

Model parameters are passed as JSON strings within the `model_parameters` field of LLM function calls. Different
providers support different parameters, allowing you to customize temperature, token limits, sampling methods, and more.

Flock also supports **model execution settings** in model arguments:

- `batch_size`: maximum number of tuples per API call (applies to all map functions).
- `max_async_calls`: maximum number of parallel in-flight provider calls (default `20`).
- `max_async_calls` works with `batch_size`: each API call still respects batch size, while in-flight calls are capped by
  `max_async_calls`.

The async mode is especially useful for experimenting with batch behavior and is independent from
`batch_size`. You can tune both in one model argument:

## Model Execution Settings

`max_async_calls` can be set at model creation time or directly inside the `model_name` argument when calling an LLM
function.

```sql
SELECT llm_complete(
  {
    'model_name': 'gpt-4o',
    'batch_size': 16,
    'max_async_calls': 40,
    'model_parameters': '{"temperature": 0.7, "max_tokens": 1000}'
  },
  {'prompt': 'Write a concise executive summary.'},
  {'text': doc_text}
);
```

If you do not set `max_async_calls`, Flock uses the model-level value, which defaults to `20`.

**Compatibility**: Works with all Flock LLM functions - `llm_complete`, `llm_filter`, `llm_embedding`, `llm_reduce`,
`llm_rerank`, `llm_first`, `llm_last`

### Async Mode with Aggregate Functions

Aggregate functions (`llm_reduce`, `llm_rerank`, `llm_first`, `llm_last`) use the same concurrency controls. Async mode controls
how many aggregate worker calls execute at once while preserving batch-size safety per request.

## Demo Provider

Flock ships with a built-in demo provider (`provider='demo'`) so you can run end-to-end tests without external
API credentials. The demo provider returns deterministic synthetic outputs and honors both `batch_size` and
`max_async_calls`.

Try it directly:

```sql
SELECT llm_complete(
  {
    'model_name': 'flock_demo',
    'batch_size': 4,
    'max_async_calls': 2
  },
  {'prompt': 'Classify this text', 'context_columns': [{'name': 'text', 'data': ['A', 'B', 'C']}]}
);
```

### Quick Demo Provider Query

You can initialize once and run a query directly with per-query overrides:

```sql
SELECT llm_filter(
  {
    'model_name': 'flock_demo',
    'batch_size': 4,
    'max_async_calls': 8
  },
  {'prompt': 'Keep only rows mentioning positive sentiment.', 'text': message}
);
```

## OpenAI Parameters

OpenAI models support a comprehensive set of parameters for controlling generation behavior. For complete parameter
reference, see [OpenAI Chat Completions API](https://platform.openai.com/docs/api-reference/chat/create).

### Syntax

```sql
{
  'model_name': 'gpt-4o',
  'model_parameters': '{
    "temperature": 0.7,
    "max_tokens": 1000,
    "top_p": 1.0,
    "frequency_penalty": 0.0,
    "presence_penalty": 0.0,
    "stop": ["\\n", "END"],
    "response_format": {...}
  }'
}
```

### Example Usage

```sql
SELECT llm_complete(
  {
    'model_name': 'gpt-4o',
    'model_parameters': '{
        "temperature": 0.7,
        "max_tokens": 1000,
        "top_p": 1.0,
        "frequency_penalty": 0.0,
        "presence_penalty": 0.0,
        "stop": ["\\n", "END"],
        "response_format": {...}
    }'
  },
  { 'prompt': 'Write a professional email subject line.' },
  { 'topic': 'quarterly report' }
) AS response;
```

## Ollama Parameters

Ollama models support different parameters optimized for local deployment. For complete parameter reference,
see [Ollama Chat Completions API](https://github.com/ollama/ollama/blob/main/docs/api.md#generate-a-chat-completion).

### Syntax

```sql
{
  'model_name': 'llama3.1',
  'model_parameters': '{
    "temperature": 0.7,
    "num_predict": 1000,
    "top_p": 0.9,
    "top_k": 40,
    "repeat_penalty": 1.1,
    "format": {...}
  }'
}
```

### Example Usage

```sql
SELECT llm_complete(
  {
    'model_name': 'llama3.1',
    'model_parameters': '{
      "temperature": 0.3,
      "num_predict": 300
    }'
  },
  { 'prompt': 'Provide a factual explanation.' },
  { 'topic': 'photosynthesis' }
) AS explanation;
```

## Azure OpenAI Parameters

Azure OpenAI supports the same parameters as OpenAI. For complete parameter reference,
see [OpenAI Chat Completions API](https://platform.openai.com/docs/api-reference/chat/create).

### Syntax

```sql
{
  'model_name': 'gpt-4o',
  'model_parameters': '{
    "temperature": 0.7,
    "max_tokens": 1000,
    "top_p": 1.0,
    "frequency_penalty": 0.0,
    "presence_penalty": 0.0,
    "stop": null
  }'
}
```

## Common Parameter Patterns

### Deterministic Output

```sql
SELECT llm_complete(
  {
    'model_name': 'gpt-4o',
    'model_parameters': '{
      "temperature": 0.0,
      "max_tokens": 500
    }'
  },
  { 'prompt': 'Explain the concept accurately.' },
  { 'concept': 'quantum computing' }
) AS factual_explanation;
```

### Creative Generation

```sql
SELECT llm_complete(
  {
    'model_name': 'gpt-4o',
    'model_parameters': '{
      "temperature": 0.9,
      "top_p": 0.95,
      "presence_penalty": 0.5,
      "max_tokens": 800
    }'
  },
  { 'prompt': 'Write a creative story.' },
  { 'genre': 'science fiction', 'setting': 'mars colony' }
) AS creative_story;
```

### Code Generation

````sql
SELECT llm_complete(
  {
    'model_name': 'gpt-4o',
    'model_parameters': '{
      "temperature": 0.0,
      "max_tokens": 300,
      "stop": ["\\n\\n", "```"]
    }'
  },
  { 'prompt': 'Generate clean, working code.' },
  { 'language': 'python', 'task': 'sort a list' }
) AS code_snippet;
````
