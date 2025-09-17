---
title: Model Parameters
sidebar_position: 6
---

# Model Parameters in Flock

Flock allows you to configure model behavior through the `model_parameters` field in LLM function calls. This provides
fine-grained control over how models generate responses, enabling you to optimize performance for specific use cases.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Overview

Model parameters are passed as JSON strings within the `model_parameters` field of LLM function calls. Different
providers support different parameters, allowing you to customize temperature, token limits, sampling methods, and more.

**Compatibility**: Works with all Flock LLM functions - `llm_complete`, `llm_filter`, `llm_embedding`, `llm_reduce`,
`llm_rerank`, `llm_first`, `llm_last`

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
