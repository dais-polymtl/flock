---
title: llm_complete
sidebar_position: 1
---

# llm_complete Function

The `llm_complete` function generates text completions using specified models and prompts for dynamic data generation.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Simple Usage (without data)

### 1.1 Inline Prompt

```sql
SELECT llm_complete(
           {'model_name': 'gpt-4o'},
    {'prompt': 'Explain the purpose of Flock.'}
) AS flock_purpose;
```

**Description**: This example uses an inline prompt to generate a text completion with the `gpt-4` model. The prompt
asks the model to explain the purpose of Flock. The function returns a completion for each row based on the provided
prompt.

### 1.2 Named Prompt

```sql
SELECT llm_complete(
           {'model_name': 'summarizer', 'secret_name': 'summarizer_secret'},
    {'prompt_name': 'description-generation', 'version': 1, 'context_columns': [{'data': product_name}]}
) AS product_description
FROM UNNEST(['Wireless Headphones', 'Gaming Laptop', 'Smart Watch']) AS t(product_name);
```

**Description**: In this example, a named prompt `description-generation` is used with the `summarizer` model. The
function generates product descriptions using data from the `product_name` column for each row in the sample data.

## 2. Actual Usage (with data)

```sql
WITH sample_products AS (
    SELECT *
    FROM (VALUES
        (1, 'MacBook Pro', 'High-performance laptop'),
        (2, 'AirPods Pro', 'Wireless noise-cancelling earbuds'),
        (3, 'iPad Air', 'Lightweight tablet for creativity')
    ) AS t(product_id, product_name, description)
),
enhanced_products AS (
    SELECT product_id,
           product_name,
           llm_complete(
               {'model_name': 'reduce-model'},
               {'prompt_name': 'summarize-content', 'version': 2, 'context_columns': [{'data': product_name}]}
           ) AS enhanced_description
    FROM sample_products
)
SELECT product_id, product_name, enhanced_description
FROM enhanced_products
WHERE LENGTH(enhanced_description) > 50;
```

**Description**: This actual example demonstrates the use of a pre-configured prompt `summarize-content` with version
`2` and the `reduce-model`. It processes the `product_name` column and generates a summarized description. The query
then filters out rows where the generated description is shorter than 50 characters.

### 2.1 Image Analysis Example

```sql
-- Analyze product images and generate descriptions
SELECT
    product_id,
    product_name,
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'Describe this product image in detail, focusing on key features and quality.',
            'context_columns': [
                {'data': product_name},
                {'data': image_url, 'type': 'image'}
            ]
        }
    ) AS image_analysis
FROM VALUES
    (1, 'Wireless Headphones', 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=400'),
    (2, 'Gaming Laptop', 'https://images.unsplash.com/photo-1496181133206-80ce9b88a853?w=400'),
    (3, 'Smart Watch', 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=400')
AS t(product_id, product_name, image_url);
```

**Description**: This example demonstrates using `llm_complete` with image data. It analyzes product images and
generates detailed descriptions, combining the product name with visual analysis of the image content.

## 3. Input Parameters

The `llm_complete` function accepts three structured inputs: model configuration, prompt configuration, and input data
columns.

### 3.1 Model Configuration

- **Parameter**: `model_name` and `secret_name`

#### 3.1.1 Model Selection

- **Description**: Specifies the model used for text generation.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4o' }
  ```

#### 3.1.2 Model Selection with Secret

- **Description**: Specifies the model along with the secret name to be used for authentication when accessing the
  model.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4o', 'secret_name': 'your_secret_name' }
  ```

### 3.2 Prompt Configuration

- **Parameter**: `prompt` or `prompt_name` with `context_columns`

  #### 3.2.1 Inline Prompt

  Directly provides the prompt with context columns.

    - **Example**:
      ```sql
      { 'prompt': 'Summarize the content: {article}', 'context_columns': [{'data': article_content, 'name': 'article'}] }
      ```

  #### 3.2.2 Named Prompt

  References a pre-configured prompt with context columns.

    - **Example**:
      ```json
      { 'prompt_name': 'summarize-content', 'context_columns': [{'data': article_content}] }
      ```

  #### 3.2.3 Named Prompt with Version

  References a specific version of a prompt with context columns.

    - **Example**:
      ```json
      { 'prompt_name': 'summarize-content', 'version': 2, 'context_columns': [{'data': article_content}] }
      ```

### 3.3 Context Columns Configuration

- **Parameter**: `context_columns` array
- **Description**: Specifies the columns from the table to be passed to the model as input. Each column can have three
  properties:
    - `data`: The SQL column data (required)
    - `name`: Custom name for the column to be referenced in the prompt (optional)
    - `type`: Data type - "tabular" (default) or "image" (optional)
- **Example**:
  ```sql
  'context_columns': [
    {'data': product_name, 'name': 'product'},
    {'data': image_url, 'type': 'image'},
    {'data': description}
  ]
  ```

## 4. Output

The function generates a text completion for each row based on the provided prompt and input data.

- **Column Type**: JSON
- **Behavior**: Maps over each row and generates a response per tuple.
