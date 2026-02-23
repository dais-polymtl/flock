import os
import subprocess
import pytest
from pathlib import Path
from dotenv import load_dotenv
from integration.setup_test_db import setup_test_db

load_dotenv()

TEST_AUDIO_FILE_PATH = Path(__file__).parent / "tests" / "flock_test_audio.mp3"


def get_audio_file_path():
    return str(TEST_AUDIO_FILE_PATH.resolve())


def get_secrets_setup_sql():
    openai_key = os.getenv("OPENAI_API_KEY", "")
    ollama_url = os.getenv("API_URL", "http://localhost:11434")
    anthropic_key = os.getenv("ANTHROPIC_API_KEY", "")

    secrets_sql = []

    if openai_key:
        secrets_sql.append(f"CREATE SECRET (TYPE OPENAI, API_KEY '{openai_key}');")

    if ollama_url:
        secrets_sql.append(f"CREATE SECRET (TYPE OLLAMA, API_URL '{ollama_url}');")

    if anthropic_key:
        secrets_sql.append(f"CREATE SECRET (TYPE ANTHROPIC, API_KEY '{anthropic_key}');")

    return " ".join(secrets_sql)


@pytest.fixture(scope="session")
def integration_setup(tmp_path_factory):
    duckdb_cli_path = os.getenv("DUCKDB_CLI_PATH", "duckdb")
    test_db_name = "test.db"
    test_db_path = tmp_path_factory.mktemp("integration") / test_db_name

    print(f"Creating temporary database at: {test_db_path}")
    setup_test_db(test_db_path)

    try:
        yield str(duckdb_cli_path), str(test_db_path)
    finally:
        if not str(test_db_path).endswith(test_db_name):
            if os.path.exists(test_db_path):
                os.remove(test_db_path)


def run_cli(duckdb_cli_path, db_path, query, with_secrets=True):
    if with_secrets:
        secrets_sql = get_secrets_setup_sql()
        if secrets_sql:
            query = f"{secrets_sql} {query}"

    result = subprocess.run(
        [duckdb_cli_path, db_path, "-csv", "-c", query],
        capture_output=True,
        text=True,
        check=False,
    )

    # Filter out the secret creation output (Success, true lines) from stdout
    if with_secrets and result.stdout:
        lines = result.stdout.split("\n")
        # Remove lines that are just "Success" or "true" from secret creation
        filtered_lines = []
        skip_count = 0
        for line in lines:
            stripped = line.strip()
            if skip_count > 0 and stripped in ("true", "false"):
                skip_count -= 1
                continue
            if stripped == "Success":
                skip_count = 1  # Skip the next line (true/false)
                continue
            filtered_lines.append(line)
        result = subprocess.CompletedProcess(
            args=result.args,
            returncode=result.returncode,
            stdout="\n".join(filtered_lines),
            stderr=result.stderr,
        )

    return result


def get_image_data_for_provider(image_url, provider):
    """
    Get image data in the appropriate format based on the provider.
    Now all providers support URLs directly - the C++ code handles
    downloading and converting to base64 for providers that need it (Ollama).
    """
    return image_url
