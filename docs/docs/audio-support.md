---
title: Audio Transcription
sidebar_position: 7
---

# Audio Transcription in Flock

Flock supports audio transcription in SQL by sending audio inputs to compatible providers and returning text transcripts
that you can join, filter, and analyze like any other column.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Overview

With audio support you can:

- Transcribe spoken content (meetings, calls, notes) directly in DuckDB.
- Combine transcripts with structured data for analytics.
- Feed transcripts into `llm_complete`, `llm_filter`, or `llm_embedding` for downstream tasks (summarization,
  classification, similarity search, RAG, etc.).

Flock uses the same `context_columns` abstraction as for images, but with `type: 'audio'` and a required
`transcription_model`.

## Supported Providers

Audio transcription is supported for:

- **OpenAI** – via the `audio/transcriptions` endpoint (e.g., Whisper models).
- **Azure OpenAI** – via the Azure audio transcription endpoint.

The following providers **do not** support audio transcription:

- **Anthropic/Claude** – not supported; calls will raise an error.
- **Ollama** – not supported; calls will raise an error.

Refer to the provider-specific getting-started guides for API key setup:

- [OpenAI](/docs/getting-started/openai)
- [Azure](/docs/getting-started/azure)
- [Anthropic](/docs/getting-started/anthropic) (for completions/vision only, no audio)

## Using Audio in Context Columns

To use audio in Flock functions, specify `type: 'audio'` and provide a `transcription_model` in the `context_columns`
array. The audio must be accessible as a file path or URL (depending on the provider).

### Context Column Structure for Audio

```sql
'context_columns': [
  {
    'data': audio_path,
    'type': 'audio',
    'transcription_model': 'whisper-1'
  }
]
```

Each audio context column supports:

- **`data`** _(required)_: SQL column containing the audio source (local file path or URL, depending on provider).
- **`type`** _(required for audio)_: Must be set to `'audio'`.
- **`transcription_model`** _(required when `type = 'audio'`)_: Provider-specific transcription model name.
- **`name`** _(optional)_: Alias for referencing in prompts after transcription.

### Validation Rules

Flock enforces the following rules at bind time:

- If `type = 'audio'`, then `transcription_model` **must** be provided, otherwise an error is raised.
- If `transcription_model` is provided but `type` is not `'audio'`, Flock raises an error.

## Basic Transcription Example

The most common pattern is to transcribe audio into text, then store or further process the transcript.

```sql
-- Transcribe a list of audio files with OpenAI
SELECT
    audio_id,
    file_path,
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'Transcribe the following audio file verbatim.',
            'context_columns': [
                {
                    'data': file_path,
                    'type': 'audio',
                    'transcription_model': 'whisper-1'
                }
            ]
        }
    ) AS transcript
FROM VALUES
    (1, '/data/audio/meeting_01.mp3'),
    (2, '/data/audio/meeting_02.mp3')
AS t(audio_id, file_path);
```

## Summarizing Transcripts

After transcription, you can treat the transcript as regular text and chain additional LLM calls.

```sql
WITH raw_transcripts AS (
    SELECT
        audio_id,
        llm_complete(
            {'model_name': 'gpt-4o'},
            {
                'prompt': 'Transcribe the following audio file verbatim.',
                'context_columns': [
                    {
                        'data': file_path,
                        'type': 'audio',
                        'transcription_model': 'whisper-1'
                    }
                ]
            }
        ) AS transcript
    FROM VALUES
        (1, '/data/audio/support_call_01.wav'),
        (2, '/data/audio/support_call_02.wav')
    AS t(audio_id, file_path)
)
SELECT
    audio_id,
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'Summarize this call in 3 bullet points.',
            'context_columns': [
                {'data': transcript, 'name': 'call'}
            ]
        }
    ) AS call_summary
FROM raw_transcripts;
```

## Filtering Based on Audio Content

You can also use `llm_filter` to flag or select rows based on the audio’s content:

```sql
-- Flag calls that mention cancellations
SELECT
    audio_id,
    customer_id,
    file_path
FROM VALUES
    (1, 101, '/data/audio/call_01.wav'),
    (2, 102, '/data/audio/call_02.wav'),
    (3, 103, '/data/audio/call_03.wav')
AS t(audio_id, customer_id, file_path)
WHERE llm_filter(
    {'model_name': 'gpt-4o'},
    {
        'prompt': 'Does this call mention cancelling a subscription? Answer true or false.',
        'context_columns': [
            {
                'data': file_path,
                'type': 'audio',
                'transcription_model': 'whisper-1'
            }
        ]
    }
);
```

## Embeddings from Audio (via Text)

There is no direct audio embedding API in Flock. Instead, you can:

1. Transcribe audio into text.
2. Generate embeddings from the transcript using `llm_embedding`.

```sql
WITH transcripts AS (
    SELECT
        audio_id,
        llm_complete(
            {'model_name': 'gpt-4o'},
            {
                'prompt': 'Transcribe the following audio file.',
                'context_columns': [
                    {
                        'data': file_path,
                        'type': 'audio',
                        'transcription_model': 'whisper-1'
                    }
                ]
            }
        ) AS transcript
    FROM VALUES
        (1, '/data/audio/note_01.m4a'),
        (2, '/data/audio/note_02.m4a')
    AS t(audio_id, file_path)
),
audio_embeddings AS (
    SELECT
        audio_id,
        llm_embedding(
            {'model_name': 'text-embedding-3-small'},
            {
                'context_columns': [
                    {'data': transcript}
                ]
            }
        ) AS embedding
    FROM transcripts
)
SELECT * FROM audio_embeddings;
```

## Function Support for Audio

Audio transcription is available in the following functions (via `type: 'audio'` + `transcription_model`):

| Function        | Audio Support | Description                                  |
| --------------- | ------------- | -------------------------------------------- |
| `llm_complete`  | ✅ Full       | Transcribe and optionally transform content  |
| `llm_filter`    | ✅ Full       | Filter rows based on audio-derived semantics |
| `llm_reduce`    | ✅ Full       | Summarize or aggregate transcripts           |
| `llm_rerank`    | ✅ Via text   | Rerank based on derived text features        |
| `llm_first`     | ✅ Via text   | Pick top row based on transcript criteria    |
| `llm_last`      | ✅ Via text   | Pick bottom row based on transcript criteria |
| `llm_embedding` | ✅ Via text   | Embeddings over transcripts (not raw audio)  |

For image-specific workflows, see the [Image Support](/docs/image-support) page.
