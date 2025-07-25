#!/usr/bin/env python3
"""
Test Database Setup Script for FlockMTL Integration Tests

This script creates and manages a persistent test database with pre-configured
models, prompts, and test data to reduce setup time during integration testing.
"""

import os
import subprocess
from pathlib import Path
from dotenv import load_dotenv

# Load environment variables
load_dotenv()

# Configuration
DUCKDB_CLI_PATH = os.getenv("DUCKDB_CLI_PATH", "duckdb")

def run_sql_command(db_path: str, sql_command: str, description: str = ""):
    """Execute SQL command using DuckDB CLI."""
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

def create_base_test_secrets(db_path: str):
    """Create basic test secrets for LLM functions."""
    secrets = {
        "openai": (os.getenv("OPENAI_API_KEY")),
        "ollama": (os.getenv("API_URL", "http://localhost:11434"))
    }
    
    def create_openai_secret(secret_key):
        return f"""CREATE PERSISTENT SECRET IF NOT EXISTS (
            TYPE OPENAI,
            API_KEY '{secret_key}'
        );"""

    def create_ollama_secret(secret_key):
        return f"""CREATE PERSISTENT SECRET IF NOT EXISTS (
            TYPE OLLAMA,
            API_URL '{secret_key}'
        );"""

    print("Creating test secrets...")
    for secret_name, secret_value in secrets.items():
        if secret_name == "openai":
            sql = create_openai_secret(secret_value)
        elif secret_name == "ollama":
            sql = create_ollama_secret(secret_value)
        else:
            continue
        run_sql_command(db_path, sql, f"Secret: {secret_name}")

def setup_test_db(db_path):
    
    create_base_test_secrets(db_path)
