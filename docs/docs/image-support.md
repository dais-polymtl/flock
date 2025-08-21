---
title: Image Support
sidebar_position: 6
---

# Image Support in FlockMTL

FlockMTL provides comprehensive support for working with images in your SQL queries through its LLM functions. You can analyze, describe, filter, and process images alongside traditional tabular data, unlocking powerful multimodal AI capabilities directly within your database.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Overview

FlockMTL's image support allows you to:

- Analyze and describe image content
- Filter records based on visual criteria
- Generate embeddings for image similarity search
- Combine image and text data in a single query
- Process images from URLs or file paths

## Supported Image Formats

FlockMTL supports images in various formats:

- **JPEG** (.jpg, .jpeg)
- **PNG** (.png)
- **GIF** (.gif)
- **WebP** (.webp)
- **BMP** (.bmp)

Images can be provided in different formats depending on your model provider:

### OpenAI Vision Models

- **HTTP/HTTPS URLs** pointing to publicly accessible images
- **Base64 encoded strings** (for inline image data)

### Ollama Vision

- **Base64 encoded strings** (for inline image data)

### Model-Specific Examples

**OpenAI with URL:**

```sql
'context_columns': [
  {'data': 'https://example.com/image.jpg', 'type': 'image'}
]
```

**Ollama with Base64:**

```sql
'context_columns': [
  {'data': '/9j/4AAQSkZJRgABAQAAAQ...', 'type': 'image'}
]
```

**OpenAI with Base64:**

```sql
'context_columns': [
  {'data': '/9j/4AAQSkZJRgABAQAAAQ...', 'type': 'image'}
]
```

## Using Images in Context Columns

To use images in FlockMTL functions, specify the column type as `'image'` in the `context_columns` array:

```sql
'context_columns': [
  {'data': column_name, 'type': 'image'}
]
```

### Basic Structure

Each context column can have three properties:

- **`data`** _(required)_: The SQL column containing image data (URL, path, or base64)
- **`name`** _(optional)_: Custom name to reference this image in your prompt
- **`type`** _(optional)_: Set to `'image'` for image data (default is `'tabular'`)
- **`detail`** _(optional)_: Image detail level for OpenAI models - `'low'`, `'medium'`, or `'high'` (default is `'low'`)

### Image Detail Parameter (OpenAI Only)

For OpenAI models, you can control the level of detail in image processing using the `detail` parameter:

- **`'low'`** _(default)_: Uses fewer tokens, faster processing, lower cost
- **`'medium'`**: Balanced processing with moderate token usage
- **`'high'`**: Maximum detail analysis, uses more tokens, higher cost

```sql
-- Example with different detail levels
'context_columns': [
  {'data': image_url, 'type': 'image', 'detail': 'low'},    -- Fast, economical
  {'data': image_url, 'type': 'image', 'detail': 'medium'}, -- Balanced
  {'data': image_url, 'type': 'image', 'detail': 'high'}   -- Maximum detail
]
```

**Note**: The `detail` parameter only works with OpenAI vision models and is ignored by other providers.

### Provider-Specific Examples

**OpenAI Model with URL:**

```sql
-- Using OpenAI GPT-4o with image URLs
SELECT
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'Describe what you see in this image.',
            'context_columns': [
                {'data': 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=400', 'type': 'image'}
            ]
        }
    ) AS description;
```

**Ollama Model with Base64:**

```sql
-- Using Ollama LLaVA with base64-encoded images
SELECT
    llm_complete(
        {'model_name': 'llava'},
        {
            'prompt': 'Describe what you see in this image.',
            'context_columns': [
                {'data': '/9j/4AAQSkZJRgABAQAAAQABAAD...', 'type': 'image'}
            ]
        }
    ) AS description;
```

## Scalar Functions with Images

### llm_complete with Images

Generate descriptions, captions, or analyze image content:

```sql
-- Basic image description with default low detail
SELECT
    product_name,
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'Describe this product image in detail.',
            'context_columns': [
                {'data': image_url, 'type': 'image'}
            ]
        }
    ) AS image_description
FROM VALUES
    ('Wireless Headphones', 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=400'),
    ('Gaming Laptop', 'https://images.unsplash.com/photo-1496181133206-80ce9b88a853?w=400'),
    ('Smart Watch', 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=400')
AS t(product_name, image_url);
```

### Image Detail Levels Example

