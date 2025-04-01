---
title: Azure
sidebar_position: 1
---

# FlockMTL Using Azure

Before starting, ensure you have followed the [Getting Started](/docs/getting-started) guide to setup FlockMTL. Next you need to get the necesary credentials to use Azure API check the [Microsoft Azure](https://learn.microsoft.com/en-us/azure/ai-services/openai/reference) Page.

Now you can dive into the details of using Azure provider with FlockMTL in the next section:

- [Azure Provider Setup](#azure-provider-setup)

## Azure Provider Setup

To use the Azure API, you need only to do two required steps:

- First, create a secret with your Azure API key, resource name, and API version. You can do this by running the following SQL command:

```sql
CREATE SECRET (
    TYPE AZURE_LLM,
    API_KEY 'your-key-here',
    RESOURCE_NAME 'resource-name',
    API_VERSION 'api-version'
);
```
- Create your Azure model in the model manager. Make sure that the name of the model is unique. You can do this by running the following SQL command:

```sql
CREATE MODEL(
   'QuackingModel',
   'gpt-4o',
   'azure',
   {"context_window": 128000, "max_output_tokens": 16384}
);
```
- Now you simply use FlockMTL with Azure provider. Here's a small query to run to test if everything is working:

```sql
SELECT llm_complete(
        {'model_name': 'QuackingModel'},
        {'prompt': 'Talk like a duck 🦆 and write a poem about a database 📚'}
        );
```