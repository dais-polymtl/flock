---
title: llm_complete
sidebar_position: 1
---

# llm_complete Function

The `llm_complete` function generates text completions using specified models and prompts for dynamic data generation.

## 1. Basic Usage Examples

### 1.1 Inline Prompt

```sql
SELECT llm_complete(
    {'model_name': 'gpt-4'},
    {'prompt': 'Explain the purpose of FlockMTL.'}
) AS flockmtl_purpose;
```

**Description**: This example uses an inline prompt to generate a text completion with the `gpt-4` model. The prompt asks the model to explain the purpose of FlockMTL. The function returns a completion for each row based on the provided prompt.

### 1.2 Named Prompt

```sql
SELECT llm_complete(
    {'model_name': 'summarizer', 'secret_name': 'summarizer_secret'},
    {'prompt_name': 'description-generation', 'version': 1},
    {'product_name': product_name}
) AS product_description
FROM products;
```

**Description**: In this example, a named prompt `description-generation` is used with the `summarizer` model. The function generates product descriptions using data from the `product_name` column for each row in the `products` table.

## 2. Advanced Example

```sql
WITH enhanced_products AS (
    SELECT
        product_id,
        product_name,
        llm_complete(
            {'model_name': 'reduce-model'},
            {'prompt_name': 'summarize-content', 'version': 2},
            {'product_name': product_name}
        ) AS enhanced_description
    FROM products
)
SELECT product_id, product_name, enhanced_description
FROM enhanced_products
WHERE LENGTH(enhanced_description) > 50;
```

**Description**: This advanced example demonstrates the use of a pre-configured prompt `summarize-content` with version `2` and the `reduce-model`. It processes the `product_name` column and generates a summarized description. The query then filters out rows where the generated description is shorter than 50 characters.

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

The function generates a text completion for each row based on the provided prompt and input data.

- **Type**: Text (string)
- **Behavior**: Maps over each row and generates a response per tuple.