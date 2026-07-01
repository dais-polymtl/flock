---
title: Ollama
sidebar_position: 2
---

# Flock Using Ollama

Choosing Ollama is a good option for local model execution. It allows you to run models locally without relying on
external APIs. But we should first set up the Ollama API and model. Here's how to do it:

- **Install Ollama**: Ensure Ollama is installed. If not, download it from
  the [Ollama download page](https://ollama.com/download).
- **Download the Model**: Confirm that the required model is available on your system.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Ollama Setup

After making sure that you have Ollama installed and the model downloaded, you can start using it with Flock. The same
as the other providers only two steps are required:

- First, create a secret with your Ollama API URL. You can do this by running the following SQL command, where Ollama is
  running locally, and the default URL is `127.0.0.1:11434` :

```sql
CREATE SECRET (
    TYPE OLLAMA,
    API_URL '127.0.0.1:11434'
);
```

- Next, create your Ollama model in the model manager. Make sure that the name of the model is unique, and the model
  you're using is already downloaded. You can create your model by running the following SQL command:

```sql
CREATE MODEL(
   'QuackingModel',
   'llama3.2',
   'ollama',
   {"tuple_format": "json", "batch_size": 32, "model_parameters": {"temperature": 0.7}}
);
```

- Now you simply use Flock with Ollama provider. Here's a small query to run to test if everything is working:

```sql
SELECT llm_complete(
    {'model_name': 'QuackingModel'},
    {'prompt': 'Talk like a duck 🦆 and write a poem about a database 📚'}
);
```
