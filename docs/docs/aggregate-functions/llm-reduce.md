---
title: llm_reduce
sidebar_position: 1
---

# llm_reduce Aggregate Function

The `llm_reduce` function in Flock consolidates multiple rows of text-based results into a single output. It is used
in SQL queries with the `GROUP BY` clause to combine multiple values into a summary or reduced form.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. **Usage Examples**

### 1.1. **Example without `GROUP BY`**

Summarize all product descriptions into one single result:

```sql
SELECT llm_reduce(
           {'model_name': 'gpt-4o'},
    {'prompt': 'Summarize the following product descriptions', 'context_columns': [{'data': product_description}]}
) AS product_summary
FROM UNNEST([
    'High-performance laptop with M2 chip and stunning Retina display',
    'Wireless earbuds with active noise cancellation and spatial audio',
    'Lightweight tablet perfect for creativity and productivity on the go'
]) AS t(product_description);
```

**Description**: This example aggregates all product descriptions into one summary. The `llm_reduce` function processes
the `product_description` column for each row, consolidating the values into a single summarized output.

### 1.2. **Example with `GROUP BY`**

Group the products by category and summarize their descriptions into one for each category:

```sql
SELECT category,
       llm_reduce(
           {'model_name': 'gpt-4o'},
           {'prompt': 'Summarize the following product descriptions', 'context_columns': [{'data': product_description}]}
       ) AS summarized_product_info
FROM VALUES
      ('Electronics', 'High-performance laptop with M2 chip and stunning display'),
      ('Electronics', 'Latest smartphone with advanced camera system and A17 Pro chip'),
      ('Accessories', 'Wireless mouse with multi-touch surface and rechargeable battery'),
      ('Accessories', 'Fast charging cable with USB-C connector and durable design')
    AS t(category, product_description)
GROUP BY category;
```

**Description**: This query groups the products by category (e.g., electronics, clothing) and summarizes all product
descriptions within each category into a single consolidated summary.

### 1.3. **Using a Named Prompt with `GROUP BY`**

Leverage a reusable named prompt for summarization, grouped by category:

```sql
SELECT category,
       llm_reduce(
           {'model_name': 'gpt-4o', 'secret_name': 'azure_key'},
           {'prompt_name': 'summarizer', 'version': 1, 'context_columns': [{'data': product_description}]}
       ) AS summarized_product_info
FROM VALUES
    ('Electronics', 'High-performance laptop with M2 chip and stunning display'),
    ('Electronics', 'Latest smartphone with advanced camera system'),
    ('Accessories', 'Wireless mouse with multi-touch surface'),
    ('Accessories', 'Fast charging cable with USB-C connector')
AS t(category, product_description)
GROUP BY category;
```

**Description**: This example uses a pre-configured named prompt (`summarizer`) with version `1` to summarize product
descriptions. The results are grouped by category, with one summary per category.

### 1.4. **Advanced Example with Multiple Columns and `GROUP BY`**

Summarize product details by category, using both the product name and description:

```sql
WITH sample_electronics AS (
    SELECT * FROM (
        VALUES
            ('Electronics', 'MacBook Pro', 'High-performance laptop with M2 chip and stunning Retina display'),
            ('Electronics', 'iPhone 15 Pro', 'Latest smartphone with titanium design and advanced camera system'),
            ('Electronics', 'iPad Air', 'Lightweight tablet with M1 chip perfect for creativity and productivity')
    ) AS t(category, product_name, product_description)
)
SELECT category,
       llm_reduce(
           {'model_name': 'gpt-4o'},
           {'prompt': 'Summarize the following product details', 'context_columns': [{'data': product_name}, {'data': product_description}]}
       ) AS detailed_summary
FROM sample_electronics
GROUP BY category;
```

**Description**: In this advanced example, the query summarizes both the `product_name` and `product_description`
columns for products in the "Electronics" category, generating a detailed summary for that category.

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

- **Description**: Specifies the model along with the secret name to be used for authentication when accessing the
  model.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4o', 'secret_name': 'your_secret_name' }
  ```

### 2.2. **Prompt Configuration**

Two types of prompts can be used:

1. **Inline Prompt**

    - Directly provides the prompt in the query.
    - **Example**:
      ```sql
      {'prompt': 'Summarize the following product descriptions'}
      ```

2. **Named Prompt**

    - Refers to a pre-configured prompt by name.
    - **Example**:
      ```sql
      {'prompt_name': 'summarizer'}
      ```

3. **Named Prompt with Version**
    - Refers to a specific version of a pre-configured prompt.
    - **Example**:
      ```sql
      {'prompt_name': 'summarizer', 'version': 1}
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

- **Column Type**: JSON.
- **Behavior**: The function consolidates multiple rows of text into a single output, summarizing or combining the
  provided data according to the model's response to the prompt.

**Output Example**:  
For a query that aggregates product descriptions, the result could look like:

- **Input Rows**:

    - `product_name`: _"Running Shoes"_
    - `product_name`: _"Wireless Headphones"_
    - `product_name`: _"Smart Watch"_

- **Output**:  
  `"A variety of products including running shoes, wireless headphones, and smart watches, each designed for comfort, convenience, and performance in their respective categories."`
