---
title: llm_rerank
sidebar_position: 3
---

# llm_rerank Aggregate Function

The `llm_rerank` aggregate function implements **progressive reranking** using a sliding window strategy, as introduced by Xueguang Ma et al. (2023) in their paper [Zero-Shot Listwise Document Reranking with a Large Language Model](https://arxiv.org/abs/2305.02156). This approach addresses the input length limitations of transformer-based models, which can only process a fixed number of tokens at a time (e.g., `16,384` tokens for `gpt-4o`). When tasked with reranking a list of documents that exceeds the model's token limit, the function uses a sliding window mechanism to progressively rank subsets of documents.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## 1. **Function Overview**

`llm_rerank` is designed to rank a list of documents or rows based on the relevance to a given prompt. When the list length exceeds the model's input limit, the function uses a sliding window strategy to progressively rerank the documents:

### 1.1. **Window Size**

The model ranks a fixed number of documents (e.g., `m` documents) at a time.

### 1.2. **Sliding Window**

After ranking the last `m` documents, the window shifts towards the list's beginning by half its size (`m/2`) and repeats.

### 1.3. **Top-Ranking**

This ensures the most relevant documents reach the top quickly, enhancing the relevance of top results.

While this approach does not fully reorder the entire list, it is effective in improving the top-ranked results by iteratively ranking smaller subsets of documents.

## 2. **Usage Examples**

### 2.1. **Example without `GROUP BY`**

Rerank documents based on their relevance to a given query:

```sql
SELECT llm_rerank(
    {'model_name': 'gpt-4o'},
    {'prompt': 'AI and machine learning innovations', 'context_columns': [{'data': document_title}, {'data': document_content}]}
) AS reranked_documents
FROM VALUES
    ('Introduction to AI', 'This document covers the basics of artificial intelligence and its applications'),
    ('Machine Learning Fundamentals', 'Comprehensive guide to machine learning algorithms and techniques'),
    ('Advanced Neural Networks', 'Deep dive into neural network architectures and optimization'),
    ('Data Science Overview', 'General overview of data science methodologies and tools')
AS t(document_title, document_content);
```

**Description**: This query will return the documents ordered by relevance based on the provided prompt.

### 2.2. **Example with `GROUP BY`**

Rerank documents for each category based on their relevance:

```sql
SELECT category,
       llm_rerank(
           {'model_name': 'gpt-4o'},
           {'prompt': 'emerging technology trends', 'context_columns': [{'data': document_title}, {'data': document_content}]}
       ) AS reranked_documents
FROM VALUES
    ('AI', 'Machine Learning Basics', 'Introduction to supervised and unsupervised learning'),
    ('AI', 'Deep Learning Guide', 'Advanced neural networks and deep learning techniques'),
    ('AI', 'Computer Vision', 'Image processing and computer vision applications'),
    ('Blockchain', 'Cryptocurrency Overview', 'Understanding Bitcoin and blockchain technology'),
    ('Blockchain', 'Smart Contracts', 'Ethereum smart contracts and decentralized applications'),
    ('IoT', 'IoT Fundamentals', 'Internet of Things devices and connectivity')
AS documents(category, document_title, document_content)
GROUP BY category;
```

**Description**: In this case, the query groups documents by category and reranks them within each category based on relevance.

### 2.3. **Using a Named Prompt with `GROUP BY`**

Use a reusable prompt, such as "document-ranking", to rank documents based on relevance to a specific query:

```sql
SELECT category,
       llm_rerank(
           {'model_name': 'gpt-4o', 'secret_name': 'school_key'},
           {'prompt_name': 'document-ranking', 'version': 1, 'context_columns': [{'data': document_title}, {'data': document_content}]}
       ) AS reranked_documents
FROM (
    SELECT * FROM (
        VALUES
            ('Technology', 'Artificial Intelligence', 'AI applications in modern technology'),
            ('Technology', 'Cloud Computing', 'Scalable cloud infrastructure solutions'),
            ('Science', 'Quantum Physics', 'Quantum mechanics and its applications'),
            ('Science', 'Molecular Biology', 'DNA structure and genetic engineering')
    ) AS t(category, document_title, document_content)
)
GROUP BY category;
```

**Description**: This example leverages a named prompt (`document-ranking`) to rerank documents within each category.

### 2.4. **Advanced Example**

Use the `llm_rerank` function to rerank documents based on their content:

```sql
WITH sample_documents AS (
    SELECT * FROM (
        VALUES
            ('Advanced AI Techniques', 'Deep learning and neural network optimization'),
            ('Machine Learning Ethics', 'Ethical considerations in AI development'),
            ('Natural Language Processing', 'Text processing and language understanding'),
            ('Computer Vision Applications', 'Image recognition and visual processing')
    ) AS t(document_title, document_content)
)
SELECT llm_rerank(
           {'model_name': 'gpt-4o'},
           {'prompt': 'cutting-edge AI research', 'context_columns': [{'data': document_title}, {'data': document_content}]}
       ) AS reranked_documents
FROM sample_documents;
```

**Description**: This example will rerank the documents within the defined subset based on their relevance to cutting-edge AI research.

## 3. **Input Parameters**

### 3.1 **Model Configuration**

- **Parameter**: `model_name` and `secret_name`

#### 3.1.1 Model Selection

- **Description**: Specifies the model used for text generation.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4o' }
  ```

#### 3.1.2 Model Selection with Secret

- **Description**: Specifies the model along with the secret name to be used for authentication when accessing the model.
- **Example**:
  ```sql
  { 'model_name': 'gpt-4o', 'secret_name': 'your_secret_name' }
  ```

### 3.2. **Prompt Configuration**

Two types of prompts can be used:

1. **Inline Prompt**

   - Directly provides the prompt in the query.
   - **Example**:
     ```sql
     {'prompt': 'AI and machine learning innovations', 'context_columns': [{'data': document_title}, {'data': document_content}]}
     ```

2. **Named Prompt**

   - Refers to a pre-configured prompt by name.
   - **Example**:
     ```sql
     {'prompt_name': 'document-ranking'}
     ```

3. **Named Prompt with Version**
   - Refers to a specific version of a pre-configured prompt.
   - **Example**:
     ```sql
     {'prompt_name': 'document-ranking', 'version': 1}
     ```

### 3.3. **Context Columns Configuration**

- **Key**: `context_columns` array.
- **Purpose**: Maps table columns to provide input data for the model. Each column can have three properties:
  - `data`: The SQL column data (required)
  - `name`: Custom name for the column to be referenced in the prompt (optional)
  - `type`: Data type - "tabular" (default) or "image" (optional)
- **Example**:
  ```sql
  'context_columns': [
    {'data': document_title, 'name': 'title'},
    {'data': document_content},
    {'data': image_url, 'type': 'image'}
  ]
  ```

## 4. **Output**

- **Type**: JSON object.
- **Behavior**: The function returns a JSON object containing the reranked documents, ordered by relevance to the prompt. The structure of the returned JSON will mirror the input columns' values.

**Output Example**:  
For a query that reranks documents based on relevance, the result could look like:

- **Input Rows**:

  - `document_title`: _"Introduction to AI"_
  - `document_content`: _"This document covers the basics of artificial intelligence."_

- **Output JSON**:
  ```json
  [
    {
      "document_title": "Introduction to AI",
      "document_content": "This document covers the basics of artificial intelligence."
    },
    {
      "document_title": "Advanced AI Techniques",
      "document_content": "This document explores advanced topics in AI."
    }
  ]
  ```

The output contains the documents ordered by relevance based on the prompt.