```sql
-- Compare different detail levels for image analysis
SELECT
    product_name,
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'Analyze this product image and describe its key features, design elements, and quality indicators.',
            'context_columns': [
                {'data': image_url, 'type': 'image', 'detail': 'low'}
            ]
        }
    ) AS low_detail_analysis,
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'Analyze this product image and describe its key features, design elements, and quality indicators.',
            'context_columns': [
                {'data': image_url, 'type': 'image', 'detail': 'high'}
            ]
        }
    ) AS high_detail_analysis
FROM VALUES
    ('Premium Camera', 'https://images.unsplash.com/photo-1502920917128-1aa500764cbd?w=400')
AS t(product_name, image_url);
```

### Combining Images with Text Data

```sql
-- Analyze product images with context
SELECT
    product_name,
    category,
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'Based on this {category} product image and its name {product}, write a marketing description.',
            'context_columns': [
                {'data': product_name, 'name': 'product'},
                {'data': category, 'name': 'category'},
                {'data': image_url, 'type': 'image'}
            ]
        }
    ) AS marketing_copy
FROM VALUES
    ('Wireless Headphones', 'Electronics', 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=400'),
    ('Coffee Mug', 'Kitchen', 'https://images.unsplash.com/photo-1495474472287-4d71bcdd2085?w=400'),
    ('Running Shoes', 'Sports', 'https://images.unsplash.com/photo-1542291026-7eec264c27ff?w=400')
AS t(product_name, category, image_url);
```

### llm_filter with Images

Filter records based on visual criteria:

```sql
-- Filter images showing outdoor scenes
SELECT *
FROM VALUES
    (1, 'Mountain Landscape', 'https://images.unsplash.com/photo-1506905925346-21bda4d32df4?w=400'),
    (2, 'City Street', 'https://images.unsplash.com/photo-1477959858617-67f85cf4f1df?w=400'),
    (3, 'Beach Sunset', 'https://images.unsplash.com/photo-1507525428034-b723cf961d3e?w=400')
AS t(photo_id, photo_title, photo_url)
WHERE llm_filter(
    {'model_name': 'gpt-4o'},
    {
        'prompt': 'Is this an outdoor landscape photograph?',
        'context_columns': [
            {'data': photo_url, 'type': 'image'}
        ]
    }
);
```

### Advanced Image Filtering

```sql
-- Filter products with professional product photography
SELECT
    product_id,
    product_name,
    image_url,
    price
FROM VALUES
    (1, 'Premium Headphones', 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=400', 150.00),
    (2, 'Gaming Mouse', 'https://images.unsplash.com/photo-1527814050087-3793815479db?w=400', 75.00),
    (3, 'Wireless Keyboard', 'https://images.unsplash.com/photo-1587829741301-dc798b83add3?w=400', 120.00),
    (4, 'Studio Monitor', 'https://images.unsplash.com/photo-1545127398-14699f92334b?w=400', 200.00)
AS t(product_id, product_name, image_url, price)
WHERE llm_filter(
    {'model_name': 'gpt-4o'},
    {
        'prompt': 'Is this a high-quality, professional product photo with good lighting and composition?',
        'context_columns': [
            {'data': image_url, 'type': 'image'},
            {'data': product_name}
        ]
    }
)
AND price > 100;
```

### llm_embedding with Images

**Note**: The `llm_embedding` function currently supports **only text data** and **does not support image inputs**. However, you can create a powerful workflow by first using `llm_complete` to generate descriptions from images, then creating embeddings from those descriptions.

#### Generating Image Descriptions and Embeddings

```sql
-- Step 1: Generate descriptions from images, then create embeddings
WITH image_descriptions AS (
    SELECT
        image_id,
        filename,
        image_url,
        llm_complete(
            {'model_name': 'gpt-4o'},
            {
                'prompt': 'Provide a detailed description of this image, including objects, colors, composition, mood, and any text visible in the image.',
                'context_columns': [
                    {'data': image_url, 'type': 'image'}
                ]
            }
        ) AS generated_description
    FROM VALUES
        (1, 'sunset_beach.jpg', 'https://images.unsplash.com/photo-1507525428034-b723cf961d3e?w=400'),
        (2, 'city_skyline.jpg', 'https://images.unsplash.com/photo-1477959858617-67f85cf4f1df?w=400'),
        (3, 'forest_path.jpg', 'https://images.unsplash.com/photo-1441974231531-c6227db76b6e?w=400')
    AS t(image_id, filename, image_url)
),
image_embeddings AS (
    SELECT
        image_id,
        filename,
        image_url,
        generated_description,
        llm_embedding(
            {'model_name': 'text-embedding-3-small'},
            {
                'context_columns': [
                    {'data': generated_description}
                ]
            }
        ) AS description_embedding
    FROM image_descriptions
)
SELECT * FROM image_embeddings;
```

