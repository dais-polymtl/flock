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

    create_model_query = (
        f"CREATE MODEL('test-reduce-model', '{model_name}', '{provider}');"
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
    INSERT INTO products VALUES 
    (1, 'Running Shoes', 'High-performance athletic footwear designed for comfort and speed'),
    (2, 'Wireless Headphones', 'Premium quality bluetooth headphones with noise cancellation'),
    (3, 'Smart Watch', 'Advanced fitness tracker with heart rate monitoring and GPS');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        llm_reduce(
            {'model_name': 'test-reduce-model'},
            {'prompt': 'Summarize the following product descriptions into a single comprehensive summary'},
            {'description': description}
        ) AS product_summary
    FROM products;
    """
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

    create_model_query = (
        f"CREATE MODEL('test-reduce-group', '{model_name}', '{provider}');"
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
    INSERT INTO product_reviews VALUES 
    (1, 'Electronics', 'Great smartphone with excellent camera quality'),
    (2, 'Electronics', 'Amazing tablet with long battery life'),
    (3, 'Clothing', 'Comfortable and stylish jacket'),
    (4, 'Clothing', 'Perfect fit jeans with premium material'),
    (5, 'Books', 'Fascinating mystery novel with unexpected twists'),
    (6, 'Books', 'Educational textbook with clear explanations');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT * FROM duckdb_secrets();
    SELECT 
        product_category,
        llm_reduce(
            {'model_name': 'test-reduce-group'},
            {'prompt': 'Create a brief summary of these product reviews'},
            {'review': review_text}
        ) AS category_summary
    FROM product_reviews 
    GROUP BY product_category
    ORDER BY product_category;
    """
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

    create_model_query = (
        f"CREATE MODEL('test-reduce-multi', '{model_name}', '{provider}');"
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
    INSERT INTO employee_feedback VALUES 
    (1, 'John Doe', 'Engineering', 'Excellent technical skills and teamwork', 5),
    (2, 'Jane Smith', 'Engineering', 'Great problem-solving abilities', 4),
    (3, 'Bob Wilson', 'Engineering', 'Strong leadership and communication', 5);
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        department,
        llm_reduce(
            {'model_name': 'test-reduce-multi'},
            {'prompt': 'Summarize the team feedback and overall performance'},
            {'name': employee_name, 'feedback': feedback, 'rating': rating}
        ) AS team_summary
    FROM employee_feedback 
    GROUP BY department;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "engineering" in result.stdout.lower()
    assert "team_summary" in result.stdout.lower()


def test_llm_reduce_with_batch_processing(integration_setup, model_config):
    """Test llm_reduce with batch size configuration."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-reduce-batch', '{model_name}', '{provider}');"
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
    INSERT INTO articles VALUES 
    (1, 'AI Revolution', 'Artificial intelligence is transforming industries worldwide'),
    (2, 'Climate Change', 'Global warming effects are becoming more evident each year'),
    (3, 'Space Exploration', 'New discoveries in space are expanding our understanding'),
    (4, 'Medical Advances', 'Breakthrough treatments are improving patient outcomes'),
    (5, 'Technology Trends', 'Emerging technologies are reshaping our daily lives');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        llm_reduce(
            {'model_name': 'test-reduce-batch', 'batch_size': 2},
            {'prompt': 'Create a comprehensive summary of these articles'},
            {'title': title, 'content': content}
        ) AS articles_summary
    FROM articles;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "articles_summary" in result.stdout.lower()
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 2, "Expected at least header and one result row"


def test_llm_reduce_with_model_parameters(integration_setup, model_config):
    """Test llm_reduce with custom model parameters."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-reduce-params', '{model_name}', '{provider}');"
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
    INSERT INTO news_items VALUES 
    (1, 'Tech Stock Surge', 'Technology stocks reached new highs this quarter'),
    (2, 'Market Volatility', 'Financial markets showed increased volatility this week');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        llm_reduce(
            {'model_name': 'test-reduce-params', 'tuple_format': 'Markdown', 'model_parameters': '{"temperature": 0.1}'},
            {'prompt': 'Provide a concise summary of these news items'},
            {'headline': headline, 'summary': summary}
        ) AS news_summary
    FROM news_items;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "news_summary" in result.stdout.lower()


def test_llm_reduce_empty_table(integration_setup, model_config):
    """Test llm_reduce behavior with empty table."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-reduce-empty', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE empty_data (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = """
    SELECT 
        llm_reduce(
            {'model_name': 'test-reduce-empty'},
            {'prompt': 'Summarize the following text'},
            {'text': text}
        ) AS summary
    FROM empty_data;
    """
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
    INSERT INTO test_data VALUES (1, 'Test content');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT llm_reduce(
        {'model_name': 'non-existent-model'},
        {'prompt': 'Summarize this'},
        {'text': text}
    ) AS result
    FROM test_data;
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

    create_model_query = (
        f"CREATE MODEL('test-reduce-empty-prompt', '{model_name}', '{provider}');"
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
    INSERT INTO test_data VALUES (1, 'Test content');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT llm_reduce(
        {'model_name': 'test-reduce-empty-prompt'},
        {'prompt': ''},
        {'text': text}
    ) AS result
    FROM test_data;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0


def test_llm_reduce_error_handling_missing_arguments(integration_setup, model_config):
    """Test error handling with missing required arguments."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-reduce-missing-args', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    # Test with only 2 arguments (should fail since llm_reduce requires 3)
    query = """
    SELECT llm_reduce(
        {'model_name': 'test-reduce-missing-args'},
        {'prompt': 'Test prompt'}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0, "Expected error for missing third argument"


def test_llm_reduce_with_special_characters(integration_setup, model_config):
    """Test llm_reduce with special characters and unicode."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-reduce-unicode', '{model_name}', '{provider}');"
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
    INSERT INTO international_content VALUES 
    (1, 'Café résumé naïve - French terms'),
    (2, 'Hello 世界 🌍 - Mixed scripts'),
    (3, 'Price: $100.99 (50% off!) - Special symbols');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        llm_reduce(
            {'model_name': 'test-reduce-unicode'},
            {'prompt': 'Summarize these international text samples'},
            {'text': text}
        ) AS summary
    FROM international_content;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "summary" in result.stdout.lower()


def test_llm_reduce_with_structured_output(integration_setup, model_config):
    """Test llm_reduce with structured JSON output."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-reduce-structured', '{model_name}', '{provider}');"
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
    INSERT INTO structured_data VALUES 
    (1, 'Technology', 'Latest smartphone releases'),
    (2, 'Technology', 'AI developments in healthcare');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT
        llm_reduce(
            {'model_name': 'test-reduce-structured',
                'model_parameters': '{
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
                    }}'
            },
            {'prompt': 'Summarize these items and identify key themes.'},
            {'category': category, 'description': description}
        ) AS structured_summary
    FROM structured_data;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "structured_summary" in result.stdout.lower()


def _test_llm_reduce_performance_large_dataset(integration_setup, model_config):
    """Performance test with larger dataset - commented out with underscore prefix for optional execution."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-reduce-perf', '{model_name}', '{provider}');"
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

    query = """
    SELECT
        category,
        llm_reduce(
            {'model_name': 'test-reduce-perf', 'batch_size': 10},
            {'prompt': 'Create a comprehensive summary of all items in this category'},
            {'content': content}
        ) AS category_summary
    FROM large_dataset
    GROUP BY category
    ORDER BY category
    LIMIT 3;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 categories), got {len(lines)}"
    )
    assert "category" in result.stdout.lower()
