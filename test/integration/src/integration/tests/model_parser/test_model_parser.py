from integration.conftest import run_cli


def test_create_and_get_model(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE MODEL('test-model', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET MODEL 'test-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "test-model" in result.stdout
    assert "gpt-4o" in result.stdout
    assert "openai" in result.stdout
    assert "local" in result.stdout


def test_create_get_delete_global_model(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE GLOBAL MODEL('global-test-model', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET MODEL 'global-test-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "global-test-model" in result.stdout
    assert "gpt-4" in result.stdout
    assert "openai" in result.stdout
    assert "global" in result.stdout
    delete_query = "DELETE MODEL 'global-test-model';"
    run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)


def test_create_local_model_explicit(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE LOCAL MODEL('local-test-model', 'llama2', 'ollama');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET MODEL 'local-test-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "local-test-model" in result.stdout
    assert "llama2" in result.stdout
    assert "ollama" in result.stdout
    assert "local" in result.stdout


def test_create_model_with_args(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE MODEL('model-with-args', 'gpt-4o', 'openai', '{\"batch_size\": 10, \"tuple_format\": \"csv\"}');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET MODEL 'model-with-args';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "model-with-args" in result.stdout
    assert "gpt-4o" in result.stdout
    assert "openai" in result.stdout


def test_delete_model(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE MODEL('delete-test-model', 'gpt-4o', 'azure');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    delete_query = "DELETE MODEL 'delete-test-model';"
    run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)
    get_query = "GET MODEL 'delete-test-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "delete-test-model" not in result.stdout or result.stdout.strip() == ""


def test_update_model_content(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE MODEL('update-test-model', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    update_query = "UPDATE MODEL('update-test-model', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, update_query, with_secrets=False)
    get_query = "GET MODEL 'update-test-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "update-test-model" in result.stdout
    assert "gpt-4" in result.stdout
    assert "openai" in result.stdout


def test_update_model_with_args(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE MODEL('update-args-model', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    update_query = "UPDATE MODEL('update-args-model', 'gpt-4o', 'openai', '{\"batch_size\": 5, \"model_parameters\": {\"temperature\": 0.7}}');"
    run_cli(duckdb_cli_path, db_path, update_query, with_secrets=False)
    get_query = "GET MODEL 'update-args-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "update-args-model" in result.stdout
    assert "gpt-4" in result.stdout


def test_update_model_scope_to_global(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE LOCAL MODEL('scope-test-model', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    update_query = "UPDATE MODEL 'scope-test-model' TO GLOBAL;"
    run_cli(duckdb_cli_path, db_path, update_query, with_secrets=False)
    get_query = "GET MODEL 'scope-test-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "scope-test-model" in result.stdout
    assert "global" in result.stdout
    delete_query = "DELETE MODEL 'scope-test-model';"
    run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)


def test_update_model_scope_to_local(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE GLOBAL MODEL('scope-test-model-2', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    update_query = "UPDATE MODEL 'scope-test-model-2' TO LOCAL;"
    run_cli(duckdb_cli_path, db_path, update_query, with_secrets=False)
    get_query = "GET MODEL 'scope-test-model-2';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "scope-test-model-2" in result.stdout
    assert "local" in result.stdout


def test_get_all_models(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query1 = "CREATE MODEL('model1', 'gpt-4o', 'openai');"
    create_query2 = "CREATE GLOBAL MODEL('model2', 'llama2', 'ollama');"
    run_cli(duckdb_cli_path, db_path, create_query1, with_secrets=False)
    run_cli(duckdb_cli_path, db_path, create_query2, with_secrets=False)
    get_all_query = "GET MODELS;"
    result = run_cli(duckdb_cli_path, db_path, get_all_query, with_secrets=False)
    assert "model1" in result.stdout
    assert "model2" in result.stdout
    assert "gpt-4o" in result.stdout
    assert "llama2" in result.stdout
    delete_query = "DELETE MODEL 'model2';"
    run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)


def test_create_model_duplicate_error(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE MODEL('duplicate-model', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    duplicate_query = "CREATE MODEL('duplicate-model', 'gpt-4o', 'openai');"
    result = run_cli(duckdb_cli_path, db_path, duplicate_query, with_secrets=False)
    assert result.returncode != 0 or "already exist" in result.stderr


def test_create_model_invalid_syntax(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    # Missing opening parenthesis
    invalid_query1 = "CREATE MODEL 'test', 'gpt-4o', 'openai');"
    result1 = run_cli(duckdb_cli_path, db_path, invalid_query1, with_secrets=False)
    assert result1.returncode != 0

    # Missing comma between parameters
    invalid_query2 = "CREATE MODEL('test' 'gpt-4o' 'openai');"
    result2 = run_cli(duckdb_cli_path, db_path, invalid_query2, with_secrets=False)
    assert result2.returncode != 0

    # Missing closing parenthesis
    invalid_query3 = "CREATE MODEL('test', 'gpt-4o', 'openai';"
    result3 = run_cli(duckdb_cli_path, db_path, invalid_query3, with_secrets=False)
    assert result3.returncode != 0


def test_create_model_invalid_json_args(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    # Invalid JSON format
    invalid_query1 = "CREATE MODEL('test-model', 'gpt-4o', 'openai', '{invalid json}');"
    result1 = run_cli(duckdb_cli_path, db_path, invalid_query1, with_secrets=False)
    assert result1.returncode != 0

    # Invalid parameter in JSON
    invalid_query2 = "CREATE MODEL('test-model', 'gpt-4o', 'openai', '{\"invalid_param\": \"value\"}');"
    result2 = run_cli(duckdb_cli_path, db_path, invalid_query2, with_secrets=False)
    assert result2.returncode != 0


def test_delete_nonexistent_model(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    delete_query = "DELETE MODEL 'nonexistent-model';"
    result = run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)
    assert result.returncode == 0


def test_update_nonexistent_model_error(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    update_query = "UPDATE MODEL('nonexistent-model', 'gpt-4o', 'openai');"
    result = run_cli(duckdb_cli_path, db_path, update_query, with_secrets=False)
    assert result.returncode != 0 or "doesn't exist" in result.stderr


def test_get_nonexistent_model(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    get_query = "GET MODEL 'nonexistent-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert result.returncode == 0
    assert "nonexistent-model" not in result.stdout or result.stdout.strip() == ""


def test_empty_model_name_error(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    invalid_query = "CREATE MODEL('', 'gpt-4o', 'openai');"
    result = run_cli(duckdb_cli_path, db_path, invalid_query, with_secrets=False)
    assert result.returncode != 0


def test_empty_model_value_error(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    invalid_query = "CREATE MODEL('test-model', '', 'openai');"
    result = run_cli(duckdb_cli_path, db_path, invalid_query, with_secrets=False)
    assert result.returncode != 0


def test_empty_provider_name_error(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    invalid_query = "CREATE MODEL('test-model', 'gpt-4o', '');"
    result = run_cli(duckdb_cli_path, db_path, invalid_query, with_secrets=False)
    assert result.returncode != 0


def test_get_model_vs_get_models(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE MODEL('test-get-model', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)

    # Test GET MODEL (singular)
    get_single_query = "GET MODEL 'test-get-model';"
    result_single = run_cli(duckdb_cli_path, db_path, get_single_query, with_secrets=False)
    assert "test-get-model" in result_single.stdout

    # Test GET MODELS (plural) - should get all models
    get_all_query = "GET MODELS;"
    result_all = run_cli(duckdb_cli_path, db_path, get_all_query, with_secrets=False)
    assert "test-get-model" in result_all.stdout


def test_model_args_allowed_parameters(integration_setup):
    duckdb_cli_path, db_path = integration_setup

    # Test valid parameters: tuple_format, batch_size, model_parameters
    valid_query = 'CREATE MODEL(\'valid-args-model\', \'gpt-4o\', \'openai\', \'{"tuple_format": "json", "batch_size": 5, "model_parameters": {"temperature": 0.8}}\');'
    result = run_cli(duckdb_cli_path, db_path, valid_query, with_secrets=False)
    assert result.returncode == 0

    get_query = "GET MODEL 'valid-args-model';"
    get_result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "valid-args-model" in get_result.stdout


def test_multiple_providers(integration_setup):
    duckdb_cli_path, db_path = integration_setup

    # Test different providers
    openai_query = "CREATE MODEL('openai-model', 'gpt-4o', 'openai');"
    azure_query = "CREATE MODEL('azure-model', 'gpt-4o', 'azure');"
    ollama_query = "CREATE MODEL('ollama-model', 'llama2', 'ollama');"

    run_cli(duckdb_cli_path, db_path, openai_query, with_secrets=False)
    run_cli(duckdb_cli_path, db_path, azure_query, with_secrets=False)
    run_cli(duckdb_cli_path, db_path, ollama_query, with_secrets=False)

    get_all_query = "GET MODELS;"
    result = run_cli(duckdb_cli_path, db_path, get_all_query, with_secrets=False)
    assert "openai-model" in result.stdout
    assert "azure-model" in result.stdout
    assert "ollama-model" in result.stdout
    assert "openai" in result.stdout
    assert "azure" in result.stdout
    assert "ollama" in result.stdout


# Comment and Semicolon Tests
def test_create_model_without_semicolon(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE MODEL('no-semicolon-model', 'gpt-4o', 'openai')"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET MODEL 'no-semicolon-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "no-semicolon-model" in result.stdout


def test_create_model_with_comment(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = (
        "CREATE MODEL('comment-model', 'gpt-4o', 'openai'); -- This is a comment"
    )
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET MODEL 'comment-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "comment-model" in result.stdout


def test_create_model_with_comment_before(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = """-- Create a test model
CREATE MODEL('comment-before-model', 'gpt-4o', 'openai');"""
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET MODEL 'comment-before-model';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "comment-before-model" in result.stdout


def test_delete_model_without_semicolon(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE MODEL('delete-no-semi', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    delete_query = "DELETE MODEL 'delete-no-semi'"
    run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)
    get_query = "GET MODEL 'delete-no-semi';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "delete-no-semi" not in result.stdout or result.stdout.strip() == ""


def test_get_models_without_semicolon(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE MODEL('get-no-semi', 'gpt-4o', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET MODELS"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "get-no-semi" in result.stdout
