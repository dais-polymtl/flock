---
title: OpenAI  
sidebar_position: 3
---

# FlockMTL Using OpenAI 

In this section we will cover how to set up and use OpenAI provider and OpenAI-Compatible provider with FlockMTL.

Before starting, you should have already followed the [Getting Started](/docs/getting-started) guide.

# Table of Contents

Below is a quick overview to help you navigate with ease through the documentation. You can click on any of the links to jump to the relevant section.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## OpenAI Provider Setup

To use OpenAI provider, you need first to get your OpenAI API key from the [OpenAI website](https://platform.openai.com/api-keys). Only two steps are required:

- First Creating a secret with your OpenAI API key.
```sql  
CREATE SECRET (
    TYPE OPENAI,
    API_KEY 'your-api-key'
);  
```
- Create your OpenAI model in the model manager. Make sure that the name of the model is unique.
```sql
CREATE MODEL(
   'QuackingModel',
   'gpt-4o', 
   'openai', 
   {"context_window": 128000, "max_output_tokens": 8400}
);
```

- Now you simply use FlockMTL with OpenAI provider. Here's a small query to run to test if everything is working:

```sql
SELECT llm_complete(
        {'model_name': 'QuackingModel'},
        {'prompt': 'Talk like a duck 🦆 and write a poem about a database 📚'}
        );
```

## OpenAI-Compatible Provider Setup

For providers with OpenAI-compatible APIs, specify the `BASE_URL` along with your API key, to illustrate we will use [groq](https://groq.com/) as an example. You need only two required steps:

- First Creating a secret for you `groq` provider by specifying its `BASE_URL` and `API_KEY`.
```sql  
CREATE SECRET (
    TYPE OPENAI,
    BASE_URL 'https://api.groq.com/openai/v1/',
    API_KEY 'your-api-key'
);
```
- Next, create your OpenAI-compatible model in the model manager. Make sure that the name of the model is unique.
```sql
CREATE MODEL(
   'QuackingModel',
   'gpt-4o', 
   'openai', 
   {"context_window": 128000, "max_output_tokens": 8400}
);
```

- Same as before, you can test if everything is working with the same query:

```sql
SELECT llm_complete(
        {'model_name': 'QuackingModel'},
        {'prompt': 'Talk like a duck 🦆 and explain what a database is 📚'}
        );
```
