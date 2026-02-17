import pytest
from integration.conftest import (
    run_cli,
    get_image_data_for_provider,
    get_audio_file_path,
)

# Expected keywords that should appear when audio is transcribed
# Audio content: "Flock transforms DuckDB into a hybrid database and a semantic AI engine"
AUDIO_EXPECTED_KEYWORDS = ["flock", "duckdb", "database", "semantic", "ai", "hybrid"]


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("gemma3:1b", "ollama")])
def model_config(request):
    """Fixture to test with different models for text-only tests."""
    return request.param


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("gemma3:4b", "ollama")])
def model_config_image(request):
    """Fixture to test with different models for image tests."""
    return request.param


def test_llm_filter_basic_functionality(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-filter-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE test_data (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO test_data
                        VALUES (1, 'This is a positive statement'),
                               (2, 'This is a negative statement'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT 
            text,
            llm_filter(
                {'model_name': '"""
        + test_model_name
        + """'},
                    {'prompt': 'Is this text positive?', 'context_columns': [{'data': text}]}
        ) AS is_positive
    FROM test_data 
    WHERE id = 1;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "true" in result.stdout.lower() or "false" in result.stdout.lower()
    assert "is_positive" in result.stdout.lower()


def test_llm_filter_without_context_columns(integration_setup, model_config):
    """Test llm_filter without context_columns parameter."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-filter-no-context_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
        """
        SELECT llm_filter(
            {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Is paris the best capital in the world?'}
        ) AS filter_result;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "true" in result.stdout.lower() or "false" in result.stdout.lower()


def test_llm_filter_batch_processing(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-batch-filter_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE test_items (
        id INTEGER,
        text VARCHAR,
        category VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO test_items
                        VALUES (1, 'Apple iPhone', 'Technology'),
                               (2, 'War and Peace', 'Literature'),
                               (3, 'Tesla Model S', 'Technology'),
                               (4, 'The Great Gatsby', 'Literature'),
                               (5, 'Samsung Galaxy', 'Technology'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT 
            text,
            category,
            llm_filter(
                {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 2},
                    {'prompt': 'Is this item technology-related?', 'context_columns': [{'data': text}]}
        ) AS is_tech
    FROM test_items;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 6, f"Expected at least 6 lines, got {len(lines)}"
    assert "true" in result.stdout.lower() or "false" in result.stdout.lower()


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
                        INSERT INTO test_data
                        VALUES (1, 'Test content'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT llm_filter(
                       {'model_name': 'non-existent-model'},
        {'prompt': 'Test prompt', 'context_columns': [{'data': text}]}
    ) AS result
            FROM test_data; \
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

    test_model_name = f"test-empty-filter_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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
        SELECT llm_filter(
            {'model_name': '"""
        + test_model_name
        + """'},
        {'prompt': '', 'context_columns': [{'data': text}]}
    ) AS result
    FROM test_data;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0


def test_llm_filter_with_special_characters(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-unicode-filter_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE special_text (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO special_text
                        VALUES (1, 'Café résumé naïve'),
                               (2, 'Price: $100.99 (50% off!)'),
                               (3, 'Hello 世界 🌍'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT 
            text,
            llm_filter(
                {'model_name': '"""
        + test_model_name
        + """'},
                    {'prompt': 'Does this text contain non-ASCII characters?', 'context_columns': [{'data': text}]}
        ) AS has_unicode
    FROM special_text
    WHERE id = 1;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "true" in result.stdout.lower() or "false" in result.stdout.lower()


def test_llm_filter_with_model_params(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-params-filter_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE test_data (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO test_data
                        VALUES (1, 'Excellent quality product'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT 
            text,
            llm_filter(
                {'model_name': '"""
        + test_model_name
        + """', 'tuple_format': 'Markdown', 'batch_size': 1, 'model_parameters': '{"temperature": 0}'},
                    {'prompt': 'Is this text expressing positive sentiment?', 'context_columns': [{'data': text}]}
        ) AS is_positive
    FROM test_data;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "true" in result.stdout.lower() or "false" in result.stdout.lower()


def test_llm_filter_with_structured_output(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-structured-filter_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE items (
        id INTEGER,
        name VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO items
                        VALUES (1, 'Smartphone'),
                               (2, 'Novel'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT
            name,
            llm_filter(
                {'model_name': '"""
        + test_model_name
        + """',
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
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 3


def test_llm_filter_error_handling_missing_arguments(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-missing-args_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Test with only 1 argument (should fail since llm_filter requires 2)
    query = (
        """
        SELECT llm_filter(
            {'model_name': '"""
        + test_model_name
        + """'}
    ) AS result;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0, "Expected error for missing second argument"


def _test_llm_filter_performance_large_dataset(integration_setup, model_config):
    """Performance test - commented out with underscore prefix for optional execution"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-perf-filter_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE large_content AS
    SELECT
        i as id,
        'Content item ' || i || ' with some text to analyze' as content
    FROM range(1, 21) t(i);
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = (
        """
        SELECT
            content,
            llm_filter(
                {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 5},
                    {'prompt': 'Does this content contain the word "item"?', 'context_columns': [{'data': content}]}
        ) AS filter_result
    FROM large_content
    LIMIT 10;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 11, (
        f"Expected at least 11 lines (header + 10 data), got {len(lines)}"
    )
    assert "true" in result.stdout.lower() or "false" in result.stdout.lower()


def test_llm_filter_with_image_integration(integration_setup, model_config_image):
    """Test llm_filter with image data integration."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-filter-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE vehicle_images (
        id INTEGER,
        vehicle_type VARCHAR,
        image_url VARCHAR,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    car_url = "https://images.unsplash.com/photo-1549317661-bd32c8ce0db2?w=400"
    motorcycle_url = "https://images.unsplash.com/photo-1558618666-fcd25c85cd64?w=400"
    bicycle_url = "https://images.unsplash.com/photo-1532298229144-0ec0c57515c7?w=400"

    # Get image data in appropriate format for provider
    car_image = get_image_data_for_provider(car_url, provider)
    motorcycle_image = get_image_data_for_provider(motorcycle_url, provider)
    bicycle_image = get_image_data_for_provider(bicycle_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO vehicle_images
                        VALUES (1, 'Car', '{car_image}',
                                'Modern sedan car'),
                               (2, 'Motorcycle', '{motorcycle_image}',
                                'Sport motorcycle'),
                               (3, 'Bicycle', '{bicycle_image}',
                                'Mountain bike'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT 
            vehicle_type,
            llm_filter(
                {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Is this image showing a motorized vehicle?',
                'context_columns': [
                    {'data': vehicle_type},
                    {'data': image_url, 'type': 'image'}
                ]
            }
        ) AS is_motorized
    FROM vehicle_images 
    WHERE id = 1;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "is_motorized" in result.stdout.lower()
    assert len(result.stdout.strip().split("\n")) >= 2


def test_llm_filter_image_batch_processing(integration_setup, model_config_image):
    """Test llm_filter with multiple images in batch processing."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-batch-filter_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE food_images (
        id INTEGER,
        food_name VARCHAR,
        image_url VARCHAR,
        cuisine_type VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    pizza_url = "https://images.unsplash.com/photo-1513104890138-7c749659a591?w=400"
    sushi_url = "https://images.unsplash.com/photo-1579584425555-c3ce17fd4351?w=400"
    burger_url = "https://images.unsplash.com/photo-1568901346375-23c9450c58cd?w=400"

    # Get image data in appropriate format for provider
    pizza_image = get_image_data_for_provider(pizza_url, provider)
    sushi_image = get_image_data_for_provider(sushi_url, provider)
    burger_image = get_image_data_for_provider(burger_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO food_images
                        VALUES (1, 'Pizza', '{pizza_image}',
                                'Italian'),
                               (2, 'Sushi', '{sushi_image}',
                                'Japanese'),
                               (3, 'Burger', '{burger_image}',
                                'American'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT 
            food_name,
            cuisine_type,
            llm_filter(
                {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Does this food image look appetizing and well-presented?',
                'context_columns': [
                    {'data': food_name},
                    {'data': image_url, 'type': 'image'}
                ]
            }
        ) AS is_appetizing
    FROM food_images 
    ORDER BY id;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 data), got {len(lines)}"
    )
    assert "is_appetizing" in result.stdout.lower()


def test_llm_filter_image_with_text_context(integration_setup, model_config_image):
    """Test llm_filter with both image and text context."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-text-filter_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE clothing_images (
        id INTEGER,
        item_name VARCHAR,
        image_url VARCHAR,
        season VARCHAR,
        price_range VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    jacket_url = "https://images.unsplash.com/photo-1706765779494-2705542ebe74?w=400"
    dress_url = "https://images.unsplash.com/photo-1515372039744-b8f02a3ae446?w=400"
    shirt_url = "https://images.unsplash.com/photo-1521572163474-6864f9cf17ab?w=400"

    # Get image data in appropriate format for provider
    jacket_image = get_image_data_for_provider(jacket_url, provider)
    dress_image = get_image_data_for_provider(dress_url, provider)
    shirt_image = get_image_data_for_provider(shirt_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO clothing_images
                        VALUES (1, 'Winter Jacket',
                                '{jacket_image}', 'Winter', 'High'),
                               (2, 'Summer Dress', '{dress_image}',
                                'Summer', 'Medium'),
                               (3, 'Casual Shirt', '{shirt_image}',
                                'All Seasons', 'Low'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT 
            item_name,
            season,
            price_range,
            llm_filter(
                {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Based on the image and the season/price information, is this clothing item appropriate for its intended season and price range?',
                'context_columns': [
                    {'data': item_name},
                    {'data': image_url, 'type': 'image'},
                    {'data': season},
                    {'data': price_range}
                ]
            }
        ) AS is_appropriate
    FROM clothing_images 
    WHERE id = 1;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "is_appropriate" in result.stdout.lower()
    assert len(result.stdout.strip().split("\n")) >= 2


def test_llm_filter_with_audio_transcription(integration_setup, model_config):
    """Test llm_filter with audio transcription using OpenAI.

    The audio content says: 'Flock transforms DuckDB into a hybrid database and a semantic AI engine'
    This test verifies that the audio is correctly transcribed and filtered.
    """
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    if provider != "openai":
        pytest.skip("Audio transcription is only supported for OpenAI provider")

    test_model_name = f"test-audio-filter_{model_name}"
    create_model_query = f"CREATE MODEL('{test_model_name}', 'gpt-4o-mini', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    transcription_model_name = f"test-transcription-filter_{model_name}"
    create_transcription_model_query = f"CREATE MODEL('{transcription_model_name}', 'gpt-4o-mini-transcribe', 'openai');"
    run_cli(
        duckdb_cli_path, db_path, create_transcription_model_query, with_secrets=False
    )

    # Get audio file path
    audio_path = get_audio_file_path()

    # Test with audio file path - the audio actually mentions DuckDB/Flock
    query = (
        """
        SELECT llm_filter(
            {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Does this audio mention DuckDB or databases?',
                'context_columns': [
                    {
                        'data': audio_path,
                        'type': 'audio',
                        'transcription_model': '"""
        + transcription_model_name
        + """'
                    }
                ]
            }
        ) AS mentions_database
        FROM VALUES ('"""
        + audio_path
        + """') AS tbl(audio_path);
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    # The audio mentions DuckDB, so the filter should return true
    result_lower = result.stdout.lower()
    assert "true" in result_lower, (
        f"Expected 'true' since audio mentions DuckDB. Got: {result.stdout}"
    )


def test_llm_filter_audio_ollama_error(integration_setup):
    """Test that Ollama provider throws error for audio transcription in llm_filter."""
    duckdb_cli_path, db_path = integration_setup

    test_model_name = "test-ollama-filter-audio"
    create_model_query = (
        "CREATE MODEL('test-ollama-filter-audio', 'gemma3:1b', 'ollama');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    transcription_model_name = "test-ollama-filter-transcription"
    create_transcription_model_query = (
        "CREATE MODEL('test-ollama-filter-transcription', 'gemma3:1b', 'ollama');"
    )
    run_cli(
        duckdb_cli_path, db_path, create_transcription_model_query, with_secrets=False
    )

    query = """
        SELECT llm_filter(
            {'model_name': 'test-ollama-filter-audio'},
            {
                'prompt': 'Is the sentiment positive?',
                'context_columns': [
                    {
                        'data': audio_url,
                        'type': 'audio',
                        'transcription_model': 'test-ollama-filter-transcription'
                    }
                ]
            }
        ) AS result
        FROM VALUES ('https://example.com/audio.mp3') AS tbl(audio_url);
        """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0
    assert (
        "ollama" in result.stderr.lower()
        or "transcription" in result.stderr.lower()
        or "not supported" in result.stderr.lower()
    )
