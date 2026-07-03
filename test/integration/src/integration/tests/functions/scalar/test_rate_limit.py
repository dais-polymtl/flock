import os
import re
import time

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

# Set RUN_SLOW_RATE_LIMIT_TESTS=1 to run the end-to-end throttle wait test (~60s).
RUN_SLOW_RATE_LIMIT_TESTS = os.getenv("RUN_SLOW_RATE_LIMIT_TESTS", "").lower() in ("1", "true", "yes")


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
                        "(comma-separated) to run rate_limit integration tests."
                    )
                ),
            )
        ],
    )


def _model_slug(model_name: str) -> str:
    return re.sub(r"[^a-zA-Z0-9]+", "-", model_name).strip("-").lower()


@pytest.mark.skipif(
    not RUN_SLOW_RATE_LIMIT_TESTS,
    reason="Set RUN_SLOW_RATE_LIMIT_TESTS=1 to run the ~60s throttle wait test.",
)
def test_rate_limit_openai_compatible_throttles_over_limit_without_error(integration_setup, openai_compatible_model):
    """End-to-end: one request over the per-minute cap waits for the window to roll, then succeeds.

    Unit tests cover limiter math and wiring with mocks; this is the only integration test
    that exercises the full DuckDB CLI path with the production 60-second rolling window.
    """
    duckdb_cli_path, db_path = integration_setup
    flock_model_name = f"test-rate-limit-{_model_slug(openai_compatible_model)}-throttle"
    rate_limit = 1
    row_count = 2

    query = (
        f"CREATE SECRET {SECRET_NAME} ("
        f"TYPE OPENAI, "
        f"API_KEY '{OPENAI_COMPATIBLE_API_KEY}', "
        f"BASE_URL '{OPENAI_COMPATIBLE_BASE_URL}'"
        f"); "
        f"CREATE MODEL('{flock_model_name}', '{openai_compatible_model}', 'openai', "
        f'{{"rate_limit": {rate_limit}, "batch_size": 1, "is_async": false}});'
        f"""
    CREATE OR REPLACE TABLE rate_limit_throttle_prompts AS
    SELECT * FROM (VALUES
        ('row-01'),
        ('row-02')
    ) AS t(prompt);

    SELECT llm_complete(
        {{'model_name': '{flock_model_name}', 'secret_name': '{SECRET_NAME}'}},
        {{'prompt': 'Reply with one word only: {{prompt}}', 'context_columns': [{{'data': prompt}}]}}
    ) AS result
    FROM rate_limit_throttle_prompts;
    """
    )

    start = time.monotonic()
    result = run_cli(duckdb_cli_path, db_path, query, with_secrets=False)
    elapsed = time.monotonic() - start

    assert result.returncode == 0, f"Expected throttled query to eventually succeed: {result.stderr}"
    assert "result" in result.stdout.lower()
    assert elapsed >= 55.0, f"Expected throttle wait (~60s), got {elapsed:.1f}s"
    assert elapsed < 90.0, f"Throttle wait took unexpectedly long ({elapsed:.1f}s)"
