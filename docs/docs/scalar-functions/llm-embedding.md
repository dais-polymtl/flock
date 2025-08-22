---
title: llm_embedding
sidebar_position: 4
---

# llm_embedding Function

The `llm_embedding` function generates vector embeddings that represent the semantic meaning of text from specified table columns.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Simple Usage (without data)

### 1.1 Basic Embedding Generation

```sql
SELECT llm_embedding(
    {'model_name': 'text-embedding-3-small', 'secret_name': 'embedding_secret'},
    {'context_columns': [{'data': product_name}, {'data': product_description}]}
) AS product_embedding
FROM VALUES
    ('Wireless Headphones', 'Premium noise-cancelling headphones with 30-hour battery life'),
    ('Gaming Laptop', 'High-performance laptop with RTX graphics and 16GB RAM'),
    ('Smart Watch', 'Fitness tracker with heart rate monitor and GPS')
AS t(product_name, product_description);
```

**Description**: This example generates vector embeddings for each product, combining the `product_name` and `product_description` columns using the `text-embedding-3-small` model. The output is a semantic vector that represents the content of the product's name and description.

### 1.2 Similarity Search

```sql
WITH sample_products AS (
    SELECT *
    FROM (VALUES
        (1, 'Wireless Headphones', 'Premium noise-cancelling headphones with 30-hour battery life'),
        (2, 'Bluetooth Earbuds', 'Compact wireless earbuds with charging case'),
        (3, 'Gaming Laptop', 'High-performance laptop with RTX graphics and 16GB RAM'),
        (4, 'Office Laptop', 'Lightweight laptop perfect for business and productivity')
    ) AS t(product_id, product_name, product_description)
),
product_embeddings AS (
    SELECT
        product_id,
        product_name,
        llm_embedding(
            {'model_name': 'text-embedding-3-small'},
            {'context_columns': [{'data': product_name}, {'data': product_description}]}
        ) AS product_embedding
    FROM sample_products
)
SELECT
    a.product_name,
    b.product_name,
    array_cosine_similarity(a.product_embedding::DOUBLE[1536], b.product_embedding::DOUBLE[1536]) AS similarity
FROM product_embeddings a
JOIN product_embeddings b
ON a.product_id != b.product_id
WHERE similarity > 0.8;
```

**Description**: This example demonstrates how to use the vector embeddings for similarity search. It calculates the cosine similarity between embeddings of different products to find similar items based on their semantic meaning. Only product pairs with a similarity greater than `0.8` are included.

## 2. Input Parameters

The `llm_embedding` function accepts two primary inputs: model configuration and column mappings.

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

### 2.2 Context Columns Configuration

- **Parameter**: `context_columns` array
- **Description**: Specifies the text columns from the table to be passed to the model for embedding generation. Each column can have two properties:
  - `data`: The SQL column data (required)
  - `name`: Custom name for the column (optional)
- **Note**: The `llm_embedding` function currently supports only text data (no image support)
- **Example**:
  ```sql
  { 'context_columns': [{'data': product_name}, {'data': product_description}] }
  ```

## 3. Output

The function returns a **JSON array** containing floating-point numbers that represent the semantic vector of the input text.

**Example Output**:  
For a product with the description _"Wireless headphones with noise cancellation"_, the output might look like this:

```json
[0.342, -0.564, 0.123, ..., 0.789]
```

This array of floating-point numbers encodes the semantic meaning of the product description in high-dimensional space.
