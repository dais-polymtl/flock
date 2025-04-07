---
title: llm_complete_json
sidebar_position: 2
---

# llm_complete_json Function

The `llm_complete_json` function extends the capabilities of `llm_complete` by producing JSON responses.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Simple Usage (without data)

### 1.1 Using an Inline Prompt

```sql
WITH product_data AS (
    SELECT 
        product_id, 
        product_name, 
        llm_complete_json(
            {'model_name': 'gpt-4'}, 
            {'prompt': 'Using the next product name generate detailed product information that contains name, description and a list of features.'},
            {'product_name': product_name}
        ) AS product_info
    FROM products
)
SELECT 
    product_id, 
    product_info.name AS name, 
    product_info.description AS description
FROM product_data;
```

**Description**: This example uses an inline prompt to generate detailed product information in JSON format. The `gpt-4` model is used to process the `product_name` column and generate structured data such as `name`, `description`, and `features` for each product.

### 1.2 Using a Named Prompt

```sql
WITH product_features AS (
    SELECT 
        product_id, 
        llm_complete_json(
            {'model_name': 'reduce-model', 'secret_name': 'my_new_key'}, 
            {'prompt_name': 'product-features', 'version': 1}, 
            {'product_name': product_name}
        ) AS feature_info
    FROM products
)
SELECT 
    product_id, 
    feature_info.name AS name, 
    feature_info.features AS features
FROM product_features;
```

**Description**: This example demonstrates the use of a named prompt (`product-features`) with version `1` to generate structured JSON data. The `reduce-model` processes the `product_name` column and outputs the product name along with a list of its features.

## 2. Actual Usage (with data)

```sql
WITH detailed_products AS (
    SELECT 
        product_id, 
        llm_complete_json(
            {'model_name': 'gpt-4'}, 
            {'prompt': 'Using the next product name generate detailed product information that contains name, description and a list of features.'}, 
            {'product_name': product_name}
        ) AS product_summary
    FROM products
)
SELECT 
    product_id, 
    product_summary.name AS name, 
    product_summary.description AS description,
    json_array_length(product_summary.features) AS feature_count
FROM detailed_products
WHERE json_array_length(product_summary.features) > 2;
```

**Description**: This actual example uses the `gpt-4` model to generate detailed product information, including a list of features. The query also calculates the number of features (`feature_count`) using `json_array_length()` and filters out products with fewer than 3 features.

## 3. Input Parameters

The `llm_complete` function accepts three structured inputs: model configuration, prompt configuration, and input data columns.

### 3.1 Model Configuration

- **Parameter**: `model_name` and `secret_name`

#### 3.1.1 Model Selection

- **Description**: Specifies the model used for text generation.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4' }
  ```

#### 3.1.2 Model Selection with Secret

- **Description**: Specifies the model along with the secret name to be used for authentication when accessing the model.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4', 'secret_name': 'your_secret_name' }
  ```

### 3.2 Prompt Configuration

- **Parameter**: `prompt` or `prompt_name`

  #### 3.2.1 Inline Prompt

  Directly provides the prompt.

  - **Example**:
    ```sql
    { 'prompt': 'Summarize the content of the article.' }
    ```

  #### 3.2.2 Named Prompt

  References a pre-configured prompt.

  - **Example**:
    ```json
    { 'prompt_name': 'summarize-content' }
    ```

  #### 3.2.3 Named Prompt with Version

  References a specific version of a prompt.

  - **Example**:
    ```json
    { 'prompt_name': 'summarize-content', 'version': 2 }
    ```

### 3.3 Input Data Columns (OPTIONAL)

- **Parameter**: Column mappings
- **Description**: Specifies the columns from the table to be passed to the model as input.
- **Example**:
  ```sql
  {'product_name': product_name, 'product_description': product_description}
  ```

## 4. Output

The function returns a **JSON object** for each row, which can be accessed dynamically to retrieve specific fields.

**Example Output**:  
For a prompt like *"Generate detailed product information that contains name, description and a list of features."*, you might see:  
- **Input Row**:  
  - `product_name`: *"Wireless Headphones"*  
- **Output JSON**:  
  ```json
  {
    "name": "Wireless Headphones",
    "description": "High-quality headphones with noise cancellation.",
    "features": ["Wireless", "Noise Cancellation", "Extended Battery Life"]
  }
  ```

You can access individual fields such as `.name`, `.description`, or `.features` for downstream tasks.
