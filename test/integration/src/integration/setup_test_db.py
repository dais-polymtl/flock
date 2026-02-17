#!/usr/bin/env python3

import os
import subprocess
from pathlib import Path
from dotenv import load_dotenv

load_dotenv()

DUCKDB_CLI_PATH = os.getenv("DUCKDB_CLI_PATH", "duckdb")


def run_sql_command(db_path: str, sql_command: str, description: str = ""):
    try:
        result = subprocess.run(
            [DUCKDB_CLI_PATH, db_path, "-c", sql_command],
            capture_output=True,
            text=True,
            check=True,
        )
        if description:
            print(f"✓ {description}")
        return result
    except subprocess.CalledProcessError as e:
        print(f"✗ Failed to execute: {description}")
        print(f"  SQL: {sql_command}")
        print(f"  Error: {e.stderr}")
        return None


def setup_test_db(db_path):
    pass
