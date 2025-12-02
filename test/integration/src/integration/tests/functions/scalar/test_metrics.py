import pytest
from integration.conftest import run_cli


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("llama3.2", "ollama")])
def model_config(request):
    return request.param


def test_flock_get_metrics_returns_json(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    query = "SELECT flock_get_metrics() AS metrics;"
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "metrics" in result.stdout.lower()
    assert "total_input_tokens" in result.stdout.lower()
    assert "total_output_tokens" in result.stdout.lower()
    assert "total_api_calls" in result.stdout.lower()


def test_flock_reset_metrics(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    query = "SELECT flock_reset_metrics() AS result;"
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "reset" in result.stdout.lower()


def test_metrics_after_llm_complete(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-metrics-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

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
    run_cli(duckdb_cli_path, db_path, query)

    metrics_query = "SELECT flock_get_metrics() AS metrics;"
    metrics_result = run_cli(duckdb_cli_path, db_path, metrics_query)

    assert metrics_result.returncode == 0
    assert "total_api_calls" in metrics_result.stdout.lower()


def test_metrics_reset_clears_counters(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reset-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    query = (
        """
        SELECT llm_complete(
            {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Say hello'}
        ) AS result;
        """
    )
    run_cli(duckdb_cli_path, db_path, query)

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    metrics_query = "SELECT flock_get_metrics() AS metrics;"
    metrics_result = run_cli(duckdb_cli_path, db_path, metrics_query)

    assert metrics_result.returncode == 0
    output = metrics_result.stdout.lower()
    assert "total_api_calls" in output and ":0" in output.replace(" ", "")
