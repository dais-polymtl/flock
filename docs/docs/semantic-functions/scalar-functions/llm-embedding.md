---
title: llm_embedding
sidebar_position: 4
---

# llm_embedding Function

The `llm_embedding` function generates vector embeddings that represent the semantic meaning of text from specified table columns.

# Table of Contents

Below is a quick overview to help you navigate with ease through the documentation. You can click on any of the links to jump to the relevant section.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. Simple Usage (without data)

### 1.1 Basic Embedding Generation

```sql
SELECT llm_embedding(
    {'model_name': 'text-embedding-3-small', 'secret_name': 'embedding_secret'}, 
    {'product_name': product_name, 'product_description': product_description}
) AS product_embedding
FROM products;
```

**Description**: This example generates vector embeddings for each product, combining the `product_name` and `product_description` columns using the `text-embedding-3-small` model. The output is a semantic vector that represents the content of the product's name and description.

### 1.2 Similarity Search

```sql
WITH product_embeddings AS (
    SELECT 
        product_id, 
        product_name, 
        llm_embedding(
            {'model_name': 'text-embedding-3-small'}, 
            {'product_name': product_name, 'product_description': product_description}
        ) AS product_embedding
    FROM products
)
SELECT 
    a.product_name, 
    b.product_name, 
    array_cosine_distance(a.product_embedding::DOUBLE[1536], b.product_embedding::DOUBLE[1536]) AS similarity
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
  { 'model_name': 'gpt-4' }
  ```

#### 2.1.2 Model Selection with Secret

- **Description**: Specifies the model along with the secret name to be used for authentication when accessing the model.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4', 'secret_name': 'your_secret_name' }
  ```

### 2.2 Column Mappings

- **Parameter**: Column mappings
- **Description**: Specifies the columns from the table to be passed to the model for embedding generation.
- **Example**:
  ```sql
  { 'product_name': product_name, 'product_description': product_description }
  ```

## 3. Output

The function returns a **JSON array** containing floating-point numbers that represent the semantic vector of the input text.

**Example Output**:  
For a product with the description *"Wireless headphones with noise cancellation"*, the output might look like this:

```json
[0.342, -0.564, 0.123, ..., 0.789]
```

This array of floating-point numbers encodes the semantic meaning of the product description in high-dimensional space.
