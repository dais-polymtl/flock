---
title: llm_first
sidebar_position: 4
---

# llm_first Aggregate Function

The `llm_first` function is used to extract the first matching result that satisfies a condition defined by a model's prompt and column data. It operates across rows, typically combined with a `GROUP BY` clause, to return the first relevant row for each group.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. **Usage Examples**

### 1.1. **Example without `GROUP BY`**

Retrieve the first relevant product feature across all rows:

```sql
SELECT llm_first(
    {'model_name': 'gpt-4o'},
    {'prompt': 'high-performance computing', 'context_columns': [{'data': product_name}, {'data': product_description}]}
) AS first_product_feature
FROM VALUES
    ('MacBook Pro', 'High-performance laptop with M2 chip and Retina display'),
    ('AirPods Pro', 'Wireless earbuds with active noise cancellation'),
    ('iPad Air', 'Lightweight tablet perfect for creativity and productivity')
AS t(product_name, product_description);
```

**Description**: This query returns the first relevant feature from all product descriptions and product names, based on the provided prompt.

### 1.2. **Example with `GROUP BY`**

Retrieve the first relevant product feature for each product category:

```sql
SELECT category,
       llm_first(
           {'model_name': 'gpt-4o'},
           {'prompt': 'premium audio quality', 'context_columns': [{'data': product_name}, {'data': product_description}]}
       ) AS first_product_feature
FROM VALUES
    ('Electronics', 'Premium Headphones', 'High-quality wireless headphones with superior sound', 89.99),
    ('Electronics', 'Gaming Mouse', 'Precision gaming mouse with RGB lighting', 45.99),
    ('Electronics', 'Wireless Keyboard', 'Ergonomic wireless keyboard with backlight', 79.99),
    ('Books', 'Python Programming', 'Complete guide to Python programming language', 39.99),
    ('Books', 'Data Science Guide', 'Comprehensive data science methodology book', 49.99),
    ('Books', 'Machine Learning', 'Introduction to machine learning algorithms', 59.99)
AS products(category, product_name, product_description, price)
GROUP BY category;
```

**Description**: The query groups the products by category and returns the first relevant feature for each group.

### 1.3. **Using a Named Prompt with `GROUP BY`**

Use a reusable prompt, such as "first-relevant-detail", to extract the first relevant feature for each product category:

```sql
SELECT category,
       llm_first(
           {'model_name': 'gpt-4o', 'secret_name': 'product_key'},
           {'prompt_name': 'first-relevant-detail', 'version': 1, 'context_columns': [{'data': product_name}, {'data': product_description}]}
       ) AS first_product_feature
FROM (
    SELECT * FROM (
        VALUES
            ('Electronics', 'MacBook Pro', 'High-performance laptop with M2 chip'),
            ('Electronics', 'iPhone 15', 'Latest smartphone with advanced camera system'),
            ('Accessories', 'Magic Mouse', 'Wireless mouse with multi-touch surface'),
            ('Accessories', 'USB-C Cable', 'Fast charging cable for modern devices')
    ) AS t(category, product_name, product_description)
)
GROUP BY category;
```

**Description**: This example leverages a named prompt (`first-relevant-detail`) to extract the first relevant feature for each product category. The query groups the results by category.

### 1.4. **Advanced Example with Multiple Columns and `GROUP BY`**

Retrieve the first relevant feature for products grouped by category, using both the product name and description:

```sql
WITH sample_products AS (
    SELECT * FROM (
        VALUES
            ('Electronics', 'MacBook Pro', 'High-performance laptop with M2 chip'),
            ('Electronics', 'iPhone 15', 'Latest smartphone with advanced camera'),
            ('Electronics', 'iPad Air', 'Lightweight tablet for creativity')
    ) AS t(category, product_name, product_description)
)
SELECT category,
       llm_first(
           {'model_name': 'gpt-4o'},
           {'prompt': 'advanced camera technology', 'context_columns': [{'data': product_name}, {'data': product_description}]}
       ) AS first_product_feature
FROM sample_products
GROUP BY category;
```

**Description**: This query extracts the first relevant feature from both the `product_name` and `product_description` columns, grouped by product category (in this case, electronics).

## 2. **Input Parameters**

### 2.1 **Model Configuration**

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

### 2.2. **Prompt Configuration**

Two types of prompts can be used:

1. **Inline Prompt**

   - Directly provides the prompt in the query with context columns.
   - **Example**:
     ```sql
     {'prompt': 'premium audio quality', 'context_columns': [{'data': product_name}, {'data': product_description}]}
     ```

2. **Named Prompt**

   - Refers to a pre-configured prompt by name with context columns.
   - **Example**:
     ```sql
     {'prompt_name': 'first-relevant-detail', 'context_columns': [{'data': product_name}]}
     ```

3. **Named Prompt with Version**
   - Refers to a specific version of a pre-configured prompt with context columns.
   - **Example**:
     ```sql
     {'prompt_name': 'first-relevant-detail', 'version': 1, 'context_columns': [{'data': product_name}]}
     ```

### 2.3. **Context Columns Configuration**

- **Key**: `context_columns` array.
- **Purpose**: Maps table columns to provide input data for the model. Each column can have three properties:
  - `data`: The SQL column data (required)
  - `name`: Custom name for the column to be referenced in the prompt (optional)
  - `type`: Data type - "tabular" (default) or "image" (optional)
- **Example**:
  ```sql
  'context_columns': [
    {'data': product_name, 'name': 'product'},
    {'data': product_description},
    {'data': image_url, 'type': 'image'}
  ]
  ```

## 3. **Output**

- **Type**: JSON object.
- **Behavior**: The function returns a JSON object containing the values of the columns you provided in the input. The structure of the returned JSON will mirror the input columns' values.

**Output Example**:  
For a query that extracts the first relevant feature, the result could look like:

- **Input Rows**:

  - `product_name`: _"Wireless Headphones"_
  - `product_description`: _"High-quality wireless headphones with noise cancellation."_

- **Output JSON**:
  ```json
  {
    "product_name": "Wireless Headphones",
    "product_description": "High-quality wireless headphones with noise cancellation."
  }
  ```

The output contains the values for `product_name` and `product_description` from the first relevant row based on the prompt.
