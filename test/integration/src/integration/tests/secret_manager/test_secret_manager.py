import pytest
from integration.conftest import run_cli


def test_create_openai_secret(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_openai_secret"
    create_query = (
        f"CREATE SECRET {secret_name} (TYPE OPENAI, API_KEY 'test-api-key-123');"
    )
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode == 0


def test_create_openai_secret_with_base_url(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_openai_secret_with_url"
    create_query = f"CREATE SECRET {secret_name} (TYPE OPENAI, API_KEY 'test-api-key-123', BASE_URL 'https://api.custom.com');"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode == 0


def test_create_azure_secret(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_azure_secret"
    create_query = f"CREATE SECRET {secret_name} (TYPE AZURE_LLM, API_KEY 'test-azure-key', RESOURCE_NAME 'test-resource', API_VERSION '2023-05-15');"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode == 0


def test_create_ollama_secret(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_ollama_secret"
    create_query = (
        f"CREATE SECRET {secret_name} (TYPE OLLAMA, API_URL 'http://localhost:11434');"
    )
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode == 0


def test_create_anthropic_secret(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_anthropic_secret"
    create_query = (
        f"CREATE SECRET {secret_name} (TYPE ANTHROPIC, API_KEY 'test-anthropic-key');"
    )
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode == 0


def test_create_anthropic_secret_with_api_version(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_anthropic_secret_with_version"
    create_query = (
        f"CREATE SECRET {secret_name} (TYPE ANTHROPIC, API_KEY 'test-anthropic-key', API_VERSION '2024-01-01');"
    )
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode == 0


def test_create_anthropic_secret_missing_required_field(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_anthropic_invalid"
    create_query = (
        f"CREATE SECRET {secret_name} (TYPE ANTHROPIC, API_VERSION '2024-01-01');"
    )
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode != 0


def test_create_openai_secret_missing_required_field(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_openai_invalid"
    create_query = (
        f"CREATE SECRET {secret_name} (TYPE OPENAI, BASE_URL 'https://api.openai.com');"
    )
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode != 0


def test_create_azure_secret_missing_required_fields(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_azure_invalid"
    create_query = f"CREATE SECRET {secret_name} (TYPE AZURE_LLM, RESOURCE_NAME 'test-resource', API_VERSION '2023-05-15');"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode != 0
    create_query = f"CREATE SECRET {secret_name} (TYPE AZURE_LLM, API_KEY 'test-key', API_VERSION '2023-05-15');"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode != 0
    create_query = f"CREATE SECRET {secret_name} (TYPE AZURE_LLM, API_KEY 'test-key', RESOURCE_NAME 'test-resource');"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode != 0


def test_create_ollama_secret_missing_required_field(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_ollama_invalid"
    create_query = f"CREATE SECRET {secret_name} (TYPE OLLAMA);"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode != 0


def test_create_secret_with_unsupported_type(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_unsupported_secret"
    create_query = (
        f"CREATE SECRET {secret_name} (TYPE UNSUPPORTED_TYPE, API_KEY 'test-key');"
    )
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode != 0


def test_create_secret_empty_name(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE SECRET '' (TYPE OPENAI, API_KEY 'test-key');"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode != 0


def test_create_secret_empty_api_key(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_empty_key_secret"
    create_query = f"CREATE SECRET {secret_name} (TYPE OPENAI, API_KEY '');"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode != 0


def test_multiple_secrets_different_types(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secrets = []
    openai_secret = "test_multi_openai"
    create_openai = (
        f"CREATE SECRET {openai_secret} (TYPE OPENAI, API_KEY 'openai-key');"
    )
    result = run_cli(duckdb_cli_path, db_path, create_openai, with_secrets=False)
    assert result.returncode == 0
    secrets.append(openai_secret)
    azure_secret = "test_multi_azure"
    create_azure = f"CREATE SECRET {azure_secret} (TYPE AZURE_LLM, API_KEY 'azure-key', RESOURCE_NAME 'resource', API_VERSION '2023-05-15');"
    result = run_cli(duckdb_cli_path, db_path, create_azure, with_secrets=False)
    assert result.returncode == 0
    secrets.append(azure_secret)
    ollama_secret = "test_multi_ollama"
    create_ollama = f"CREATE SECRET {ollama_secret} (TYPE OLLAMA, API_URL 'http://localhost:11434');"
    result = run_cli(duckdb_cli_path, db_path, create_ollama, with_secrets=False)
    assert result.returncode == 0
    secrets.append(ollama_secret)
    anthropic_secret = "test_multi_anthropic"
    create_anthropic = f"CREATE SECRET {anthropic_secret} (TYPE ANTHROPIC, API_KEY 'anthropic-key');"
    result = run_cli(duckdb_cli_path, db_path, create_anthropic, with_secrets=False)
    assert result.returncode == 0
    secrets.append(anthropic_secret)


def test_secret_scope_handling(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_scope_secret"
    create_query = f"CREATE SECRET {secret_name} (TYPE OPENAI, API_KEY 'test-key');"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode == 0


@pytest.mark.parametrize(
    "provider_type,required_fields",
    [
        ("OPENAI", {"API_KEY": "test-key"}),
        (
            "AZURE_LLM",
            {
                "API_KEY": "test-key",
                "RESOURCE_NAME": "test-resource",
                "API_VERSION": "2023-05-15",
            },
        ),
        ("OLLAMA", {"API_URL": "http://localhost:11434"}),
        ("ANTHROPIC", {"API_KEY": "test-key"}),
        (
            "ANTHROPIC",
            {"API_KEY": "test-key", "API_VERSION": "2024-01-01"},
        ),
    ],
)
def test_create_secrets_parametrized(integration_setup, provider_type, required_fields):
    duckdb_cli_path, db_path = integration_setup
    secret_name = f"test_param_{provider_type.lower()}_secret"
    fields_str = ", ".join(
        [f"{key} '{value}'" for key, value in required_fields.items()]
    )
    create_query = f"CREATE SECRET {secret_name} (TYPE {provider_type}, {fields_str});"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode == 0


def test_persistent_secret_lifecycle(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    secret_name = "test_persistent_lifecycle_secret"

    create_query = f"CREATE PERSISTENT SECRET {secret_name} (TYPE OPENAI, API_KEY 'test-persistent-key');"
    result = run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    assert result.returncode == 0

    check_query = f"SELECT name, type, persistent FROM duckdb_secrets() WHERE name = '{secret_name}';"
    result = run_cli(duckdb_cli_path, db_path, check_query, with_secrets=False)
    assert secret_name in result.stdout
    assert "OPENAI" in result.stdout or "openai" in result.stdout
    assert (
        "1" in result.stdout
        or "true" in result.stdout.lower()
        or "t" in result.stdout.lower()
    )

    drop_query = f"DROP PERSISTENT SECRET {secret_name};"
    result = run_cli(duckdb_cli_path, db_path, drop_query, with_secrets=False)
    assert result.returncode == 0

    check_query = f"SELECT name FROM duckdb_secrets() WHERE name = '{secret_name}';"
    result = run_cli(duckdb_cli_path, db_path, check_query, with_secrets=False)
    assert secret_name not in result.stdout
