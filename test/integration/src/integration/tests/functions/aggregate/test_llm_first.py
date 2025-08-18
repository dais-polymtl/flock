import pytest
from integration.conftest import run_cli


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("llama3.2", "ollama")])
def model_config(request):
    """Fixture to test with different models."""
    return request.param


def test_llm_first_basic_functionality(integration_setup, model_config):
    """Test basic llm_first functionality without GROUP BY."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-first-model', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE candidates (
        id INTEGER,
        name VARCHAR,
        experience VARCHAR,
        skills VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO candidates
                        VALUES (1, 'Alice Johnson', '5 years in software development', 'Python, JavaScript, React'),
                               (2, 'Bob Smith', '8 years in data science', 'Python, R, Machine Learning'),
                               (3, 'Carol Davis', '3 years in web development', 'HTML, CSS, JavaScript, Vue.js'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT llm_first(
                       {'model_name': 'test-first-model'},
            {'prompt': 'Which candidate is best suited for a senior software engineer position? Return the ID number only.', 'context_columns': [{'data': name}, {'data': experience}, {'data': skills}]}
        ) AS selected_candidate
            FROM candidates; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "selected_candidate" in result.stdout.lower()
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 2, "Expected at least header and one result row"


def test_llm_first_with_group_by(integration_setup, model_config):
    """Test llm_first with GROUP BY clause."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-first-group', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE job_applications (
        id INTEGER,
        department VARCHAR,
        candidate_name VARCHAR,
        score INTEGER,
        notes VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO job_applications
                        VALUES (1, 'Engineering', 'John Doe', 85, 'Strong technical skills'),
                               (2, 'Engineering', 'Jane Smith', 92, 'Excellent problem solver'),
                               (3, 'Marketing', 'Bob Wilson', 78, 'Good communication skills'),
                               (4, 'Marketing', 'Alice Brown', 88, 'Creative and analytical'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT *
            FROM duckdb_secrets();
            SELECT department,
                   llm_first(
                       {'model_name': 'test-first-group'},
            {'prompt': 'Who is the best candidate for this department? Return the ID number only.', 'context_columns': [{'data': candidate_name}, {'data': score::VARCHAR}, {'data': notes}]}
        ) AS best_candidate_id
            FROM job_applications
            GROUP BY department
            ORDER BY department; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    # Should have header + 2 department groups (Engineering, Marketing)
    assert len(lines) >= 3, f"Expected at least 3 lines, got {len(lines)}"
    assert (
        "engineering" in result.stdout.lower() or "marketing" in result.stdout.lower()
    )


def test_llm_first_with_batch_processing(integration_setup, model_config):
    """Test llm_first with batch size configuration."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-first-batch', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE investment_options (
        id INTEGER,
        name VARCHAR,
        risk_level VARCHAR,
        expected_return DECIMAL,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO investment_options
                        VALUES (1, 'Government Bonds', 'Low', 3.5, 'Safe investment with guaranteed returns'),
                               (2, 'Stock Market Index', 'Medium', 8.2, 'Diversified portfolio with moderate risk'),
                               (3, 'Growth Stocks', 'High', 12.8, 'High potential returns but volatile'),
                               (4, 'Real Estate', 'Medium', 6.5, 'Property investment with steady growth'),
                               (5, 'Cryptocurrency', 'Very High', 15.0, 'Digital assets with extreme volatility'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT llm_first(
                       {'model_name': 'test-first-batch', 'batch_size': 3},
            {'prompt': 'Which investment option is best for a conservative investor? Return the ID number only.', 'context_columns': [{'data': name}, {'data': risk_level}, {'data': expected_return::VARCHAR}, {'data': description}]}
        ) AS best_conservative_investment
            FROM investment_options; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "best_conservative_investment" in result.stdout.lower()


def test_llm_first_with_model_parameters(integration_setup, model_config):
    """Test llm_first with custom model parameters."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-first-params', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE startup_pitches (
        id INTEGER,
        company_name VARCHAR,
        sector VARCHAR,
        funding_request INTEGER,
        team_size INTEGER,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO startup_pitches
                        VALUES (1, 'TechStart AI', 'Technology', 500000, 8,
                                'AI-powered automation platform for small businesses'),
                               (2, 'GreenEnergy Solutions', 'Renewable Energy', 1000000, 12,
                                'Solar panel installation and maintenance service'),
                               (3, 'HealthTrack App', 'Healthcare', 250000, 5,
                                'Personal health monitoring mobile application'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT llm_first(
                       {'model_name': 'test-first-params', 'tuple_format': 'Markdown',
                                                           'model_parameters': '{"temperature": 0.1}'},
            {'prompt': 'Which startup has the most promising business model for investment? Return the ID number only.', 'context_columns': [{'data': company_name}, {'data': sector}, {'data': funding_request::VARCHAR}, {'data': team_size::VARCHAR}, {'data': description}]}
        ) AS most_promising_startup
            FROM startup_pitches; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "most_promising_startup" in result.stdout.lower()


def test_llm_first_multiple_criteria(integration_setup, model_config):
    """Test llm_first with multiple selection criteria."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-first-multi', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE course_options (
        id INTEGER,
        course_name VARCHAR,
        difficulty VARCHAR,
        duration_weeks INTEGER,
        cost INTEGER,
        rating DECIMAL,
        prerequisites VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO course_options
                        VALUES (1, 'Python for Beginners', 'Beginner', 8, 199, 4.8, 'None'),
                               (2, 'Advanced Machine Learning', 'Advanced', 16, 599, 4.9, 'Python, Statistics'),
                               (3, 'Web Development Bootcamp', 'Intermediate', 12, 399, 4.6, 'Basic HTML/CSS'),
                               (4, 'Data Science Fundamentals', 'Intermediate', 10, 299, 4.7, 'Basic programming'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT llm_first(
                       {'model_name': 'test-first-multi'},
            {'prompt': 'Which course is best for someone new to programming with a budget of $300 and 12 weeks available? Return the ID number only.', 'context_columns': [{'data': course_name}, {'data': difficulty}, {'data': duration_weeks::VARCHAR}, {'data': cost::VARCHAR}, {'data': rating::VARCHAR}, {'data': prerequisites}]}
        ) AS best_course_for_beginner
            FROM course_options; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "best_course_for_beginner" in result.stdout.lower()


def test_llm_first_empty_table(integration_setup, model_config):
    """Test llm_first behavior with empty table."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-first-empty', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE empty_candidates (
        id INTEGER,
        name VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = """
            SELECT llm_first(
                       {'model_name': 'test-first-empty'},
            {'prompt': 'Select the best candidate', 'context_columns': [{'data': name}]}
        ) AS selected
            FROM empty_candidates; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    # Should return empty result or no rows
    lines = result.stdout.strip().split("\n")
    assert len(lines) <= 2, "Expected at most header line for empty table"


def test_llm_first_error_handling_invalid_model(integration_setup):
    """Test error handling with non-existent model."""
    duckdb_cli_path, db_path = integration_setup

    create_table_query = """
    CREATE OR REPLACE TABLE test_data (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO test_data
                        VALUES (1, 'Test content'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT llm_first(
                       {'model_name': 'non-existent-model'},
        {'prompt': 'Select the best item', 'context_columns': [{'data': text}]}
    ) AS result
            FROM test_data; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert (
        result.returncode != 0
        or "error" in result.stderr.lower()
        or "Error" in result.stdout
    )


def test_llm_first_error_handling_empty_prompt(integration_setup, model_config):
    """Test error handling with empty prompt."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-first-empty-prompt', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE test_data (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO test_data
                        VALUES (1, 'Test content'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT llm_first(
                       {'model_name': 'test-first-empty-prompt'},
        {'prompt': '', 'context_columns': [{'data': text}]}
    ) AS result
            FROM test_data; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0


def test_llm_first_error_handling_missing_arguments(integration_setup, model_config):
    """Test error handling with missing required arguments."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-first-missing-args', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    # Test with only 1 argument (should fail since llm_first requires 2)
    query = """
    SELECT llm_first(
        {'model_name': 'test-first-missing-args'}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0, "Expected error for missing second argument"


def test_llm_first_with_special_characters(integration_setup, model_config):
    """Test llm_first with special characters and unicode."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-first-unicode', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE international_universities (
        id INTEGER,
        name VARCHAR,
        location VARCHAR,
        ranking INTEGER,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO international_universities
                        VALUES (1, 'École Polytechnique', 'Paris, France 🇫🇷', 15,
                                'Top engineering school with rigorous curriculum'),
                               (2, '東京大学 (University of Tokyo)', 'Tokyo, Japan 🇯🇵', 8,
                                'Premier research university in Asia'),
                               (3, 'MIT', 'Cambridge, MA, USA 🇺🇸', 2, 'Leading technology and innovation institute'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT llm_first(
                       {'model_name': 'test-first-unicode'},
            {'prompt': 'Which university offers the best combination of prestige and innovation for engineering? Return the ID number only.', 'context_columns': [{'data': name}, {'data': location}, {'data': ranking::VARCHAR}, {'data': description}]}
        ) AS top_engineering_university
            FROM international_universities; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "top_engineering_university" in result.stdout.lower()


def _test_llm_first_performance_large_dataset(integration_setup, model_config):
    """Performance test with larger dataset - commented out with underscore prefix for optional execution."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-first-perf', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE large_candidate_pool AS
    SELECT
        i as id,
        'Candidate ' || i as name,
        'Category ' || (i % 3) as category,
        (i % 10) + 1 as score
    FROM range(1, 31) t(i);
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = """
            SELECT category,
                   llm_first(
                       {'model_name': 'test-first-perf', 'batch_size': 5},
            {'prompt': 'Who is the best candidate in this category based on score? Return the ID number only.', 'context_columns': [{'data': name}, {'data': score::VARCHAR}]}
        ) AS best_candidate
            FROM large_candidate_pool
            GROUP BY category
            ORDER BY category LIMIT 3; \
            """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 categories), got {len(lines)}"
    )
    assert "category" in result.stdout.lower()
