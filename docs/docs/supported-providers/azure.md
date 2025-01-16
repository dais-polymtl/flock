---
title: Azure
sidebar_position: 3
---

# Setting Up and Using Azure API

## 1. Configuring the Azure API

To use the Azure API, configure it using the secret manager. Use the following query to set the required details:

```sql
CREATE SECRET (
    TYPE AZURE_LLM,
    API_KEY 'your-key-here',
    RESOURCE_NAME 'resource-name',
    API_VERSION 'api-version'
);
```

This command securely stores the Azure API key, resource name, and API version. For more details, refer to the [secret management](/docs/resource-management/secret-management) section.

## 2. Creating an Azure Model

Define your Azure model in the model manager with this query:

```sql
CREATE MODEL('azure-gpt', 'gpt-4o', 'azure', {"context_window": 128000, "max_output_tokens": 16384});
```

For more information on managing models, refer to the [model management](/docs/resource-management/model-management) section.

## 3. Practical Example

Generate product descriptions using the `azure-gpt` model for entries in a `products` table:

```sql
SELECT llm_complete(
    {'model_name': 'azure-gpt'},
    {'prompt': 'Generate a description for the next product based on its product name.'},
    {'product_name': product_name}
) AS product_description
FROM products;
```
