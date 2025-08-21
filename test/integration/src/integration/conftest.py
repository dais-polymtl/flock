import os
import subprocess
import pytest
from pathlib import Path
from dotenv import load_dotenv
import base64
import requests
from integration.setup_test_db import setup_test_db

load_dotenv()

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

def run_cli(duckdb_cli_path, db_path, query):
    return subprocess.run(
        [duckdb_cli_path, db_path, "-csv", "-c", query],
        capture_output=True,
        text=True,
        check=False,
    )

def get_image_data_for_provider(image_url, provider):
    """
    Get image data in the appropriate format based on the provider.
    OpenAI uses URLs directly, Ollama uses base64 encoding.
    """
    if provider == "openai":
        return image_url
    elif provider == "ollama":
        # Fetch the image and convert to base64
        try:
            response = requests.get(image_url, timeout=10)
            response.raise_for_status()
            image_base64 = base64.b64encode(response.content).decode("utf-8")
            return image_base64
        except Exception as e:
            # Fallback to URL if fetching fails
            print(
                f"Warning: Failed to fetch image {image_url}: {e}. Using URL instead."
            )
            return image_url
    else:
        return image_url
