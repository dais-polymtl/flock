import pytest
from integration.conftest import run_cli


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("llama3.2", "ollama")])
def model_config(request):
    """Fixture to test with different models."""
    return request.param


def test_llm_reduce_basic_functionality(integration_setup, model_config):
    """Test basic llm_reduce functionality without GROUP BY."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE products (
        id INTEGER,
        name VARCHAR,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO products
                        VALUES (1, 'Running Shoes',
                                'High-performance athletic footwear designed for comfort and speed'),
                               (2, 'Wireless Headphones',
                                'Premium quality bluetooth headphones with noise cancellation'),
                               (3, 'Smart Watch', 'Advanced fitness tracker with heart rate monitoring and GPS'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """'},
                    {'prompt': 'Summarize the following product descriptions into a single comprehensive summary', 'context_columns': [{'data': description}]}
        ) AS product_summary
            FROM products; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "product_summary" in result.stdout.lower()
    # Check that we got some meaningful output (not empty)
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 2, "Expected at least header and one result row"


def test_llm_reduce_with_group_by(integration_setup, model_config):
    """Test llm_reduce with GROUP BY clause."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-group_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE product_reviews (
        id INTEGER,
        product_category VARCHAR,
        review_text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO product_reviews
                        VALUES (1, 'Electronics', 'Great smartphone with excellent camera quality'),
                               (2, 'Electronics', 'Amazing tablet with long battery life'),
                               (3, 'Clothing', 'Comfortable and stylish jacket'),
                               (4, 'Clothing', 'Perfect fit jeans with premium material'),
                               (5, 'Books', 'Fascinating mystery novel with unexpected twists'),
                               (6, 'Books', 'Educational textbook with clear explanations'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT *
            FROM duckdb_secrets();
            SELECT product_category,
                   llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """'},
                    {'prompt': 'Create a brief summary of these product reviews', 'context_columns': [{'data': review_text}]}
        ) AS category_summary
            FROM product_reviews
            GROUP BY product_category
            ORDER BY product_category; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    # Should have header + 3 category groups (Books, Clothing, Electronics)
    assert len(lines) >= 4, f"Expected at least 4 lines, got {len(lines)}"
    assert (
        "electronics" in result.stdout.lower()
        or "clothing" in result.stdout.lower()
        or "books" in result.stdout.lower()
    )


def test_llm_reduce_multiple_columns(integration_setup, model_config):
    """Test llm_reduce with multiple input columns."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-multi_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE employee_feedback (
        id INTEGER,
        employee_name VARCHAR,
        department VARCHAR,
        feedback VARCHAR,
        rating INTEGER
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO employee_feedback
                        VALUES (1, 'John Doe', 'Engineering', 'Excellent technical skills and teamwork', 5),
                               (2, 'Jane Smith', 'Engineering', 'Great problem-solving abilities', 4),
                               (3, 'Bob Wilson', 'Engineering', 'Strong leadership and communication', 5); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT department,
                   llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """'},
                    {'prompt': 'Summarize the team feedback and overall performance', 'context_columns': [{'data': employee_name}, {'data': feedback}, {'data': rating::VARCHAR}]}
        ) AS team_summary
            FROM employee_feedback
            GROUP BY department; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "engineering" in result.stdout.lower()
    assert "team_summary" in result.stdout.lower()


def test_llm_reduce_with_batch_processing(integration_setup, model_config):
    """Test llm_reduce with batch size configuration."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-batch_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE articles (
        id INTEGER,
        title VARCHAR,
        content VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO articles
                        VALUES (1, 'AI Revolution', 'Artificial intelligence is transforming industries worldwide'),
                               (2, 'Climate Change', 'Global warming effects are becoming more evident each year'),
                               (3, 'Space Exploration', 'New discoveries in space are expanding our understanding'),
                               (4, 'Medical Advances', 'Breakthrough treatments are improving patient outcomes'),
                               (5, 'Technology Trends', 'Emerging technologies are reshaping our daily lives'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 2},
                    {'prompt': 'Create a comprehensive summary of these articles', 'context_columns': [{'data': title}, {'data': content}]}
        ) AS articles_summary
            FROM articles; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "articles_summary" in result.stdout.lower()
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 2, "Expected at least header and one result row"


def test_llm_reduce_with_model_parameters(integration_setup, model_config):
    """Test llm_reduce with custom model parameters."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-params_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE news_items (
        id INTEGER,
        headline VARCHAR,
        summary VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO news_items
                        VALUES (1, 'Tech Stock Surge', 'Technology stocks reached new highs this quarter'),
                               (2, 'Market Volatility', 'Financial markets showed increased volatility this week'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """', 'tuple_format': 'Markdown',
                                                            'model_parameters': '{"temperature": 0.1}'},
                    {'prompt': 'Provide a concise summary of these news items', 'context_columns': [{'data': headline}, {'data': summary}]}
        ) AS news_summary
            FROM news_items; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "news_summary" in result.stdout.lower()


def test_llm_reduce_empty_table(integration_setup, model_config):
    """Test llm_reduce behavior with empty table."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-empty_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE empty_data (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = (
        """
            SELECT llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """'},
                    {'prompt': 'Summarize the following text', 'context_columns': [{'data': text}]}
        ) AS summary
            FROM empty_data; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    # Should return empty result or no rows
    lines = result.stdout.strip().split("\n")
    assert len(lines) <= 2, "Expected at most header line for empty table"


