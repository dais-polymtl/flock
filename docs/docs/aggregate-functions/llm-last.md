---
title: llm_last
sidebar_position: 5
---

# llm_last Function

The `llm_last` function is used to extract the least relevant result from a set of rows based on a model's prompt and input columns. It operates over a set of rows, generally combined with a `GROUP BY` clause, to return the least relevant row for each group.

## 1. **Usage Examples**

### 1.1. **Example without `GROUP BY`**

Retrieve the least relevant product feature across all rows:

```sql
SELECT llm_last(
    {'model_name': 'gpt-4'},
    {'prompt': 'What is the least relevant detail for these products, based on their names and descriptions?'},
    {'product_name': product_name, 'product_description': product_description}
) AS last_product_feature
FROM products;
```

This query will return the least relevant feature from all product descriptions and product names.

### 1.2. **Example with `GROUP BY`**

Retrieve the least relevant product feature for each product category:

```sql
SELECT category,
       llm_last(
           {'model_name': 'gpt-4'},
           {'prompt': 'What is the least relevant detail for these products, based on their names and descriptions?'},
           {'product_name': product_name, 'product_description': product_description}
       ) AS last_product_feature
FROM products
GROUP BY category;
```

In this case, the query groups products by category and returns the least relevant feature for each category.

### 1.3. **Using a Named Prompt with `GROUP BY`**

Use a reusable prompt such as "least-relevant-detail" to extract the least relevant feature for each product category:

```sql
SELECT category,
       llm_last(
           {'model_name': 'gpt-4', 'secret_name': 'my_key'},
           {'prompt_name': 'least-relevant-detail', 'version': 1},
           {'product_name': product_name, 'product_description': product_description}
       ) AS last_product_feature
FROM products
GROUP BY category;
```

If the `version` parameter is omitted, the system will use the latest version of the `least-relevant-detail` prompt by default.

### 1.4. **Advanced Example with Multiple Columns and `GROUP BY`**

Retrieve the least relevant feature for products grouped by category, using both the product name and description:

```sql
WITH product_info AS (
    SELECT category, product_name, product_description
    FROM products
    WHERE category = 'Electronics'
)
SELECT category,
       llm_last(
           {'model_name': 'gpt-4'},
           {'prompt': 'What is the least relevant detail for these products, based on their names and descriptions?'},
           {'product_name': product_name, 'product_description': product_description}
       ) AS last_product_feature
FROM product_info
GROUP BY category;
```

This example will extract the least relevant feature from both the product name and description for each product category.

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
     {'prompt': 'What is the least relevant detail for these products, based on their names and descriptions?'}
     ```

2. **Named Prompt**

   - Refers to a pre-configured prompt by name.
   - **Example**:
     ```sql
     {'prompt_name': 'least-relevant-detail'}
     ```

3. **Named Prompt with Version**
   - Refers to a specific version of a pre-configured prompt.
   - **Example**:
     ```sql
     {'prompt_name': 'least-relevant-detail', 'version': 1}
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
For a query that extracts the least relevant feature, the result could look like:

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

The output contains the values for `product_name` and `product_description` from the least relevant row based on the prompt.
