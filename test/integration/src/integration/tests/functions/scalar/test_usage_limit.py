import os
import re
from typing import Optional

import pytest
from integration.conftest import run_cli

# OpenAI-compatible server config (e.g. llama.cpp):
#   llama-server -m model.gguf --port 8080
#   OPENAI_COMPATIBLE_BASE_URL=http://127.0.0.1:8080/v1
#   OPENAI_COMPATIBLE_MODELS=model-a,model-b
OPENAI_COMPATIBLE_BASE_URL = os.getenv("OPENAI_COMPATIBLE_BASE_URL", "")
OPENAI_COMPATIBLE_API_KEY = os.getenv("OPENAI_COMPATIBLE_API_KEY", "sk-no-key")
OPENAI_COMPATIBLE_MODELS = [
    model.strip() for model in os.getenv("OPENAI_COMPATIBLE_MODELS", "").split(",") if model.strip()
]
SECRET_NAME = "test_openai_compatible_llamacpp"


def pytest_generate_tests(metafunc):
    if "openai_compatible_model" not in metafunc.fixturenames:
        return

    if OPENAI_COMPATIBLE_MODELS and OPENAI_COMPATIBLE_BASE_URL:
        metafunc.parametrize("openai_compatible_model", OPENAI_COMPATIBLE_MODELS)
        return

    metafunc.parametrize(
        "openai_compatible_model",
        [
            pytest.param(
                "",
                marks=pytest.mark.skip(
                    reason=(
                        "Set OPENAI_COMPATIBLE_BASE_URL and OPENAI_COMPATIBLE_MODELS "
                        "(comma-separated) to run usage_limit integration tests."
                    )
                ),
            )
        ],
    )


def _model_slug(model_name: str) -> str:
    return re.sub(r"[^a-zA-Z0-9]+", "-", model_name).strip("-").lower()


def _usage_limit_json(
    *,
    prompt_tokens_limit: Optional[int] = None,
    completion_tokens_limit: Optional[int] = None,
    total_tokens_limit: Optional[int] = None,
) -> str:
    fields = []
    if prompt_tokens_limit is not None:
        fields.append(f'"prompt_tokens_limit": {prompt_tokens_limit}')
    if completion_tokens_limit is not None:
        fields.append(f'"completion_tokens_limit": {completion_tokens_limit}')
    if total_tokens_limit is not None:
        fields.append(f'"total_tokens_limit": {total_tokens_limit}')
    return "{" + ", ".join(fields) + "}"


def _openai_compatible_setup_sql(
    provider_model: str,
    flock_model_name: str,
    *,
    batch_size: int = 1,
    is_async: bool = False,
    prompt_tokens_limit: Optional[int] = None,
    completion_tokens_limit: Optional[int] = None,
    total_tokens_limit: Optional[int] = None,
) -> str:
    usage_limit = _usage_limit_json(
        prompt_tokens_limit=prompt_tokens_limit,
        completion_tokens_limit=completion_tokens_limit,
        total_tokens_limit=total_tokens_limit,
    )
    # Secrets are session-scoped; bundle secret + model setup with each query.
    return (
        f"CREATE SECRET {SECRET_NAME} ("
        f"TYPE OPENAI, "
        f"API_KEY '{OPENAI_COMPATIBLE_API_KEY}', "
        f"BASE_URL '{OPENAI_COMPATIBLE_BASE_URL}'"
        f"); "
        f"CREATE MODEL('{flock_model_name}', '{provider_model}', 'openai', "
        f'{{"usage_limit": {usage_limit}, '
        f'"batch_size": {batch_size}, "is_async": {str(is_async).lower()}}});'
    )


def _llm_complete_over_prompts_sql(flock_model_name: str, table_name: str) -> str:
    return f"""
    SELECT llm_complete(
        {{'model_name': '{flock_model_name}', 'secret_name': '{SECRET_NAME}'}},
        {{'prompt': 'Reply with one word only: {{prompt}}', 'context_columns': [{{'data': prompt}}]}}
    ) AS result
    FROM {table_name};
    """


