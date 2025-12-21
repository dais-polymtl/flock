---
title: Anthropic
sidebar_position: 4
---

# Flock Using Anthropic

In this section we will cover how to set up and use the Anthropic provider with Flock to access Claude models.

Before starting, you should have already followed the [Getting Started](/docs/getting-started) guide.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Anthropic Provider Setup

To use the Anthropic provider, you need to get your API key from the [Anthropic Console](https://console.anthropic.com/). Only two steps are required:

- First, create a secret with your Anthropic API key:

```sql
CREATE SECRET (
    TYPE ANTHROPIC,
    API_KEY 'your-api-key'
);
```

- Next, create your Claude model in the model manager. Make sure that the name of the model is unique:

```sql
CREATE MODEL(
   'ClaudeModel',
   'claude-3-haiku-20240307',
   'anthropic',
   {"tuple_format": "json", "batch_size": 32, "model_parameters": {"temperature": 0.7, "max_tokens": 1024}}
);
```

- Now you can use Flock with the Anthropic provider:

```sql
SELECT llm_complete(
    {'model_name': 'ClaudeModel'},
    {'prompt': 'Explain what a database is in simple terms'}
);
```

## Available Claude Models

Anthropic offers several Claude models with different capabilities and price points:

| Model | Description | Best For |
|-------|-------------|----------|
| `claude-3-opus-20240229` | Most capable model | Complex analysis, research, nuanced tasks |
| `claude-3-sonnet-20240229` | Balanced performance and speed | General purpose tasks |
| `claude-3-haiku-20240307` | Fastest and most cost-effective | Quick responses, high-volume tasks |
| `claude-3-5-sonnet-20241022` | Latest Sonnet version | Best overall performance for most tasks |

## Model Parameters

You can customize Claude's behavior with model parameters:

```sql
CREATE MODEL(
   'ClaudeCustom',
   'claude-3-5-sonnet-20241022',
   'anthropic',
   {
     "model_parameters": {
       "temperature": 0.5,
       "max_tokens": 2048,
       "system": "You are a helpful data analyst assistant."
     }
   }
);
```

### Supported Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `temperature` | Controls randomness (0.0 to 1.0) | 1.0 |
| `max_tokens` | Maximum response length (required by Anthropic) | 1024 |
| `system` | System prompt for context and instructions | None |
| `top_p` | Nucleus sampling threshold | 1.0 |
| `top_k` | Limits token selection to top K options | None |

## System Prompts

Unlike other providers, Anthropic handles system prompts separately from the message history. You can set a system prompt in the model parameters:

```sql
CREATE MODEL(
   'AnalystClaude',
   'claude-3-haiku-20240307',
   'anthropic',
   {
     "model_parameters": {
       "system": "You are an expert data analyst. Always provide structured, actionable insights.",
       "max_tokens": 1024
     }
   }
);
```

## Image Support

Claude models support image analysis. Images must be provided as base64-encoded data:

```sql
SELECT llm_complete(
    {'model_name': 'ClaudeModel'},
    {
        'prompt': 'Describe what you see in this image',
        'context_columns': [
            {'data': image_column, 'type': 'image'}
        ]
    }
) AS description
FROM images_table;
```

Note: URL-based images are not directly supported. Images must be base64-encoded before use.

## Structured Output

Anthropic supports structured output through their tool_use feature. Flock automatically handles this conversion:

```sql
SELECT llm_complete(
    {'model_name': 'ClaudeModel',
     'model_parameters': '{
       "response_format": {
         "type": "json_schema",
         "json_schema": {
           "name": "analysis",
           "schema": {
             "type": "object",
             "properties": {
               "sentiment": {"type": "string"},
               "confidence": {"type": "number"}
             },
             "required": ["sentiment", "confidence"]
           }
         }
       }
     }'
    },
    {'prompt': 'Analyze the sentiment of this text: I love this product!'}
) AS analysis;
```

## Important: No Embedding Support

Anthropic does not provide an embeddings API. If you need embeddings for similarity search or RAG pipelines, use OpenAI or Ollama instead:

```sql
-- For embeddings, use a different provider
CREATE MODEL('EmbeddingModel', 'text-embedding-3-small', 'openai');

-- Use Claude for completions
CREATE MODEL('ClaudeModel', 'claude-3-haiku-20240307', 'anthropic');
```

Attempting to use `llm_embedding` with an Anthropic model will result in a clear error message.

## Rate Limits and Usage

Anthropic has rate limits based on your plan tier. Monitor your usage in the [Anthropic Console](https://console.anthropic.com/). Consider using batch processing with appropriate `batch_size` settings for high-volume workloads.

## API Version

The default API version is `2023-06-01`. The adapter automatically includes the required `anthropic-version` header with all requests.
