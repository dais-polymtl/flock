import pytest
from integration.conftest import run_cli


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("llama3.2", "ollama")])
def model_config(request):
    """Fixture to test with different models."""
    return request.param


def test_llm_filter_basic_functionality(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-filter-model', '{model_name}', '{provider}');"
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
    INSERT INTO test_data VALUES 
    (1, 'This is a positive statement'),
    (2, 'This is a negative statement');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        text,
        llm_filter(
            {'model_name': 'test-filter-model'},
                    {'prompt': 'Is this text positive? Answer true or false.', 'context_columns': [{'data': text}]}
        ) AS is_positive
    FROM test_data 
    WHERE id = 1;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "true" in result.stdout.lower() or "false" in result.stdout.lower()
    assert "is_positive" in result.stdout.lower()


def test_llm_filter_batch_processing(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-batch-filter', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE test_items (
        id INTEGER,
        text VARCHAR,
        category VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO test_items VALUES 
    (1, 'Apple iPhone', 'Technology'),
    (2, 'War and Peace', 'Literature'),
    (3, 'Tesla Model S', 'Technology'),
    (4, 'The Great Gatsby', 'Literature'),
    (5, 'Samsung Galaxy', 'Technology');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        text,
        category,
        llm_filter(
            {'model_name': 'test-batch-filter', 'batch_size': 2},
                    {'prompt': 'Is this item technology-related? Answer true or false.', 'context_columns': [{'data': text}]}
        ) AS is_tech
    FROM test_items;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 6, f"Expected at least 6 lines, got {len(lines)}"
    assert "true" in result.stdout.lower() and "false" in result.stdout.lower()


def test_llm_filter_error_handling_invalid_model(integration_setup):
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
    SELECT llm_filter(
        {'model_name': 'non-existent-model'},
        {'prompt': 'Test prompt', 'context_columns': [{'data': text}]}
    ) AS result
    FROM test_data;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert (
        result.returncode != 0
        or "error" in result.stderr.lower()
        or "Error" in result.stdout
    )


def test_llm_filter_error_handling_empty_prompt(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-empty-filter', '{model_name}', '{provider}');"
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
    SELECT llm_filter(
        {'model_name': 'test-empty-filter'},
        {'prompt': '', 'context_columns': [{'data': text}]}
    ) AS result
    FROM test_data;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0


def test_llm_filter_with_special_characters(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-unicode-filter', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE special_text (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO special_text VALUES 
    (1, 'Café résumé naïve'),
    (2, 'Price: $100.99 (50% off!)'),
    (3, 'Hello 世界 🌍');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        text,
        llm_filter(
            {'model_name': 'test-unicode-filter'},
                    {'prompt': 'Does this text contain non-ASCII characters? Answer true or false.', 'context_columns': [{'data': text}]}
        ) AS has_unicode
    FROM special_text
    WHERE id = 1;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "true" in result.stdout.lower() or "false" in result.stdout.lower()


def test_llm_filter_with_model_params(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-params-filter', '{model_name}', '{provider}');"
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
    INSERT INTO test_data VALUES 
    (1, 'Excellent quality product');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        text,
        llm_filter(
            {'model_name': 'test-params-filter', 'tuple_format': 'Markdown', 'batch_size': 1, 'model_parameters': '{"temperature": 0}'},
                    {'prompt': 'Is this text expressing positive sentiment? Answer true or false only.', 'context_columns': [{'data': text}]}
        ) AS is_positive
    FROM test_data;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "true" in result.stdout.lower() or "false" in result.stdout.lower()


def test_llm_filter_with_structured_output(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-structured-filter', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE items (
        id INTEGER,
        name VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO items VALUES 
    (1, 'Smartphone'),
    (2, 'Novel');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT
        name,
        llm_filter(
            {'model_name': 'test-structured-filter',
                'model_parameters': '{
                    "response_format": {
                        "type": "json_schema",
                        "json_schema": {
                            "name": "filter_response",
                            "schema": {
                                "type": "object",
                                "properties": {
                                    "result": { 
                                        "type": "boolean"
                                    }
                                },
                                "required": ["result"],
                                "additionalProperties": false
                            }
                        },
                        "strict": true
                    }}'
            },
                    {'prompt': 'Is this item an electronic device? Respond with a boolean result.', 'context_columns': [{'data': name}]}
        ) AS is_electronic
    FROM items
    WHERE id <= 2;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 3


def test_llm_filter_error_handling_missing_arguments(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-missing-args', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    # Test with only 1 argument (should fail since llm_filter requires 2)
    query = """
    SELECT llm_filter(
        {'model_name': 'test-missing-args'}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0, "Expected error for missing second argument"


def _test_llm_filter_performance_large_dataset(integration_setup, model_config):
    """Performance test - commented out with underscore prefix for optional execution"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-perf-filter', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE large_content AS
    SELECT
        i as id,
        'Content item ' || i || ' with some text to analyze' as content
    FROM range(1, 21) t(i);
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = """
    SELECT
        content,
        llm_filter(
            {'model_name': 'test-perf-filter', 'batch_size': 5},
                    {'prompt': 'Does this content contain the word "item"? Answer true or false.', 'context_columns': [{'data': content}]}
        ) AS filter_result
    FROM large_content
    LIMIT 10;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 11, (
        f"Expected at least 11 lines (header + 10 data), got {len(lines)}"
    )
    assert "true" in result.stdout.lower() or "false" in result.stdout.lower()
