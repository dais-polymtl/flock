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


# Soft cap: in-flight requests that cross the quota return; later ones are rejected as NULL rows.
def _result_rows(stdout: str, alias: str, row_count: int) -> list:
    lines = stdout.splitlines()
    assert alias in lines, f"Expected a '{alias}' column in output: {stdout!r}"
    start = lines.index(alias) + 1
    rows = lines[start : start + row_count]
    assert len(rows) == row_count, f"Expected {row_count} '{alias}' rows, got {rows!r}"
    return rows


def _assert_capped_after_first(result, row_count: int):
    """Sync batch_size 1: the first request crosses the quota; every later row is rejected."""
    assert result.returncode == 0, f"Query failed: {result.stderr}"
    rows = _result_rows(result.stdout, "result", row_count)
    assert rows[0] != "NULL", f"Expected the in-flight request to return: {rows!r}"
    assert all(row == "NULL" for row in rows[1:]), f"Expected rows after the quota to be NULL: {rows!r}"


def _batch_prompts_table_sql(table_name: str, row_count: int) -> str:
    rows = ",\n        ".join(f"('row-{i:02d}')" for i in range(1, row_count + 1))
    return f"""
    CREATE OR REPLACE TABLE {table_name} AS
    SELECT * FROM (VALUES
        {rows}
    ) AS t(prompt);
    """


def _followup_sql(flock_model_name: str) -> str:
    return f"""
    SELECT llm_complete(
        {{'model_name': '{flock_model_name}', 'secret_name': '{SECRET_NAME}'}},
        {{'prompt': 'Reply with one word only: {{prompt}}', 'context_columns': [{{'data': 'hello'}}]}}
    ) AS followup;
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
    assert _result_rows(result.stdout, "result", 1) != ["NULL"]


def test_usage_limit_openai_compatible_exceeded_on_batch(integration_setup, openai_compatible_model):
    """Sync batch_size 1: the first request crosses total_tokens_limit and later rows are rejected."""
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-usage-limit-{_model_slug(openai_compatible_model)}-batch"

    query = (
        _openai_compatible_setup_sql(openai_compatible_model, flock_model_name, total_tokens_limit=100)
        + _batch_prompts_table_sql("usage_limit_prompts", row_count=5)
        + _llm_complete_over_prompts_sql(flock_model_name, "usage_limit_prompts")
    )
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)

    _assert_capped_after_first(result, row_count=5)


def test_usage_limit_openai_compatible_exceeded_prompt_tokens(integration_setup, openai_compatible_model):
    """Sync batch: provider-reported prompt_tokens crossing prompt_tokens_limit rejects later rows."""
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-usage-limit-{_model_slug(openai_compatible_model)}-prompt"

    query = (
        _openai_compatible_setup_sql(openai_compatible_model, flock_model_name, prompt_tokens_limit=80)
        + _batch_prompts_table_sql("usage_limit_prompt_tokens", row_count=8)
        + _llm_complete_over_prompts_sql(flock_model_name, "usage_limit_prompt_tokens")
    )
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)

    _assert_capped_after_first(result, row_count=8)


def test_usage_limit_openai_compatible_exceeded_completion_tokens(integration_setup, openai_compatible_model):
    """Sync batch: cumulative completion_tokens crossing completion_tokens_limit rejects later rows."""
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-usage-limit-{_model_slug(openai_compatible_model)}-completion"
    row_count = 10

    query = (
        _openai_compatible_setup_sql(openai_compatible_model, flock_model_name, completion_tokens_limit=20)
        + _batch_prompts_table_sql("usage_limit_completion_tokens", row_count=row_count)
        + _llm_complete_over_prompts_sql(flock_model_name, "usage_limit_completion_tokens")
    )
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)

    assert result.returncode == 0, f"Query failed: {result.stderr}"
    rows = _result_rows(result.stdout, "result", row_count)
    assert rows[0] != "NULL", f"Expected the first request to return: {rows!r}"
    assert rows[-1] == "NULL", f"Expected the completion quota to reject later rows: {rows!r}"


def test_usage_limit_openai_compatible_exceeded_on_async_batch(integration_setup, openai_compatible_model):
    """Async, one request for all 8 rows: it returns despite crossing the quota; the next query is rejected."""
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-usage-limit-{_model_slug(openai_compatible_model)}-async-batch"
    row_count = 8

    query = (
        _openai_compatible_setup_sql(
            openai_compatible_model,
            flock_model_name,
            total_tokens_limit=100,
            batch_size=16,
            is_async=True,
        )
        + _batch_prompts_table_sql("usage_limit_async_prompts", row_count=row_count)
        + _llm_complete_over_prompts_sql(flock_model_name, "usage_limit_async_prompts")
        + _followup_sql(flock_model_name)
    )
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)

    assert result.returncode == 0, f"Query failed: {result.stderr}"
    rows = _result_rows(result.stdout, "result", row_count)
    assert all(row != "NULL" for row in rows), f"Expected the in-flight batch to return: {rows!r}"
    assert _result_rows(result.stdout, "followup", 1) == ["NULL"]


def test_usage_limit_openai_compatible_exceeded_on_async_parallel_batches(integration_setup, openai_compatible_model):
    """Async, two batches sent in parallel: both return despite crossing the quota; the next query is rejected."""
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-usage-limit-{_model_slug(openai_compatible_model)}-async-parallel"
    row_count = 20

    query = (
        _openai_compatible_setup_sql(
            openai_compatible_model,
            flock_model_name,
            total_tokens_limit=100,
            batch_size=16,
            is_async=True,
        )
        + _batch_prompts_table_sql("usage_limit_async_parallel", row_count=row_count)
        + _llm_complete_over_prompts_sql(flock_model_name, "usage_limit_async_parallel")
        + _followup_sql(flock_model_name)
    )
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)

    assert result.returncode == 0, f"Query failed: {result.stderr}"
    rows = _result_rows(result.stdout, "result", row_count)
    assert all(row != "NULL" for row in rows), f"Expected both in-flight batches to return: {rows!r}"
    assert _result_rows(result.stdout, "followup", 1) == ["NULL"]