# Flock surfaces usage-limit failures as a DuckDB HTTPException. The DuckDB CLI prints:
#   HTTP Error: <token_type> usage <count> exceeded limit of <limit> for this model; ...
_USAGE_LIMIT_MESSAGE_RE = re.compile(
    r"(?P<token_type>prompt_tokens|completion_tokens|total_tokens)\s+usage\s+(?P<usage>\d+)\s+"
    r"exceeded limit of\s+(?P<limit>\d+)"
)


def _extract_usage_limit_error(result):
    """Return (token_type, usage, limit) parsed from the CLI error output, or fail."""
    combined_output = result.stderr + result.stdout
    assert (
        "HTTP Error:" in combined_output
    ), f"Expected an HTTP Error message, got: stdout={result.stdout!r} stderr={result.stderr!r}"
    match = _USAGE_LIMIT_MESSAGE_RE.search(combined_output)
    assert (
        match is not None
    ), f"Expected a usage-limit error message, got: stdout={result.stdout!r} stderr={result.stderr!r}"
    return match.group("token_type"), int(match.group("usage")), int(match.group("limit"))


def _assert_usage_limit_exceeded(result, expected_token_type: Optional[str] = None):
    assert result.returncode != 0, "Expected usage_limit to be exceeded"
    token_type, usage, limit = _extract_usage_limit_error(result)
    assert usage >= limit, f"Expected usage {usage} to be at or above limit {limit}"
    if expected_token_type is not None:
        assert (
            token_type == expected_token_type
        ), f"Expected {expected_token_type!r} limit to be exceeded, got {token_type!r}"


def _batch_prompts_table_sql(table_name: str, row_count: int) -> str:
    rows = ",\n        ".join(f"('row-{i:02d}')" for i in range(1, row_count + 1))
    return f"""
    CREATE OR REPLACE TABLE {table_name} AS
    SELECT * FROM (VALUES
        {rows}
    ) AS t(prompt);
    """