## Aggregate Functions with Images

### llm_first with Images

Find the first/best image based on criteria:

```sql
-- Find the best product image for each category
SELECT
    category,
    llm_first(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'Which product has the most appealing and professional product image?',
            'context_columns': [
                {'data': product_name},
                {'data': image_url, 'type': 'image'},
                {'data': price::VARCHAR}
            ]
        }
    ) AS best_product_image
FROM VALUES
    ('Electronics', 'Wireless Headphones', 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=400', 89.99),
    ('Electronics', 'Gaming Mouse', 'https://images.unsplash.com/photo-1527814050087-3793815479db?w=400', 45.99),
    ('Electronics', 'Wireless Keyboard', 'https://images.unsplash.com/photo-1587829741301-dc798b83add3?w=400', 79.99),
    ('Kitchen', 'Coffee Maker', 'https://images.unsplash.com/photo-1495474472287-4d71bcdd2085?w=400', 129.99),
    ('Kitchen', 'Blender', 'https://images.unsplash.com/photo-1570197788417-0e82375c9371?w=400', 99.99)
AS t(category, product_name, image_url, price)
GROUP BY category;
```

### llm_reduce with Images

Summarize or analyze multiple images:

```sql
-- Analyze a collection of images for common themes
SELECT
    album_id,
    llm_reduce(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'What are the common visual themes and subjects across these images?',
            'context_columns': [
                {'data': image_url, 'type': 'image'},
                {'data': image_caption}
            ]
        }
    ) AS album_summary
FROM VALUES
    (1, 'https://images.unsplash.com/photo-1506905925346-21bda4d32df4?w=400', 'Mountain landscape at sunrise'),
    (1, 'https://images.unsplash.com/photo-1441974231531-c6227db76b6e?w=400', 'Forest hiking trail'),
    (1, 'https://images.unsplash.com/photo-1447752875215-b2761acb3c5d?w=400', 'Alpine lake reflection'),
    (2, 'https://images.unsplash.com/photo-1477959858617-67f85cf4f1df?w=400', 'City street at night'),
    (2, 'https://images.unsplash.com/photo-1449824913935-59a10b8d2000?w=400', 'Urban architecture'),
    (2, 'https://images.unsplash.com/photo-1513656428746-66aea1af5590?w=400', 'Downtown skyline')
AS t(album_id, image_url, image_caption)
GROUP BY album_id;
```

## Practical Examples

### E-commerce Product Analysis

```sql
-- Analyze product images for quality and appeal
WITH image_analysis AS (
    SELECT
        product_id,
        product_name,
        category,
        price,
        llm_complete(
            {'model_name': 'gpt-4o'},
            {
                'prompt': 'Rate this product image quality (1-10) and explain what makes it appealing or not. Consider lighting, composition, and product presentation.',
                'context_columns': [
                    {'data': product_name},
                    {'data': image_url, 'type': 'image'}
                ]
            }
        ) AS quality_assessment,
        llm_filter(
            {'model_name': 'gpt-4o'},
            {
                'prompt': 'Does this image show the product clearly with professional lighting?',
                'context_columns': [
                    {'data': image_url, 'type': 'image'}
                ]
            }
        ) AS is_professional
    FROM VALUES
        (1, 'Premium Headphones', 'Electronics', 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=400', 150.00),
        (2, 'Gaming Mouse', 'Electronics', 'https://images.unsplash.com/photo-1527814050087-3793815479db?w=400', 75.00),
        (3, 'Coffee Maker', 'Kitchen', 'https://images.unsplash.com/photo-1495474472287-4d71bcdd2085?w=400', 129.99),
        (4, 'Running Shoes', 'Sports', 'https://images.unsplash.com/photo-1542291026-7eec264c27ff?w=400', 89.99)
    AS t(product_id, product_name, category, image_url, price)
)
SELECT * FROM image_analysis
WHERE is_professional = true
ORDER BY price DESC;
```

### Content Moderation

