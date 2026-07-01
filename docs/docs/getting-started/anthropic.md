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

- First, create a secret with your Anthropic API key. Optionally, you can specify an API version (defaults to `2023-06-01`):

```sql
-- Minimal: API key only (uses default API version)
CREATE SECRET (
    TYPE ANTHROPIC,
    API_KEY 'your-api-key'
);

-- With custom API version (optional)
CREATE SECRET (
    TYPE ANTHROPIC,
    API_KEY 'your-api-key',
    API_VERSION '2024-01-01'
);
```

- Next, create your Claude model in the model manager. Make sure that the name of the model is unique:

```sql
CREATE MODEL(
   'ClaudeModel',
   'claude-sonnet-4-5',
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

Anthropic offers several Claude models with different capabilities and price points. See the [Anthropic Models Documentation](https://docs.anthropic.com/en/docs/about-claude/models) for the full list.

| Model | Description | API Method |
|-------|-------------|------------|
| `claude-sonnet-4-5` | Latest Sonnet, best overall | `output_format` |
| `claude-haiku-4-5` | Fast and cost-effective | `output_format` |
| `claude-opus-4-5` | Most capable | `output_format` |
| `claude-3-5-sonnet-20241022` | Previous Sonnet version | `tool_use` |
| `claude-3-haiku-20240307` | Previous Haiku version | `tool_use` |

All models are fully supported. The adapter automatically selects the appropriate API method based on model version.

## Model Parameters

You can customize Claude's behavior with model parameters:

```sql
CREATE MODEL(
   'ClaudeCustom',
   'claude-sonnet-4-5',
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

Flock supports all parameters provided by the [Anthropic Messages API](https://docs.anthropic.com/en/api/messages). Common examples include:

- `temperature` - Controls randomness (0.0 to 1.0)
- `max_tokens` - Maximum response length (required by Anthropic, default: 4096)
- `system` - System prompt for context and instructions
- `top_p` - Nucleus sampling threshold
- `top_k` - Limits token selection to top K options

## System Prompts

Unlike other providers, Anthropic handles system prompts separately from the message history. You can set a system prompt in the model parameters:

```sql
CREATE MODEL(
   'AnalystClaude',
   'claude-sonnet-4-5',
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

Claude models support image analysis. Images can be provided as:

- **URLs** – `http://` or `https://` URLs are downloaded and converted to base64 automatically
- **File paths** – Local files are read and converted to base64
- **Base64-encoded data** – Used directly

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

URLs and file paths are resolved the same way as with OpenAI and Ollama.

## Structured Output

Flock ensures structured JSON responses from all Claude models using a hybrid approach:

- **Claude 4.x models**: Uses the native [`output_format`](https://docs.anthropic.com/en/docs/build-with-claude/structured-outputs) API (preferred, guaranteed schema compliance)
- **Claude 3.x models**: Falls back to [`tool_use`](https://docs.anthropic.com/en/docs/build-with-claude/tool-use) for structured output (compatible with older models)

This hybrid approach ensures all Claude models work seamlessly with Flock, while using the optimal API for each model version.

### Custom Schema (Claude 4.x only)

For Claude 4.x models, you can specify a custom JSON schema:

```sql
SELECT llm_complete(
    {'model_name': 'ClaudeModel',
     'model_parameters': '{
       "output_format": {
         "type": "json_schema",
         "schema": {
           "type": "object",
           "properties": {
             "sentiment": {"type": "string"},
             "confidence": {"type": "number"}
           },
           "required": ["sentiment", "confidence"],
           "additionalProperties": false
         }
       }
     }'
    },
    {'prompt': 'Analyze the sentiment of this text: I love this product!'}
) AS analysis;
```

Note: Custom schemas require `"additionalProperties": false` for all objects.

## Important: No Embedding Support

Anthropic does not provide an embeddings API. If you need embeddings for similarity search or RAG pipelines, use OpenAI or Ollama instead:

```sql
-- For embeddings, use a different provider
CREATE MODEL('EmbeddingModel', 'text-embedding-3-small', 'openai');

-- Use Claude for completions
CREATE MODEL('ClaudeModel', 'claude-sonnet-4-5', 'anthropic');
```

Attempting to use `llm_embedding` with an Anthropic model will result in a clear error message.

## Rate Limits and Usage

Anthropic has rate limits based on your plan tier. Monitor your usage in the [Anthropic Console](https://console.anthropic.com/). Consider using batch processing with appropriate `batch_size` settings for high-volume workloads.

## API Version

The default API version is `2023-06-01`. You can override this when creating a secret by passing the optional `API_VERSION` parameter. The adapter automatically includes the required `anthropic-version` header with all requests.
