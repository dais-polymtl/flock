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
    {'model_name': 'gpt-4'},
    {'prompt': 'What is the most relevant detail for these products, based on their names and descriptions?'},
    {'product_name': product_name, 'product_description': product_description}
) AS first_product_feature
FROM products;
```

**Description**: This query returns the first relevant feature from all product descriptions and product names, based on the provided prompt.

### 1.2. **Example with `GROUP BY`**

Retrieve the first relevant product feature for each product category:

```sql
SELECT category,
       llm_first(
           {'model_name': 'gpt-4'},
           {'prompt': 'What is the most relevant detail for these products, based on their names and descriptions?'},
           {'product_name': product_name, 'product_description': product_description}
       ) AS first_product_feature
FROM products
GROUP BY category;
```

**Description**: The query groups the products by category and returns the first relevant feature for each group.

### 1.3. **Using a Named Prompt with `GROUP BY`**

Use a reusable prompt, such as "first-relevant-detail", to extract the first relevant feature for each product category:

```sql
SELECT category,
       llm_first(
           {'model_name': 'gpt-4', 'secret_name': 'product_key'},
           {'prompt_name': 'first-relevant-detail', 'version': 1},
           {'product_name': product_name, 'product_description': product_description}
       ) AS first_product_feature
FROM products
GROUP BY category;
```

**Description**: This example leverages a named prompt (`first-relevant-detail`) to extract the first relevant feature for each product category. The query groups the results by category.

### 1.4. **Advanced Example with Multiple Columns and `GROUP BY`**

Retrieve the first relevant feature for products grouped by category, using both the product name and description:

```sql
WITH product_info AS (
    SELECT category, product_name, product_description
    FROM products
    WHERE category = 'Electronics'
)
SELECT category,
       llm_first(
           {'model_name': 'gpt-4'},
           {'prompt': 'What is the most relevant detail for these products, based on their names and descriptions?'},
           {'product_name': product_name, 'product_description': product_description}
       ) AS first_product_feature
FROM product_info
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
  { 'model_name': 'gpt-4' }
  ```

#### 2.1.2 Model Selection with Secret

- **Description**: Specifies the model along with the secret name to be used for authentication when accessing the model.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4', 'secret_name': 'your_secret_name' }
  ```

### 2.2. **Prompt Configuration**

Two types of prompts can be used:

1. **Inline Prompt**

   - Directly provides the prompt in the query.
   - **Example**:
     ```sql
     {'prompt': 'What is the most relevant detail for these products, based on their names and descriptions?'}
     ```

2. **Named Prompt**

   - Refers to a pre-configured prompt by name.
   - **Example**:
     ```sql
     {'prompt_name': 'first-relevant-detail'}
     ```

3. **Named Prompt with Version**
   - Refers to a specific version of a pre-configured prompt.
   - **Example**:
     ```sql
     {'prompt_name': 'first-relevant-detail', 'version': 1}
     ```

### 2.3. **Column Mappings (Optional)**

- **Key**: Column mappings.
- **Purpose**: Maps table columns to prompt variables for input.
- **Example**:
  ```sql
  {'product_name': product_name, 'product_description': product_description}
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
