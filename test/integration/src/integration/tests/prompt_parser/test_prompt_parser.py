from integration.conftest import run_cli


def test_create_and_get_prompt(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE PROMPT('test-prompt', 'Test prompt content');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET PROMPT 'test-prompt';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "test-prompt" in result.stdout
    assert "Test prompt content" in result.stdout
    assert "local" in result.stdout


def test_create_get_delete_global_prompt(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE GLOBAL PROMPT('global-test-prompt', 'Global test content');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET PROMPT 'global-test-prompt';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "global-test-prompt" in result.stdout
    assert "Global test content" in result.stdout
    assert "global" in result.stdout
    delete_query = "DELETE PROMPT 'global-test-prompt';"
    run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)


def test_create_local_prompt_explicit(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE LOCAL PROMPT('local-test-prompt', 'Local test content');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET PROMPT 'local-test-prompt';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "local-test-prompt" in result.stdout
    assert "Local test content" in result.stdout
    assert "local" in result.stdout


def test_delete_prompt(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE PROMPT('delete-test-prompt', 'To be deleted');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    delete_query = "DELETE PROMPT 'delete-test-prompt';"
    run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)
    get_query = "GET PROMPT 'delete-test-prompt';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "delete-test-prompt" not in result.stdout or result.stdout.strip() == ""


def test_update_prompt_content(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE PROMPT('update-test-prompt', 'Original content');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    update_query = "UPDATE PROMPT('update-test-prompt', 'Updated content');"
    run_cli(duckdb_cli_path, db_path, update_query, with_secrets=False)
    get_query = "GET PROMPT 'update-test-prompt';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "update-test-prompt" in result.stdout
    assert "Updated content" in result.stdout


def test_update_prompt_scope_to_global(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE LOCAL PROMPT('scope-test-prompt', 'Test content');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    update_query = "UPDATE PROMPT 'scope-test-prompt' TO GLOBAL;"
    run_cli(duckdb_cli_path, db_path, update_query, with_secrets=False)
    get_query = "GET PROMPT 'scope-test-prompt';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "scope-test-prompt" in result.stdout
    assert "global" in result.stdout
    delete_query = "DELETE PROMPT 'scope-test-prompt';"
    run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)


def test_update_prompt_scope_to_local(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE GLOBAL PROMPT('scope-test-prompt-2', 'Test content');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    update_query = "UPDATE PROMPT 'scope-test-prompt-2' TO LOCAL;"
    run_cli(duckdb_cli_path, db_path, update_query, with_secrets=False)
    get_query = "GET PROMPT 'scope-test-prompt-2';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "scope-test-prompt-2" in result.stdout
    assert "local" in result.stdout


def test_get_all_prompts(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query1 = "CREATE PROMPT('prompt1', 'Content 1');"
    create_query2 = "CREATE GLOBAL PROMPT('prompt2', 'Content 2');"
    run_cli(duckdb_cli_path, db_path, create_query1, with_secrets=False)
    run_cli(duckdb_cli_path, db_path, create_query2, with_secrets=False)
    get_all_query = "GET PROMPTS;"
    result = run_cli(duckdb_cli_path, db_path, get_all_query, with_secrets=False)
    assert "prompt1" in result.stdout
    assert "prompt2" in result.stdout
    assert "Content 1" in result.stdout
    assert "Content 2" in result.stdout
    delete_query = "DELETE PROMPT 'prompt2';"
    run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)


def test_create_prompt_duplicate_error(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE PROMPT('duplicate-prompt', 'Original');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    duplicate_query = "CREATE PROMPT('duplicate-prompt', 'Duplicate');"
    result = run_cli(duckdb_cli_path, db_path, duplicate_query, with_secrets=False)
    assert result.returncode != 0 or "already exist" in result.stderr


def test_create_prompt_invalid_syntax(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    invalid_query1 = "CREATE PROMPT 'test', 'content');"
    result1 = run_cli(duckdb_cli_path, db_path, invalid_query1, with_secrets=False)
    assert result1.returncode != 0
    invalid_query2 = "CREATE PROMPT('test' 'content');"
    result2 = run_cli(duckdb_cli_path, db_path, invalid_query2, with_secrets=False)
    assert result2.returncode != 0
    invalid_query3 = "CREATE PROMPT('test', 'content';"
    result3 = run_cli(duckdb_cli_path, db_path, invalid_query3, with_secrets=False)
    assert result3.returncode != 0


def test_delete_nonexistent_prompt(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    delete_query = "DELETE PROMPT 'nonexistent-prompt';"
    result = run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)
    assert result.returncode == 0


def test_update_nonexistent_prompt_error(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    update_query = "UPDATE PROMPT('nonexistent-prompt', 'New content');"
    result = run_cli(duckdb_cli_path, db_path, update_query, with_secrets=False)
    assert result.returncode != 0 or "doesn't exist" in result.stderr


def test_get_nonexistent_prompt(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    get_query = "GET PROMPT 'nonexistent-prompt';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert result.returncode == 0
    assert "nonexistent-prompt" not in result.stdout or result.stdout.strip() == ""


def test_empty_prompt_name_error(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    invalid_query = "CREATE PROMPT('', 'content');"
    result = run_cli(duckdb_cli_path, db_path, invalid_query, with_secrets=False)
    assert result.returncode != 0


def test_empty_prompt_content_error(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    invalid_query = "CREATE PROMPT('test', '');"
    result = run_cli(duckdb_cli_path, db_path, invalid_query, with_secrets=False)
    assert result.returncode != 0


# Comment and Semicolon Tests
def test_create_prompt_without_semicolon(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE PROMPT('no-semi-prompt', 'Test content')"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET PROMPT 'no-semi-prompt';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "no-semi-prompt" in result.stdout


def test_create_prompt_with_comment(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = (
        "CREATE PROMPT('comment-prompt', 'Test content'); -- This is a comment"
    )
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET PROMPT 'comment-prompt';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "comment-prompt" in result.stdout


def test_create_prompt_with_comment_before(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = """-- Create a test prompt
CREATE PROMPT('comment-before-prompt', 'Test content');"""
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET PROMPT 'comment-before-prompt';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "comment-before-prompt" in result.stdout


def test_delete_prompt_without_semicolon(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE PROMPT('delete-no-semi', 'Test content');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    delete_query = "DELETE PROMPT 'delete-no-semi'"
    run_cli(duckdb_cli_path, db_path, delete_query, with_secrets=False)
    get_query = "GET PROMPT 'delete-no-semi';"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "delete-no-semi" not in result.stdout or result.stdout.strip() == ""


def test_get_prompts_without_semicolon(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    create_query = "CREATE PROMPT('get-no-semi', 'Test content');"
    run_cli(duckdb_cli_path, db_path, create_query, with_secrets=False)
    get_query = "GET PROMPTS"
    result = run_cli(duckdb_cli_path, db_path, get_query, with_secrets=False)
    assert "get-no-semi" in result.stdout
