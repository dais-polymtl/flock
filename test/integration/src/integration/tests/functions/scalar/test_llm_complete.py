import pytest
from integration.conftest import (
    run_cli,
    get_image_data_for_provider,
    get_audio_file_path,
)

# Expected keywords that should appear when audio is transcribed
# Audio content: "Flock transforms DuckDB into a hybrid database and a semantic AI engine"
AUDIO_EXPECTED_KEYWORDS = ["flock", "duckdb", "database", "semantic", "ai", "hybrid"]


@pytest.fixture(params=[
    ("gpt-4o-mini", "openai"),
    ("gemma3:1b", "ollama"),
    ("claude-3-haiku-20240307", "anthropic")
])
def model_config(request):
    """Fixture to test with different models for text-only tests."""
    return request.param


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("gemma3:4b", "ollama")])
def model_config_image(request):
    """Fixture to test with different models for image tests."""
    return request.param


def test_llm_complete_basic_functionality(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
        """
        SELECT llm_complete(
            {'model_name': '"""
        + test_model_name
        + """'},
        {'prompt': 'What is 2+2?'}
    ) AS result;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "result" in result.stdout.lower()


def test_llm_complete_with_input_columns(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-model-input_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE countries (
        id INTEGER,
        name VARCHAR,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO countries
                        VALUES (1, 'France', 'European country'),
                               (2, 'Canada', 'North American country'),
                               (3, 'Japan', 'Asian island nation'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
                SELECT name,
                       llm_complete(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'What is the capital of {country}?', 'context_columns': [{'data': name}]}
        ) AS capital
            FROM countries
            WHERE id <= 2; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 3, f"Expected at least 3 lines, got {len(lines)}"


def test_llm_complete_batch_processing(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-batch-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE product_reviews (
        id INTEGER,
        review_text VARCHAR,
        product_name VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO product_reviews
                        VALUES (1, 'This laptop is amazing! Fast and reliable.', 'Laptop Pro'),
                               (2, 'The phone has poor battery life but good camera.', 'Smart Phone X'),
                               (3, 'Great value for money. Highly recommended!', 'Budget Tablet'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
                SELECT product_name,
                       llm_complete(
                           {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 2},
            {'prompt': 'Analyze the sentiment of this review: {review}. Respond with POSITIVE, NEGATIVE, or NEUTRAL.', 'context_columns': [{'data': review_text}]}
        ) AS sentiment
            FROM product_reviews; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, f"Expected at least 4 lines, got {len(lines)}"


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

    test_model_name = f"test-empty-prompt_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
        """
        SELECT llm_complete(
            {'model_name': '"""
        + test_model_name
        + """'},
        {'prompt': ''}
    ) AS result;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0


def test_llm_complete_with_special_characters(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-unicode-model_{model_name}"
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
                SELECT llm_complete(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Translate this text to English if needed: {text}', 'context_columns': [{'data': text}]}
        ) AS translation
            FROM special_text
            WHERE id = 1; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert lines[0] == "translation"


def test_llm_complete_with_model_params(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-params-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
        """
        SELECT llm_complete(
            {'model_name': '"""
        + test_model_name
        + """', 'tuple_format': 'Markdown', 'batch_size': 1, 'model_parameters': '{"temperature": 0}'},
        {'prompt': 'Briefly, what is the capital of France?'}
    ) AS result;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "paris" in result.stdout.lower()


def test_llm_complete_with_structured_output_without_table(
    integration_setup, model_config
):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-structured-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    response_format = ""
    if provider == "openai":
        response_format = """
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
            }
            """
    elif provider == "ollama":
        response_format = """
            "format": {
                "type": "object",
                "properties": {
                    "capital": { "type": "string" }
                },
                "required": ["capital"]
            }
            """
    elif provider == "anthropic":
        # Anthropic uses output_format for structured output
        response_format = """
            "output_format": {
                "type": "json_schema",
                "schema": {
                    "type": "object",
                    "properties": {
                        "capital": { "type": "string" }
                    },
                    "required": ["capital"]
                }
            }
            """
    query = (
        """
            SELECT llm_complete(
                {'model_name': '"""
        + test_model_name
        + """',
            'model_parameters': '{"""
        + response_format
        + """}'
            },
            {'prompt': 'What is the capital of Canada?'}
        ) AS summary;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    response = result.stdout.strip().split("\n")[1].split(",")[0].lower()
    assert "ottawa" in response


def test_llm_complete_with_structured_output_with_table(
    integration_setup, model_config
):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-structured-table-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE countries (
        id INTEGER,
        name VARCHAR,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO countries
                        VALUES (1, 'France', 'European country'),
                               (2, 'Canada', 'North American country'),
                               (3, 'Japan', 'Asian island nation'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    response_format = ""
    if provider == "openai":
        response_format = """
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
            }
            """
    elif provider == "ollama":
        response_format = """
            "format": {
                "type": "object",
                "properties": {
                    "capital": {
                        "type": "string",
                        "pattern": "^[A-Za-z]+$"
                    }
                },
                "required": ["capital"]
            }
            """
    elif provider == "anthropic":
        # Anthropic uses output_format for structured output
        response_format = """
            "output_format": {
                "type": "json_schema",
                "schema": {
                    "type": "object",
                    "properties": {
                        "capital": { "type": "string" }
                    },
                    "required": ["capital"]
                }
            }
            """

    query = (
        """
                SELECT name,
                       llm_complete(
                           {'model_name': '"""
        + test_model_name
        + """', 'model_parameters': '{"""
        + response_format
        + """}' },
            {'prompt': 'What is the capital of each country?', 'context_columns': [{'data': name}]}
        ) AS capital_info
            FROM countries
            WHERE id <= 2; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)
    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 3
    # Check for expected capitals in output (flexible assertion for different response formats)
    assert "paris" in result.stdout.lower() and "ottawa" in result.stdout.lower()


def _llm_complete_performance_large_dataset(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-perf-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE large_dataset AS
    SELECT
        i as id,
        'Item ' || i as item_name,
        'Description for item ' || i as description
    FROM range(1, 11) t(i);
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = (
        """
                SELECT item_name,
                       llm_complete(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Create a short marketing slogan for: {item}', 'context_columns': [{'data': item_name}]}
        ) AS slogan
            FROM large_dataset LIMIT 5; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 6, (
        f"Expected at least 6 lines (header + 5 data), got {len(lines)}"
    )


def test_llm_complete_with_image_integration(integration_setup, model_config_image):
    """Test llm_complete with image data integration."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE animal_images (
        id INTEGER,
        name VARCHAR,
        image VARCHAR,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    lion_url = "https://images.unsplash.com/photo-1549366021-9f761d450615?w=400"
    elephant_url = "https://images.unsplash.com/photo-1557050543-4d5f2e07c723?w=400"
    giraffe_url = "https://images.unsplash.com/photo-1547721064-da6cfb341d50?w=400"

    # Get image data in appropriate format for provider
    lion_image = get_image_data_for_provider(lion_url, provider)
    elephant_image = get_image_data_for_provider(elephant_url, provider)
    giraffe_image = get_image_data_for_provider(giraffe_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO animal_images
                        VALUES (1, 'Lion', '{lion_image}',
                                'African lion in savanna'),
                               (2, 'Elephant', '{elephant_image}',
                                'African elephant in nature'),
                               (3, 'Giraffe', '{giraffe_image}',
                                'Giraffe in the wild'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
                SELECT name,
                       llm_complete(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Describe what you see in this image. What animal is it and what are its characteristics?',
                'context_columns': [
                    {'data': name},
                    {'data': image, 'type': 'image'}
                ]
            }
        ) AS image_description
            FROM animal_images
            WHERE id = 1; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "image_description" in result.stdout.lower()
    # Check that we got some meaningful output about the image
    assert len(result.stdout.strip().split("\n")) >= 2


def test_llm_complete_image_batch_processing(integration_setup, model_config_image):
    """Test llm_complete with multiple images in batch processing."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-batch-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE product_images (
        id INTEGER,
        product_name VARCHAR,
        image_url VARCHAR,
        category VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    chair_url = "https://images.unsplash.com/photo-1567538096630-e0c55bd6374c?w=400"
    smartphone_url = (
        "https://images.unsplash.com/photo-1511707171634-5f897ff02aa9?w=400"
    )
    coffee_url = "https://images.unsplash.com/photo-1495474472287-4d71bcdd2085?w=400"

    # Get image data in appropriate format for provider
    chair_image = get_image_data_for_provider(chair_url, provider)
    smartphone_image = get_image_data_for_provider(smartphone_url, provider)
    coffee_image = get_image_data_for_provider(coffee_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO product_images
                        VALUES (1, 'Modern Chair', '{chair_image}',
                                'Furniture'),
                               (2, 'Smartphone', '{smartphone_image}',
                                'Electronics'),
                               (3, 'Coffee Cup', '{coffee_image}',
                                'Kitchenware'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
                SELECT product_name,
                       category,
                       llm_complete(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Analyze this product image and describe its design, style, and potential use case.',
                'context_columns': [
                    {'data': product_name},
                    {'data': image_url, 'type': 'image'}
                ]
            }
        ) AS product_analysis
            FROM product_images
            ORDER BY id; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 data), got {len(lines)}"
    )
    assert "product_analysis" in result.stdout.lower()


def test_llm_complete_image_with_text_context(integration_setup, model_config_image):
    """Test llm_complete with both image and text context."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-text-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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

    # Image URLs
    mountain_url = "https://images.unsplash.com/photo-1506905925346-21bda4d32df4?w=400"
    forest_url = "https://images.unsplash.com/photo-1441974231531-c6227db76b6e?w=400"
    beach_url = "https://images.unsplash.com/photo-1507525428034-b723cf961d3e?w=400"

    # Get image data in appropriate format for provider
    mountain_image = get_image_data_for_provider(mountain_url, provider)
    forest_image = get_image_data_for_provider(forest_url, provider)
    beach_image = get_image_data_for_provider(beach_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO landscape_photos
                        VALUES (1, 'Mountain Peak',
                                '{mountain_image}', 'Clear',
                                'Summer'),
                               (2, 'Forest Trail', '{forest_image}',
                                'Overcast', 'Autumn'),
                               (3, 'Beach Sunset', '{beach_image}',
                                'Clear', 'Summer'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
                SELECT location,
                       weather_condition,
                       season,
                       llm_complete(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Based on this landscape image and the weather/season information, describe the atmosphere and mood of this scene.',
                'context_columns': [
                    {'data': location},
                    {'data': image_url, 'type': 'image'},
                    {'data': weather_condition},
                    {'data': season}
                ]
            }
        ) AS atmosphere_description
            FROM landscape_photos
            WHERE id = 1; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "atmosphere_description" in result.stdout.lower()
    assert len(result.stdout.strip().split("\n")) >= 2


def test_llm_complete_with_audio_transcription(integration_setup, model_config):
    """Test llm_complete with audio transcription using OpenAI."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    # Skip if not OpenAI (only OpenAI supports transcription currently)
    if provider != "openai":
        pytest.skip("Audio transcription is only supported for OpenAI provider")

    # Create main completion model
    test_model_name = f"test-audio-complete_{model_name}"
    create_model_query = f"CREATE MODEL('{test_model_name}', 'gpt-4o-mini', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Create transcription model
    transcription_model_name = f"test-transcription-model_{model_name}"
    create_transcription_model_query = f"CREATE MODEL('{transcription_model_name}', 'gpt-4o-mini-transcribe', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_transcription_model_query, with_secrets=False)

    # Get audio file path
    audio_path = get_audio_file_path()

    # Test with audio file path using VALUES
    query = (
        """
        SELECT llm_complete(
            {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'What product or technology is mentioned in this audio? Provide a brief answer.',
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
        ) AS audio_summary
        FROM VALUES ('"""
        + audio_path
        + """') AS tbl(audio_path);
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "audio_summary" in result.stdout.lower()
    # Verify the response is based on the audio content
    output_lower = result.stdout.lower()
    assert any(keyword in output_lower for keyword in AUDIO_EXPECTED_KEYWORDS), (
        f"Expected response to contain at least one of {AUDIO_EXPECTED_KEYWORDS}, got: {result.stdout}"
    )


def test_llm_complete_with_audio_and_text(integration_setup, model_config):
    """Test llm_complete with both audio and text context columns."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    if provider != "openai":
        pytest.skip("Audio transcription is only supported for OpenAI provider")

    test_model_name = f"test-audio-text_{model_name}"
    create_model_query = f"CREATE MODEL('{test_model_name}', 'gpt-4o-mini', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    transcription_model_name = f"test-transcription_{model_name}"
    create_transcription_model_query = f"CREATE MODEL('{transcription_model_name}', 'gpt-4o-mini-transcribe', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_transcription_model_query, with_secrets=False)

    # Get audio file path
    audio_path = get_audio_file_path()

    # Test with audio file path using VALUES - combining text context with audio
    query = (
        """
        SELECT llm_complete(
            {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Given the category {category}, describe how the technology mentioned in the audio fits into this category.',
                'context_columns': [
                    {'data': category_name, 'name': 'category'},
                    {
                        'data': audio_path,
                        'type': 'audio',
                        'transcription_model': '"""
        + transcription_model_name
        + """'
                    }
                ]
            }
        ) AS tech_description
        FROM VALUES ('Database Technology', '"""
        + audio_path
        + """') AS tbl(category_name, audio_path);
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "tech_description" in result.stdout.lower()
    # Verify the response mentions something from the audio content
    output_lower = result.stdout.lower()
    assert any(keyword in output_lower for keyword in AUDIO_EXPECTED_KEYWORDS), (
        f"Expected response to contain at least one of {AUDIO_EXPECTED_KEYWORDS}, got: {result.stdout}"
    )


def test_llm_complete_audio_missing_transcription_model(integration_setup):
    """Test that audio type requires transcription_model."""
    duckdb_cli_path, db_path = integration_setup

    test_model_name = "test-audio-error"
    create_model_query = f"CREATE MODEL('{test_model_name}', 'gpt-4o-mini', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Get audio file path
    audio_path = get_audio_file_path()

    query = (
        """
        SELECT llm_complete(
            {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Summarize this audio',
                'context_columns': [
                    {
                        'data': audio_path,
                        'type': 'audio'
                    }
                ]
            }
        ) AS result
        FROM VALUES ('"""
        + audio_path
        + """') AS tbl(audio_path);
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    # Should fail because transcription_model is required for audio type
    assert result.returncode != 0
    assert (
        "transcription_model" in result.stderr.lower()
        or "required" in result.stderr.lower()
    )


def test_llm_complete_audio_ollama_error(integration_setup):
    """Test that Ollama provider throws error for audio transcription."""
    duckdb_cli_path, db_path = integration_setup

    create_model_query = "CREATE MODEL('test-ollama-audio', 'gemma3:1b', 'ollama');"
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_transcription_model_query = (
        "CREATE MODEL('test-ollama-transcription', 'gemma3:1b', 'ollama');"
    )
    run_cli(duckdb_cli_path, db_path, create_transcription_model_query, with_secrets=False)

    # Get audio file path
    audio_path = get_audio_file_path()

    query = f"""
        SELECT llm_complete(
            {{"model_name": 'test-ollama-audio'}},
            {{
                'prompt': 'Summarize this audio',
                'context_columns': [
                    {{
                        'data': audio_path,
                        'type': 'audio',
                        'transcription_model': 'test-ollama-transcription'
                    }}
                ]
            }}
        ) AS result
        FROM VALUES ('{audio_path}') AS tbl(audio_path);
        """
    result = run_cli(duckdb_cli_path, db_path, query)

    # Should fail because Ollama doesn't support transcription
    assert result.returncode != 0
    assert (
        "ollama" in result.stderr.lower()
        or "transcription" in result.stderr.lower()
        or "not supported" in result.stderr.lower()
    )


def test_llm_complete_audio_batch_processing(integration_setup, model_config):
    """Test batch processing with multiple audio files."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    if provider != "openai":
        pytest.skip("Audio transcription is only supported for OpenAI provider")

    test_model_name = f"test-audio-batch_{model_name}"
    create_model_query = f"CREATE MODEL('{test_model_name}', 'gpt-4o-mini', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    transcription_model_name = f"test-transcription-batch_{model_name}"
    create_transcription_model_query = f"CREATE MODEL('{transcription_model_name}', 'gpt-4o-mini-transcribe', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_transcription_model_query, with_secrets=False)

    # Get audio file path
    audio_path = get_audio_file_path()

    create_table_query = """
    CREATE OR REPLACE TABLE audio_clips (
        id INTEGER,
        audio_path VARCHAR,
        product_name VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = f"""
    INSERT INTO audio_clips
    VALUES 
        (1, '{audio_path}', 'Headphones'),
        (2, '{audio_path}', 'Speaker'),
        (3, '{audio_path}', 'Microphone');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT product_name,
               llm_complete(
                   {'model_name': '"""
        + test_model_name
        + """'},
                   {
                       'prompt': 'Based on the product {product} and its audio, write a short description.',
                       'context_columns': [
                           {'data': product_name, 'name': 'product'},
                           {
                               'data': audio_path,
                               'type': 'audio',
                               'transcription_model': '"""
        + transcription_model_name
        + """'
                           }
                       ]
                   }
               ) AS description
        FROM audio_clips
        WHERE id <= 2;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 3  # Header + at least 2 data rows
