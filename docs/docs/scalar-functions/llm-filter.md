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
FROM VALUES
    (1, 'Eco-friendly bamboo toothbrush made from sustainable materials'),
    (2, 'Plastic water bottle made from recycled content'),
    (3, 'Organic cotton t-shirt with natural dyes')
AS t(product_id, product_description)
WHERE llm_filter(
    {'model_name': 'gpt-4o'},
    {'prompt': 'Is this product description eco-friendly?', 'context_columns': [{'data': product_description, 'name': 'description'}]}
);
```

**Description**: This example uses an inline prompt to filter rows based on whether the product description is considered eco-friendly by the `gpt-4` model. If the model returns `TRUE`, the row is included in the result.

### 1.2 Using a Named Prompt

```sql
SELECT *
FROM VALUES
    (1, 'Solar-powered garden lights with LED technology'),
    (2, 'Disposable plastic cups for parties'),
    (3, 'Reusable stainless steel water bottle')
AS t(product_id, product_description)
WHERE llm_filter(
    {'model_name': 'gpt-4o'},
    {'prompt_name': 'eco-friendly-check', 'context_columns': [{'data': product_description, 'name': 'description'}]}
);
```

**Description**: In this example, a named prompt (`eco-friendly-check`) is used to determine if the product description is eco-friendly. This allows for reusing pre-configured prompts for similar filtering tasks.

### 1.3 Combining with Other SQL Logic

```sql
WITH sample_products AS (
    SELECT *
    FROM (VALUES
        (1, 'Eco-friendly bamboo phone case', 'Made from sustainable bamboo'),
        (2, 'Plastic keyboard covers', 'Protective covers for keyboards'),
        (3, 'Biodegradable cleaning supplies', 'Plant-based cleaning products')
    ) AS t(product_id, product_name, product_description)
),
filtered_products AS (
    SELECT product_id, product_name, product_description
    FROM sample_products
    WHERE llm_filter(
        {'model_name': 'gpt-4o', 'secret_name': 'openai_key'},
        {'prompt': 'Is this product description eco-friendly?', 'context_columns': [{'data': product_description, 'name': 'description'}]}
    )
)
SELECT * FROM filtered_products;
```

**Description**: This example demonstrates how to combine `llm_filter` with other SQL logic. It filters the products based on the eco-friendliness of their descriptions and processes the result in a subquery for further use.

### 1.4 Actual Usage (with data)

```sql
WITH sample_reviews AS (
    SELECT *
    FROM (VALUES
        (1, 'This product exceeded my expectations! Amazing quality and fast delivery.'),
        (2, 'Terrible experience. Product broke after one day and customer service was unhelpful.'),
        (3, 'Good value for money. Works as expected and arrived on time.'),
        (4, 'Outstanding! Best purchase I have made this year. Highly recommended!')
    ) AS t(review_id, review_content)
),
relevant_reviews AS (
    SELECT review_id, review_content
    FROM sample_reviews
    WHERE llm_filter(
        {'model_name': 'gpt-4o'},
        {'prompt': 'Does this review content contain a positive sentiment?', 'context_columns': [{'data': review_content, 'name': 'content'}]}
    )
)
SELECT * FROM relevant_reviews
WHERE LENGTH(review_content) > 50;
```

**Description**: This actual example uses `llm_filter` to filter reviews based on positive sentiment. It then further filters the results to only include reviews with content longer than 50 characters.

### 1.5 Image Filtering Example

```sql
-- Filter products with high-quality product images
SELECT
    product_id,
    product_name,
    image_url
FROM VALUES
    (1, 'Premium Headphones', 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=400', 150.00),
    (2, 'Gaming Mouse', 'https://images.unsplash.com/photo-1527814050087-3793815479db?w=400', 75.00),
    (3, 'Wireless Keyboard', 'https://images.unsplash.com/photo-1587829741301-dc798b83add3?w=400', 120.00)
AS t(product_id, product_name, image_url, price)
WHERE llm_filter(
    {'model_name': 'gpt-4o'},
    {
        'prompt': 'Is this a high-quality, professional product photo with good lighting and clear visibility of the product?',
        'context_columns': [
            {'data': image_url, 'type': 'image'},
            {'data': product_name}
        ]
    }
)
AND price > 50;
```

## 2. Input Parameters

The `llm_filter` function accepts three structured inputs: model configuration, prompt configuration, and input data columns.

### 2.1 Model Configuration

- **Parameter**: `model_name` and `secret_name`

#### 2.1.1 Model Selection

- **Description**: Specifies the model used for text generation.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4o' }
  ```

#### 2.1.2 Model Selection with Secret

- **Description**: Specifies the model along with the secret name to be used for authentication when accessing the model.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4o', 'secret_name': 'your_secret_name' }
  ```

### 2.2 Prompt Configuration

- **Parameter**: `prompt` or `prompt_name` with `context_columns`

  #### 2.2.1 Inline Prompt

  Directly provides the prompt with context columns.

  - **Example**:
    ```sql
    { 'prompt': 'Summarize the content: {article}', 'context_columns': [{'data': article_content, 'name': 'article'}] }
    ```

  #### 2.2.2 Named Prompt

  References a pre-configured prompt with context columns.

  - **Example**:
    ```json
    { 'prompt_name': 'summarize-content', 'context_columns': [{'data': article_content}] }
    ```

  #### 2.2.3 Named Prompt with Version

  References a specific version of a prompt with context columns.

  - **Example**:
    ```json
    { 'prompt_name': 'summarize-content', 'version': 2, 'context_columns': [{'data': article_content}] }
    ```

### 2.3 Context Columns Configuration

- **Parameter**: `context_columns` array
- **Description**: Specifies the columns from the table to be passed to the model as input. Each column can have three properties:
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

## 3. Output

The function returns a **BOOLEAN** value (`TRUE` or `FALSE`), indicating whether the row satisfies the condition specified in the prompt.

**Example Output**:  
For a prompt like _"Is this product description eco-friendly?"_:

- **Input Row**:
  - `product_description`: _"Made from 100% recyclable materials, this product is perfect for eco-conscious buyers."_
- **Output**:
  - `TRUE`
