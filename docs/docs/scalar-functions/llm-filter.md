---
title: llm_filter
sidebar_position: 3
---

# llm_filter Function

The `llm_filter` function evaluates a condition based on a given prompt and returns a boolean value (`TRUE` or `FALSE`). This function mostly used in the workload of `WHERE` clause of a query.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Simple Usage (without data)

### 1.1 Using an Inline Prompt

```sql
SELECT * 
FROM products
WHERE llm_filter(
    {'model_name': 'gpt-4'}, 
    {'prompt': 'Is this product description eco-friendly?'}, 
    {'description': product_description}
);
```

**Description**: This example uses an inline prompt to filter rows based on whether the product description is considered eco-friendly by the `gpt-4` model. If the model returns `TRUE`, the row is included in the result.

### 1.2 Using a Named Prompt

```sql
SELECT * 
FROM products
WHERE llm_filter(
    {'model_name': 'gpt-4'}, 
    {'prompt_name': 'eco-friendly-check'}, 
    {'description': product_description}
);
```

**Description**: In this example, a named prompt (`eco-friendly-check`) is used to determine if the product description is eco-friendly. This allows for reusing pre-configured prompts for similar filtering tasks.

### 1.3 Combining with Other SQL Logic

```sql
WITH filtered_products AS (
    SELECT product_id, product_name, product_description
    FROM products
    WHERE llm_filter(
        {'model_name': 'gpt-4', 'secret_name': 'openai_key'}, 
        {'prompt': 'Is this product description eco-friendly?'}, 
        {'description': product_description}
    )
)
SELECT * FROM filtered_products;
```

**Description**: This example demonstrates how to combine `llm_filter` with other SQL logic. It filters the products based on the eco-friendliness of their descriptions and processes the result in a subquery for further use.

### 1.4 Actual Usage (with data)

```sql
WITH relevant_reviews AS (
    SELECT review_id, review_content
    FROM reviews
    WHERE llm_filter(
        {'model_name': 'gpt-4'}, 
        {'prompt': 'Does this review content contain a positive sentiment?'}, 
        {'content': review_content}
    )
)
SELECT * FROM relevant_reviews
WHERE LENGTH(review_content) > 50;
```

**Description**: This actual example uses `llm_filter` to filter reviews based on positive sentiment. It then further filters the results to only include reviews with content longer than 50 characters.

## 2. Input Parameters

The `llm_filter` function accepts three structured inputs: model configuration, prompt configuration, and input data columns.

### 2.1 Model Configuration

- **Parameter**: `model_name` and `secret_name`

#### 2.1.1 Model Selection

- **Description**: Specifies the model used for text generation.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4' }
  ```

#### 2.1.2 Model Selection with Secret

- **Description**: Specifies the model along with the secret name to be used for authentication when accessing the model.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4', 'secret_name': 'your_secret_name' }
  ```

### 2.2 Prompt Configuration

- **Parameter**: `prompt` or `prompt_name`

  #### 2.2.1 Inline Prompt

  Directly provides the prompt.

  - **Example**:
    ```sql
    { 'prompt': 'Summarize the content of the article.' }
    ```

  #### 2.2.2 Named Prompt

  References a pre-configured prompt.

  - **Example**:
    ```json
    { 'prompt_name': 'summarize-content' }
    ```

  #### 2.2.3 Named Prompt with Version

  References a specific version of a prompt.

  - **Example**:
    ```json
    { 'prompt_name': 'summarize-content', 'version': 2 }
    ```

### 2.3 Input Data Columns (OPTIONAL)

- **Parameter**: Column mappings
- **Description**: Specifies the columns from the table to be passed to the model as input.
- **Example**:
  ```sql
  {'product_name': product_name, 'product_description': product_description}
  ```

## 3. Output

The function returns a **BOOLEAN** value (`TRUE` or `FALSE`), indicating whether the row satisfies the condition specified in the prompt.

**Example Output**:  
For a prompt like *"Is this product description eco-friendly?"*:

- **Input Row**:  
  - `product_description`: *"Made from 100% recyclable materials, this product is perfect for eco-conscious buyers."*
- **Output**:  
  - `TRUE`