```sql
-- Filter inappropriate content from user uploads
SELECT
    upload_id,
    user_id,
    upload_time,
    image_url
FROM VALUES
    (1, 'user123', '2025-08-19 10:30:00', 'https://images.unsplash.com/photo-1506905925346-21bda4d32df4?w=400'),
    (2, 'user456', '2025-08-19 11:15:00', 'https://images.unsplash.com/photo-1441974231531-c6227db76b6e?w=400'),
    (3, 'user789', '2025-08-19 12:00:00', 'https://images.unsplash.com/photo-1507525428034-b723cf961d3e?w=400')
AS t(upload_id, user_id, upload_time, image_url)
WHERE llm_filter(
    {'model_name': 'gpt-4o'},
    {
        'prompt': 'Is this image appropriate for all audiences? Does it contain any violence, adult content, or offensive material?',
        'context_columns': [
            {'data': image_url, 'type': 'image'}
        ]
    }
);
```

### Visual Inventory Management

```sql
-- Categorize products based on visual appearance
SELECT
    product_id,
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'What category would this product belong to based on its appearance? Choose from: Electronics, Clothing, Home & Garden, Sports, Automotive.',
            'context_columns': [
                {'data': product_image, 'type': 'image'}
            ]
        }
    ) AS predicted_category,
    actual_category
FROM VALUES
    (1, 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=400', 'Electronics'),
    (2, 'https://images.unsplash.com/photo-1542291026-7eec264c27ff?w=400', 'Sports'),
    (3, 'https://images.unsplash.com/photo-1495474472287-4d71bcdd2085?w=400', 'Home & Garden'),
    (4, 'https://images.unsplash.com/photo-1527814050087-3793815479db?w=400', 'Electronics')
AS t(product_id, product_image, actual_category);
```

## Performance Considerations

### Batch Processing

Use batch processing for better performance when analyzing multiple images:

```sql
SELECT
    image_id,
    llm_complete(
        {
            'model_name': 'gpt-4o',
            'batch_size': 5  -- Process 5 images at once
        },
        {
            'prompt': 'Describe this image briefly.',
            'context_columns': [
                {'data': image_url, 'type': 'image'}
            ]
        }
    ) AS description
FROM VALUES
    (1, 'https://images.unsplash.com/photo-1506905925346-21bda4d32df4?w=400'),
    (2, 'https://images.unsplash.com/photo-1441974231531-c6227db76b6e?w=400'),
    (3, 'https://images.unsplash.com/photo-1507525428034-b723cf961d3e?w=400'),
    (4, 'https://images.unsplash.com/photo-1477959858617-67f85cf4f1df?w=400'),
    (5, 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=400'),
    (6, 'https://images.unsplash.com/photo-1542291026-7eec264c27ff?w=400')
AS t(image_id, image_url)
LIMIT 100;
```

### Token Optimization (OpenAI Models)

Control token usage and costs with the `detail` parameter:

```sql
-- Cost-effective analysis with low detail (default)
SELECT
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'What type of product is this?',
            'context_columns': [
                {'data': image_url, 'type': 'image'}  -- Uses 'low' detail by default
            ]
        }
    ) AS product_type
FROM product_images;

-- Detailed analysis when precision is needed
SELECT
    llm_complete(
        {'model_name': 'gpt-4o'},
        {
            'prompt': 'Perform detailed quality control analysis of this product image.',
            'context_columns': [
                {'data': image_url, 'type': 'image', 'detail': 'high'}  -- More tokens, higher accuracy
            ]
        }
    ) AS quality_analysis
FROM critical_product_images;
```

## FlockMTL Function Support

FlockMTL's image support is available in the following functions:

| Function        | Image Support | Description                                 |
| --------------- | ------------- | ------------------------------------------- |
| `llm_complete`  | ✅ Full       | Generate text based on image content        |
| `llm_filter`    | ✅ Full       | Filter records based on visual criteria     |
| `llm_embedding` | ❌ Text only  | Currently supports only text data           |
| `llm_reduce`    | ✅ Full       | Aggregate operations on image collections   |
| `llm_rerank`    | ✅ Full       | Rank items based on visual relevance        |
| `llm_first`     | ✅ Full       | Select top item based on visual criteria    |
| `llm_last`      | ✅ Full       | Select bottom item based on visual criteria |

This comprehensive image support in FlockMTL opens up countless possibilities for multimodal AI applications directly within your SQL workflows.
