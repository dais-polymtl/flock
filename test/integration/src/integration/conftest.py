import os
import subprocess
import pytest
from pathlib import Path
from dotenv import load_dotenv
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