def test_llm_reduce_error_handling_invalid_model(integration_setup):
    """Test error handling with non-existent model."""
    duckdb_cli_path, db_path = integration_setup

    create_table_query = """
    CREATE OR REPLACE TABLE test_data (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO test_data
                        VALUES (1, 'Test content'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT llm_reduce(
                       {'model_name': 'non-existent-model'},
        {'prompt': 'Summarize this', 'context_columns': [{'data': text}]}
    ) AS result
            FROM test_data; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert (
        result.returncode != 0
        or "error" in result.stderr.lower()
        or "Error" in result.stdout
    )


def test_llm_reduce_error_handling_empty_prompt(integration_setup, model_config):
    """Test error handling with empty prompt."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-empty-prompt_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE test_data (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO test_data
                        VALUES (1, 'Test content'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """'},
        {'prompt': '', 'context_columns': [{'data': text}]}
    ) AS result
            FROM test_data; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0


def test_llm_reduce_error_handling_missing_arguments(integration_setup, model_config):
    """Test error handling with missing required arguments."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-missing-args_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    # Test with only 1 argument (should fail since llm_reduce requires 2)
    query = (
        """
    SELECT llm_reduce(
        {'model_name': '"""
        + test_model_name
        + """'}
    ) AS result;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0, "Expected error for missing second argument"


def test_llm_reduce_with_special_characters(integration_setup, model_config):
    """Test llm_reduce with special characters and unicode."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-unicode_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE international_content (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO international_content
                        VALUES (1, 'Café résumé naïve - French terms'),
                               (2, 'Hello 世界 🌍 - Mixed scripts'),
                               (3, 'Price: $100.99 (50% off!) - Special symbols'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """'},
                    {'prompt': 'Summarize these international text samples', 'context_columns': [{'data': text}]}
        ) AS summary
            FROM international_content; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "summary" in result.stdout.lower()


def test_llm_reduce_with_structured_output(integration_setup, model_config):
    """Test llm_reduce with structured JSON output."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-structured_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE structured_data (
        id INTEGER,
        category VARCHAR,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO structured_data
                        VALUES (1, 'Technology', 'Latest smartphone releases'),
                               (2, 'Technology', 'AI developments in healthcare'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """', 'model_parameters': '{
                    "response_format": {
                        "type": "json_schema",
                        "json_schema": {
                            "name": "summary_response",
                            "schema": {
                                "type": "object",
                                "properties": {
                                    "summary": { 
                                        "type": "string"
                                    },
                                    "key_themes": {
                                        "type": "array",
                                        "items": {"type": "string"}
                                    }
                                },
                                "required": ["summary", "key_themes"],
                                "additionalProperties": false
                            }
                        },
                        "strict": true
                    }}' },
            {'prompt': 'Summarize these items and identify key themes.', 'context_columns': [{'data': category}, {'data': description}]}
        ) AS structured_summary
            FROM structured_data; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "structured_summary" in result.stdout.lower()


def _test_llm_reduce_performance_large_dataset(integration_setup, model_config):
    """Performance test with larger dataset - commented out with underscore prefix for optional execution."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reduce-perf_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE large_dataset AS
    SELECT
        i as id,
        'Category ' || (i % 5) as category,
        'Content item ' || i || ' with detailed description and information' as content
    FROM range(1, 51) t(i);
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = (
        """
            SELECT category,
                   llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 10},
                    {'prompt': 'Create a comprehensive summary of all items in this category', 'context_columns': [{'data': content}]}
        ) AS category_summary
            FROM large_dataset
            GROUP BY category
            ORDER BY category LIMIT 3; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 categories), got {len(lines)}"
    )
    assert "category" in result.stdout.lower()


def test_llm_reduce_with_image_integration(integration_setup, model_config):
    """Test llm_reduce with image data integration."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-image-reduce-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE animal_images (
        id INTEGER,
        name VARCHAR,
        image VARCHAR,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Insert data with Unsplash image URLs
    insert_data_query = """
                        INSERT INTO animal_images
                        VALUES (1, 'Lion', 'https://images.unsplash.com/photo-1549366021-9f761d450615?w=400',
                                'African lion in savanna'),
                               (2, 'Elephant', 'https://images.unsplash.com/photo-1557050543-4d5f4e07ef46?w=400',
                                'African elephant in nature'),
                               (3, 'Giraffe', 'https://images.unsplash.com/photo-1534567110243-8875d64ca8ff?w=400',
                                'Giraffe in the wild'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Summarize the next data in json do not miss any data',
                'context_columns': [
                    {'data': name},
                    {'data': image, 'type': 'image'}
                ]
            }
        ) AS animal_summary
            FROM animal_images; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "animal_summary" in result.stdout.lower()
    # Check that we got some meaningful output about the images
    assert len(result.stdout.strip().split("\n")) >= 2


def test_llm_reduce_image_with_group_by(integration_setup, model_config):
    """Test llm_reduce with images and GROUP BY clause."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-image-group-reduce_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE product_images (
        id INTEGER,
        product_name VARCHAR,
        image_url VARCHAR,
        category VARCHAR,
        price_range VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Insert data with Unsplash product image URLs
    insert_data_query = """
                        INSERT INTO product_images
                        VALUES (1, 'Modern Chair', 'https://images.unsplash.com/photo-1567538096630-e0c55bd6374c?w=400',
                                'Furniture', 'High'),
                               (2, 'Smartphone', 'https://images.unsplash.com/photo-1511707171634-5f897ff02aa9?w=400',
                                'Electronics', 'High'),
                               (3, 'Coffee Cup', 'https://images.unsplash.com/photo-1495474472287-4d71bcdd2085?w=400',
                                'Kitchenware', 'Low'),
                               (4, 'Laptop', 'https://images.unsplash.com/photo-1496181133206-80ce9b88a853?w=400',
                                'Electronics', 'High'),
                               (5, 'Table Lamp', 'https://images.unsplash.com/photo-1507473885765-e6ed057f782c?w=400',
                                'Furniture', 'Medium'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT category,
                   llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Analyze these product images in this category and provide a summary of their design characteristics and market positioning.',
                'context_columns': [
                    {'data': product_name},
                    {'data': image_url, 'type': 'image'},
                    {'data': price_range}
                ]
            }
        ) AS category_analysis
            FROM product_images
            GROUP BY category
            ORDER BY category; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 categories), got {len(lines)}"
    )
    assert "category_analysis" in result.stdout.lower()


def test_llm_reduce_image_batch_processing(integration_setup, model_config):
    """Test llm_reduce with multiple images in batch processing."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-image-batch-reduce_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE landscape_photos (
        id INTEGER,
        location VARCHAR,
        image_url VARCHAR,
        weather_condition VARCHAR,
        season VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Insert data with Unsplash landscape image URLs
    insert_data_query = """
                        INSERT INTO landscape_photos
                        VALUES (1, 'Mountain Peak',
                                'https://images.unsplash.com/photo-1506905925346-21bda4d32df4?w=400', 'Clear',
                                'Summer'),
                               (2, 'Forest Trail', 'https://images.unsplash.com/photo-1441974231531-c6227db76b6e?w=400',
                                'Overcast', 'Autumn'),
                               (3, 'Beach Sunset', 'https://images.unsplash.com/photo-1507525428034-b723cf961d3e?w=400',
                                'Clear', 'Summer'),
                               (4, 'Desert Dunes', 'https://images.unsplash.com/photo-1509316785289-025f5b846b35?w=400',
                                'Clear', 'Spring'),
                               (5, 'Lake View', 'https://images.unsplash.com/photo-1506905925346-21bda4d32df4?w=400',
                                'Partly Cloudy', 'Summer'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_reduce(
                       {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 3},
            {
                'prompt': 'Analyze these landscape photographs and create a comprehensive summary of the natural environments, weather conditions, and seasonal characteristics shown.',
                'context_columns': [
                    {'data': location},
                    {'data': image_url, 'type': 'image'},
                    {'data': weather_condition},
                    {'data': season}
                ]
            }
        ) AS landscape_summary
            FROM landscape_photos; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "landscape_summary" in result.stdout.lower()
    assert len(result.stdout.strip().split("\n")) >= 2
