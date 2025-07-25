import pytest
from integration.conftest import run_cli


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("llama3.2", "ollama")])
def model_config(request):
    """Fixture to test with different models."""
    return request.param


def test_llm_complete_basic_functionality(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = f"CREATE MODEL('test-model', '{model_name}', '{provider}');"
    run_cli(duckdb_cli_path, db_path, create_model_query)

    query = """
    SELECT llm_complete(
        {'model_name': 'test-model'},
        {'prompt': 'What is 2+2?'}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert "4" in result.stdout or "four" in result.stdout.lower()
    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "result" in result.stdout.lower()


def test_llm_complete_with_input_columns(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-model-input', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE countries (
        id INTEGER,
        name VARCHAR,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO countries VALUES 
    (1, 'France', 'European country'),
    (2, 'Canada', 'North American country'),
    (3, 'Japan', 'Asian island nation');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        name,
        llm_complete(
            {'model_name': 'test-model-input'},
            {'prompt': 'What is the capital of {country}?'},
            {'country': name}
        ) AS capital
    FROM countries 
    WHERE id <= 2;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "paris" in result.stdout.lower()
    assert "ottawa" in result.stdout.lower()


def test_llm_complete_batch_processing(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-batch-model', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE product_reviews (
        id INTEGER,
        review_text VARCHAR,
        product_name VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO product_reviews VALUES 
    (1, 'This laptop is amazing! Fast and reliable.', 'Laptop Pro'),
    (2, 'The phone has poor battery life but good camera.', 'Smart Phone X'),
    (3, 'Great value for money. Highly recommended!', 'Budget Tablet');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        product_name,
        llm_complete(
            {'model_name': 'test-batch-model', 'batch_size': 2},
            {'prompt': 'Analyze the sentiment of this review: {review}. Respond with POSITIVE, NEGATIVE, or NEUTRAL.'},
            {'review': review_text}
        ) AS sentiment
    FROM product_reviews;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, f"Expected at least 4 lines, got {len(lines)}"
    assert (
        "positive" in result.stdout.lower()
        or "negative" in result.stdout.lower()
        or "neutral" in result.stdout.lower()
    )


def test_llm_complete_error_handling_invalid_model(integration_setup):
    duckdb_cli_path, db_path = integration_setup

    query = """
    SELECT llm_complete(
        {'model_name': 'non-existent-model'},
        {'prompt': 'Test prompt'}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert (
        result.returncode != 0
        or "error" in result.stderr.lower()
        or "Error" in result.stdout
    )


def test_llm_complete_error_handling_empty_prompt(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-empty-prompt', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    query = """
    SELECT llm_complete(
        {'model_name': 'test-empty-prompt'},
        {'prompt': ''}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0


def test_llm_complete_with_special_characters(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-unicode-model', '{model_name}', '{provider}');"
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
        llm_complete(
            {'model_name': 'test-unicode-model'},
            {'prompt': 'Translate this text to English if needed: {text}'},
            {'text': text}
        ) AS translation
    FROM special_text
    WHERE id = 1;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert lines[0] == "translation"


def test_llm_complete_with_model_params(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-params-model', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    query = """
    SELECT llm_complete(
        {'model_name': 'test-params-model', 'tuple_format': 'Markdown', 'batch_size': 1, 'model_parameters': '{"temperature": 0}'},
        {'prompt': 'Briefly, what is the capital of France?'}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "paris" in result.stdout.lower()


def test_llm_complete_with_structured_output_without_table(
    integration_setup, model_config
):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-structured-model', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    query = """
    SELECT llm_complete(
        {'model_name': 'test-structured-model', 
         'model_parameters': '{
            "response_format": {
                "type": "json_schema",
                "json_schema": {
                "name": "capital_finder",
                "schema": {
                    "type": "object",
                    "properties": {
                    "capital": { "type": "string" }
                    },
                    "required": ["capital"],
                    "additionalProperties": false
                }
                },
                "strict": true
            }}'
        },
        {'prompt': 'What is the capital of Canada?'}
    ) AS summary;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    response = result.stdout.strip().split("\n")[1].split(",")[0].lower()
    assert response == '"{""capital"":""ottawa""}"'


def test_llm_complete_with_structured_output_with_table(
    integration_setup, model_config
):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-structured-table-model', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE countries (
        id INTEGER,
        name VARCHAR,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO countries VALUES 
    (1, 'France', 'European country'),
    (2, 'Canada', 'North American country'),
    (3, 'Japan', 'Asian island nation');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT
        name,
        llm_complete(
            {'model_name': 'test-structured-table-model',
                'model_parameters': '{
                    "response_format": {
                        "type": "json_schema",
                        "json_schema": {
                            "name": "capital_finder",
                            "schema": {
                                "type": "object",
                                "properties": {
                                    "capital": { 
                                        "type": "string",
                                        "pattern": "^[A-Za-z]+$"
                                    }
                                },
                                "required": ["capital"],
                                "additionalProperties": false
                            }
                        },
                        "strict": true
                    }}'
            },
            {'prompt': 'What is the capital of each country?'},
            {'country': name}
        ) AS capital_info
    FROM countries
    WHERE id <= 2;
    """
    result = run_cli(duckdb_cli_path, db_path, query)
    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 3
    assert (
        '"{""capital"":""paris""}"' in result.stdout.lower()
        and '"{""capital"":""ottawa""}"' in result.stdout.lower()
    )


def _llm_complete_performance_large_dataset(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-perf-model', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE large_dataset AS
    SELECT
        i as id,
        'Item ' || i as item_name,
        'Description for item ' || i as description
    FROM range(1, 11) t(i);
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = """
    SELECT
        item_name,
        llm_complete(
            {'model_name': 'test-perf-model'},
            {'prompt': 'Create a short marketing slogan for: {item}'},
            {'item': item_name}
        ) AS slogan
    FROM large_dataset
    LIMIT 5;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 6, (
        f"Expected at least 6 lines (header + 5 data), got {len(lines)}"
    )