def test_usage_limit_openai_compatible_single_call_succeeds(integration_setup, openai_compatible_model):
    """A single completion stays under a generous cumulative token quota."""
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-usage-limit-{_model_slug(openai_compatible_model)}-single"

    query = (
        _openai_compatible_setup_sql(openai_compatible_model, flock_model_name, total_tokens_limit=100_000)
        + f"""
    SELECT llm_complete(
        {{'model_name': '{flock_model_name}', 'secret_name': '{SECRET_NAME}'}},
        {{'prompt': 'Reply with one word: hello'}}
    ) AS result;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)

    assert result.returncode == 0, f"Expected success under quota: {result.stderr}"
    assert "result" in result.stdout.lower()


def test_usage_limit_openai_compatible_exceeded_on_batch(integration_setup, openai_compatible_model):
    """Sync batch_size 1: sequential requests accumulate tokens until the quota is exceeded."""
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-usage-limit-{_model_slug(openai_compatible_model)}-batch"

    query = (
        _openai_compatible_setup_sql(openai_compatible_model, flock_model_name, total_tokens_limit=100)
        + """
    CREATE OR REPLACE TABLE usage_limit_prompts AS
    SELECT * FROM (VALUES
        ('alpha'),
        ('beta'),
        ('gamma'),
        ('delta'),
        ('epsilon')
    ) AS t(prompt);
    """
        + _llm_complete_over_prompts_sql(flock_model_name, "usage_limit_prompts")
    )
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)

    _assert_usage_limit_exceeded(result, expected_token_type="total_tokens")


def test_usage_limit_openai_compatible_exceeded_prompt_tokens(integration_setup, openai_compatible_model):
    """Sync batch: cumulative prompt_tokens from provider usage exceeds prompt_tokens_limit."""
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-usage-limit-{_model_slug(openai_compatible_model)}-prompt"

    query = (
        _openai_compatible_setup_sql(
            openai_compatible_model,
            flock_model_name,
            prompt_tokens_limit=80,
        )
        + _batch_prompts_table_sql("usage_limit_prompt_tokens", row_count=8)
        + _llm_complete_over_prompts_sql(flock_model_name, "usage_limit_prompt_tokens")
    )
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)

    _assert_usage_limit_exceeded(result, expected_token_type="prompt_tokens")


def test_usage_limit_openai_compatible_exceeded_completion_tokens(integration_setup, openai_compatible_model):
    """Sync batch: cumulative completion_tokens from provider usage exceeds completion_tokens_limit."""
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-usage-limit-{_model_slug(openai_compatible_model)}-completion"

    query = (
        _openai_compatible_setup_sql(
            openai_compatible_model,
            flock_model_name,
            completion_tokens_limit=20,
        )
        + _batch_prompts_table_sql("usage_limit_completion_tokens", row_count=10)
        + _llm_complete_over_prompts_sql(flock_model_name, "usage_limit_completion_tokens")
    )
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)

    _assert_usage_limit_exceeded(result, expected_token_type="completion_tokens")


def test_usage_limit_openai_compatible_exceeded_on_async_batch(integration_setup, openai_compatible_model):
    """Async with batch_size 16 and 8 rows: all rows fit in one HTTP request."""
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-usage-limit-{_model_slug(openai_compatible_model)}-async-batch"

    query = (
        _openai_compatible_setup_sql(
            openai_compatible_model,
            flock_model_name,
            total_tokens_limit=100,
            batch_size=16,
            is_async=True,
        )
        + """
    CREATE OR REPLACE TABLE usage_limit_async_prompts AS
    SELECT * FROM (VALUES
        ('alpha'),
        ('beta'),
        ('gamma'),
        ('delta'),
        ('epsilon'),
        ('zeta'),
        ('eta'),
        ('theta')
    ) AS t(prompt);
    """
        + _llm_complete_over_prompts_sql(flock_model_name, "usage_limit_async_prompts")
    )
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)

    _assert_usage_limit_exceeded(result, expected_token_type="total_tokens")


def test_usage_limit_openai_compatible_exceeded_on_async_parallel_batches(integration_setup, openai_compatible_model):
    """Async with batch_size 16: one batch (16 rows) fits, two batches (20 rows) exceed the quota."""
    duckdb_cli_path, db_path = integration_setup
    slug = _model_slug(openai_compatible_model)
    token_limit = 1200

    rows_16 = ",\n        ".join(f"('r{i:02d}')" for i in range(1, 17))
    control_model = f"test-usage-limit-{slug}-async-parallel-16"
    control_query = (
        _openai_compatible_setup_sql(
            openai_compatible_model,
            control_model,
            total_tokens_limit=token_limit,
            batch_size=16,
            is_async=True,
        )
        + f"""
    CREATE OR REPLACE TABLE usage_limit_async_parallel_16 AS
    SELECT * FROM (VALUES
        {rows_16}
    ) AS t(prompt);
    """
        + _llm_complete_over_prompts_sql(control_model, "usage_limit_async_parallel_16")
    )
    control_result = run_cli(duckdb_cli_path, db_path, control_query, with_secrets=False)
    assert (
        control_result.returncode == 0
    ), f"Expected one 16-row async batch to stay under {token_limit} tokens: {control_result.stderr}"

    rows_20 = ",\n        ".join(f"('r{i:02d}')" for i in range(1, 21))
    test_model = f"test-usage-limit-{slug}-async-parallel-20"
    test_query = (
        _openai_compatible_setup_sql(
            openai_compatible_model,
            test_model,
            total_tokens_limit=token_limit,
            batch_size=16,
            is_async=True,
        )
        + f"""
    CREATE OR REPLACE TABLE usage_limit_async_parallel_20 AS
    SELECT * FROM (VALUES
        {rows_20}
    ) AS t(prompt);
    """
        + _llm_complete_over_prompts_sql(test_model, "usage_limit_async_parallel_20")
    )
    test_result = run_cli(duckdb_cli_path, db_path, test_query, with_secrets=False)

    _assert_usage_limit_exceeded(test_result, expected_token_type="total_tokens")
