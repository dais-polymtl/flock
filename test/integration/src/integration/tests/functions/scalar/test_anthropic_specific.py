"""
Anthropic-specific integration tests.

These tests verify Anthropic/Claude-specific behavior, particularly:
- Embedding requests correctly fail with descriptive error
- Basic completion works with Claude models
- Image support with URLs (downloaded and converted to base64)
"""

import pytest
from integration.conftest import run_cli, get_image_data_for_provider


def test_anthropic_embedding_error(integration_setup):
    """Test that Anthropic provider correctly rejects embedding requests."""
    duckdb_cli_path, db_path = integration_setup

    test_model_name = "test-anthropic-embedding-error"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', 'claude-3-haiku-20240307', 'anthropic');"
    )
    result = run_cli(duckdb_cli_path, db_path, create_model_query)

    # Skip if Anthropic is not configured
    if result.returncode != 0 and "secret" in result.stderr.lower():
        pytest.skip("Anthropic API key not configured")

    query = f"""
    SELECT llm_embedding(
        {{'model_name': '{test_model_name}'}},
        {{'context_columns': [{{'data': 'Test text for embedding'}}]}}
    ) AS embedding;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    # Should fail with clear error message about embeddings not being supported
    assert result.returncode != 0 or "error" in result.stderr.lower()
    output = (result.stderr + result.stdout).lower()
    assert any(
        keyword in output
        for keyword in ["embedding", "not support", "anthropic", "claude"]
    ), f"Expected embedding error message, got: {output}"


def test_anthropic_completion_basic(integration_setup):
    """Test basic Anthropic completion works."""
    duckdb_cli_path, db_path = integration_setup

    test_model_name = "test-anthropic-basic"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', 'claude-3-haiku-20240307', 'anthropic');"
    )
    result = run_cli(duckdb_cli_path, db_path, create_model_query)

    # Skip if Anthropic is not configured
    if result.returncode != 0 and "secret" in result.stderr.lower():
        pytest.skip("Anthropic API key not configured")

    query = f"""
    SELECT llm_complete(
        {{'model_name': '{test_model_name}'}},
        {{'prompt': 'What is 2+2? Reply with just the number.'}}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "result" in result.stdout.lower()


def test_anthropic_with_system_prompt(integration_setup):
    """Test Anthropic completion with system prompt."""
    duckdb_cli_path, db_path = integration_setup

    test_model_name = "test-anthropic-system-prompt"
    create_model_query = f"""
    CREATE MODEL(
        '{test_model_name}',
        'claude-3-haiku-20240307',
        'anthropic',
        {{'model_parameters': {{'system': 'You are a helpful math tutor.', 'max_tokens': 100}}}}
    );
    """
    result = run_cli(duckdb_cli_path, db_path, create_model_query)

    # Skip if Anthropic is not configured
    if result.returncode != 0 and "secret" in result.stderr.lower():
        pytest.skip("Anthropic API key not configured")

    query = f"""
    SELECT llm_complete(
        {{'model_name': '{test_model_name}'}},
        {{'prompt': 'What is the square root of 16?'}}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "4" in result.stdout


def test_anthropic_model_parameters(integration_setup):
    """Test Anthropic completion with custom model parameters."""
    duckdb_cli_path, db_path = integration_setup

    test_model_name = "test-anthropic-params"
    create_model_query = f"""
    CREATE MODEL(
        '{test_model_name}',
        'claude-3-haiku-20240307',
        'anthropic',
        {{'model_parameters': {{'temperature': 0, 'max_tokens': 50}}}}
    );
    """
    result = run_cli(duckdb_cli_path, db_path, create_model_query)

    # Skip if Anthropic is not configured
    if result.returncode != 0 and "secret" in result.stderr.lower():
        pytest.skip("Anthropic API key not configured")

    query = f"""
    SELECT llm_complete(
        {{'model_name': '{test_model_name}'}},
        {{'prompt': 'Say hello in exactly one word.'}}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"


def test_anthropic_image_with_url(integration_setup):
    """Test Anthropic completion with image URL (downloaded and converted to base64)."""
    duckdb_cli_path, db_path = integration_setup

    test_model_name = "test-anthropic-image-url"
    create_model_query = f"""
    CREATE MODEL(
        '{test_model_name}',
        'claude-3-haiku-20240307',
        'anthropic',
        {{'model_parameters': {{'max_tokens': 256}}}}
    );
    """
    result = run_cli(duckdb_cli_path, db_path, create_model_query)

    if result.returncode != 0 and "secret" in result.stderr.lower():
        pytest.skip("Anthropic API key not configured")

    # Use a small test image URL
    image_url = "https://images.unsplash.com/photo-1549366021-9f761d450615?w=100"
    image_data = get_image_data_for_provider(image_url, "anthropic")

    create_table_query = """
    CREATE OR REPLACE TABLE test_images (id INTEGER, image_url VARCHAR);
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_query = f"""
    INSERT INTO test_images VALUES (1, '{image_data}');
    """
    run_cli(duckdb_cli_path, db_path, insert_query)

    # Use literal "image_url" as column reference (not Python variable)
    context_cols = "[{'data': image_url, 'type': 'image'}]"
    query = f"""
    SELECT llm_complete(
        {{'model_name': '{test_model_name}'}},
        {{'prompt': 'What animal do you see in this image? Reply with one word.',
          'context_columns': {context_cols}}}
    ) AS result
    FROM test_images WHERE id = 1;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "result" in result.stdout.lower()
