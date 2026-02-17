import pytest
import json
import csv
from io import StringIO
from integration.conftest import (
    run_cli,
    get_image_data_for_provider,
    get_audio_file_path,
)

# Expected keywords that should appear when audio is transcribed
# Audio content: "Flock transforms DuckDB into a hybrid database and a semantic AI engine"
AUDIO_EXPECTED_KEYWORDS = ["flock", "duckdb", "database", "semantic", "ai", "hybrid"]


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("gemma3:1b", "ollama")])
def model_config(request):
    """Fixture to test with different models for text-only tests."""
    return request.param


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("gemma3:4b", "ollama")])
def model_config_image(request):
    """Fixture to test with different models for image tests."""
    return request.param


def test_llm_first_basic_functionality(integration_setup, model_config):
    """Test basic llm_first functionality without GROUP BY."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-first-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    r = run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    assert r.returncode == 0, f"Query failed with error: {create_model_query} {r.stderr}"

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

    query = (
        """
                SELECT llm_first(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Which candidate is best suited for a senior software engineer position? Return the ID number only.', 'context_columns': [{'data': name}, {'data': experience}, {'data': skills}]}
        ) AS selected_candidate
            FROM candidates; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)
    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "selected_candidate" in result.stdout.lower()
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 2, "Expected at least header and one result row"


def test_llm_first_with_group_by(integration_setup, model_config):
    """Test llm_first with GROUP BY clause."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-first-group_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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

    query = (
        """
            SELECT *
            FROM duckdb_secrets();
            SELECT department,
                   llm_first(
                       {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Who is the best candidate for this department? Return the ID number only.', 'context_columns': [{'data': candidate_name}, {'data': score::VARCHAR}, {'data': notes}]}
        ) AS best_candidate_id
            FROM job_applications
            GROUP BY department
            ORDER BY department; \
            """
    )
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

    test_model_name = f"test-first-batch_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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

    query = (
        """
                SELECT llm_first(
                           {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 3},
            {'prompt': 'Which investment option is best for a conservative investor? Return the ID number only.', 'context_columns': [{'data': name}, {'data': risk_level}, {'data': expected_return::VARCHAR}, {'data': description}]}
        ) AS best_conservative_investment
            FROM investment_options; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "best_conservative_investment" in result.stdout.lower()


def test_llm_first_with_model_parameters(integration_setup, model_config):
    """Test llm_first with custom model parameters."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-first-params_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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

    query = (
        """
                SELECT llm_first(
                           {'model_name': '"""
        + test_model_name
        + """', 'tuple_format': 'Markdown',
                                                           'model_parameters': '{"temperature": 0.1}'},
            {'prompt': 'Which startup has the most promising business model for investment? Return the ID number only.', 'context_columns': [{'data': company_name}, {'data': sector}, {'data': funding_request::VARCHAR}, {'data': team_size::VARCHAR}, {'data': description}]}
        ) AS most_promising_startup
            FROM startup_pitches; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "most_promising_startup" in result.stdout.lower()


def test_llm_first_multiple_criteria(integration_setup, model_config):
    """Test llm_first with multiple selection criteria."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-first-multi_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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

    query = (
        """
                SELECT llm_first(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Which course is best for someone new to programming with a budget of $300 and 12 weeks available? Return the ID number only.', 'context_columns': [{'data': course_name}, {'data': difficulty}, {'data': duration_weeks::VARCHAR}, {'data': cost::VARCHAR}, {'data': rating::VARCHAR}, {'data': prerequisites}]}
        ) AS best_course_for_beginner
            FROM course_options; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "best_course_for_beginner" in result.stdout.lower()


def test_llm_first_empty_table(integration_setup, model_config):
    """Test llm_first behavior with empty table."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-first-empty_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE empty_candidates (
        id INTEGER,
        name VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = (
        """
                SELECT llm_first(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Select the best candidate', 'context_columns': [{'data': name}]}
        ) AS selected
            FROM empty_candidates; \
            """
    )
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

    test_model_name = f"test-first-empty-prompt_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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

    query = (
        """
                SELECT llm_first(
                           {'model_name': '"""
        + test_model_name
        + """'},
        {'prompt': '', 'context_columns': [{'data': text}]}
    ) AS result
            FROM test_data; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0


def test_llm_first_error_handling_missing_arguments(integration_setup, model_config):
    """Test error handling with missing required arguments."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-first-missing-args_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Test with only 1 argument (should fail since llm_first requires 2)
    query = (
        """
        SELECT llm_first(
            {'model_name': '"""
        + test_model_name
        + """'}
    ) AS result;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0, "Expected error for missing second argument"


def test_llm_first_with_special_characters(integration_setup, model_config):
    """Test llm_first with special characters and unicode."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-first-unicode_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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

    query = (
        """
                SELECT llm_first(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Which university offers the best combination of prestige and innovation for engineering? Return the ID number only.', 'context_columns': [{'data': name}, {'data': location}, {'data': ranking::VARCHAR}, {'data': description}]}
        ) AS top_engineering_university
            FROM international_universities; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "top_engineering_university" in result.stdout.lower()


def _test_llm_first_performance_large_dataset(integration_setup, model_config):
    """Performance test with larger dataset - commented out with underscore prefix for optional execution."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-first-perf_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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

    query = (
        """
                SELECT category,
                       llm_first(
                           {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 5},
            {'prompt': 'Who is the best candidate in this category based on score? Return the ID number only.', 'context_columns': [{'data': name}, {'data': score::VARCHAR}]}
        ) AS best_candidate
            FROM large_candidate_pool
            GROUP BY category
            ORDER BY category LIMIT 3; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 categories), got {len(lines)}"
    )
    assert "category" in result.stdout.lower()


def test_llm_first_with_image_integration(integration_setup, model_config_image):
    """Test llm_first with image data integration."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-first-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE pet_images (
        id INTEGER,
        pet_name VARCHAR,
        image_url VARCHAR,
        breed VARCHAR,
        age INTEGER
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    buddy_url = "https://images.unsplash.com/photo-1552053831-71594a27632d?w=400"
    whiskers_url = "https://images.unsplash.com/photo-1514888286974-6c03e2ca1dba?w=400"
    max_url = "https://images.unsplash.com/photo-1548199973-03cce0bbc87b?w=400"

    # Get image data in appropriate format for provider
    buddy_image = get_image_data_for_provider(buddy_url, provider)
    whiskers_image = get_image_data_for_provider(whiskers_url, provider)
    max_image = get_image_data_for_provider(max_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO pet_images
                        VALUES (1, 'Buddy', '{buddy_image}',
                                'Golden Retriever', 3),
                               (2, 'Whiskers', '{whiskers_image}',
                                'Persian Cat', 2),
                               (3, 'Max', '{max_image}',
                                'German Shepherd', 4); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
                SELECT llm_first(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Which pet image shows the youngest animal? Return the pet name only.',
                'context_columns': [
                    {'data': pet_name},
                    {'data': image_url, 'type': 'image'},
                    {'data': age::VARCHAR}
                ]
            }
        ) AS youngest_pet
            FROM pet_images;
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "youngest_pet" in result.stdout.lower()
    assert len(result.stdout.strip().split("\n")) >= 2


def test_llm_first_image_with_group_by(integration_setup, model_config_image):
    """Test llm_first with images and GROUP BY clause."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-group-first_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE artwork_images (
        id INTEGER,
        title VARCHAR,
        image_url VARCHAR,
        artist VARCHAR,
        style VARCHAR,
        year_created INTEGER
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    abstract_url = "https://images.unsplash.com/photo-1541961017774-22349e4a1262?w=400"
    landscape_url = "https://images.unsplash.com/photo-1506905925346-21bda4d32df4?w=400"
    portrait_url = "https://images.unsplash.com/photo-1507003211169-0a1dd7228f2d?w=400"
    sculpture_url = "https://images.unsplash.com/photo-1672343385650-8d5bb804580a?w=400"

    # Get image data in appropriate format for provider
    abstract_image = get_image_data_for_provider(abstract_url, provider)
    landscape_image = get_image_data_for_provider(landscape_url, provider)
    portrait_image = get_image_data_for_provider(portrait_url, provider)
    sculpture_image = get_image_data_for_provider(sculpture_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO artwork_images
                        VALUES (1, 'Abstract Composition',
                                '{abstract_image}', 'Modern Artist',
                                'Abstract', 2020),
                               (2, 'Landscape Painting',
                                '{landscape_image}', 'Nature Painter',
                                'Realistic', 2019),
                               (3, 'Portrait Study',
                                '{portrait_image}', 'Portrait Artist',
                                'Realistic', 2021),
                               (4, 'Modern Sculpture',
                                '{sculpture_image}', 'Sculptor',
                                'Contemporary', 2022);
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
                SELECT llm_first(
                           {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Which artwork in this style is the most recent? Return the title only.',
                'context_columns': [
                    {'data': title},
                    {'data': image_url, 'type': 'image'},
                    {'data': year_created::VARCHAR}
                ]
            }
        ) AS most_recent_artwork
            FROM artwork_images
            GROUP BY style;
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, (
        f"Query failed with error: {result.stdout} {result.stderr}"
    )
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 styles), got {len(lines)}"
    )
    assert "most_recent_artwork" in result.stdout.lower()


def test_llm_first_image_batch_processing(integration_setup, model_config_image):
    """Test llm_first with multiple images in batch processing."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-batch-first_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE building_images (
        id INTEGER,
        building_name VARCHAR,
        image_url VARCHAR,
        city VARCHAR,
        height_meters INTEGER,
        year_built INTEGER
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    skyscraper_url = (
        "https://images.unsplash.com/photo-1486406146926-c627a92ad1ab?w=400"
    )
    office_url = "https://images.unsplash.com/photo-1497366216548-37526070297c?w=400"
    modern_url = "https://images.unsplash.com/photo-1486406146926-c627a92ad1ab?w=400"
    corporate_url = "https://images.unsplash.com/photo-1497366216548-37526070297c?w=400"

    # Get image data in appropriate format for provider
    skyscraper_image = get_image_data_for_provider(skyscraper_url, provider)
    office_image = get_image_data_for_provider(office_url, provider)
    modern_image = get_image_data_for_provider(modern_url, provider)
    corporate_image = get_image_data_for_provider(corporate_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO building_images
                        VALUES (1, 'Skyscraper A', '{skyscraper_image}',
                                'New York', 300, 2015),
                               (2, 'Office Tower', '{office_image}',
                                'Chicago', 250, 2018),
                               (3, 'Modern Complex',
                                '{modern_image}', 'Los Angeles',
                                200, 2020),
                               (4, 'Corporate Center',
                                '{corporate_image}', 'Miami', 180,
                                2019);
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT city,
               llm_first(
                   {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 2},
            {
                'prompt': 'Which building in this city has the highest height? Return the building name only.',
                'context_columns': [
                    {'data': building_name},
                    {'data': image_url, 'type': 'image'},
                    {'data': height_meters::VARCHAR}
                ]
            }
        ) AS tallest_building
            FROM building_images
            GROUP BY city
            ORDER BY city;
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 cities), got {len(lines)}"
    )
    assert "tallest_building" in result.stdout.lower()


def test_llm_first_with_audio_transcription(integration_setup, model_config):
    """Test llm_first with audio transcription using OpenAI."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    if provider != "openai":
        pytest.skip("Audio transcription is only supported for OpenAI provider")

    test_model_name = f"test-audio-first_{model_name}"
    create_model_query = f"CREATE MODEL('{test_model_name}', 'gpt-4o-mini', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    transcription_model_name = f"test-transcription-first_{model_name}"
    create_transcription_model_query = f"CREATE MODEL('{transcription_model_name}', 'gpt-4o-mini-transcribe', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_transcription_model_query, with_secrets=False)

    # Get audio file path
    audio_path = get_audio_file_path()

    create_table_query = """
    CREATE OR REPLACE TABLE audio_descriptions (
        id INTEGER,
        audio_path VARCHAR,
        topic VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Both rows have the same audio about Flock/DuckDB
    insert_data_query = f"""
    INSERT INTO audio_descriptions
    VALUES 
        (0, '{audio_path}', 'Database'),
        (1, '{audio_path}', 'AI');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
        SELECT llm_first(
            {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Based on the audio content, which description best relates to database technology? Return the ID number (0 or 1) only.',
                'context_columns': [
                    {
                        'data': audio_path,
                        'type': 'audio',
                        'transcription_model': '"""
        + transcription_model_name
        + """'
                    },
                    {'data': topic}
                ]
            }
        ) AS selected_id
        FROM audio_descriptions;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "selected_id" in result.stdout.lower()

    # Parse the JSON output to verify the returned tuple
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 2, "Expected at least header and one result row"

    # Parse CSV output to get the JSON result
    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None and "selected_id" in row

    # Parse the JSON result which contains the tuple data
    result_json = json.loads(row["selected_id"])
    assert isinstance(result_json, list), (
        f"Expected list of tuples, got: {type(result_json)}"
    )
    assert len(result_json) > 0, "Expected at least one tuple in result"


def test_llm_first_audio_ollama_error(integration_setup):
    """Test that Ollama provider throws error for audio transcription in llm_first."""
    duckdb_cli_path, db_path = integration_setup

    test_model_name = "test-ollama-first-audio"
    create_model_query = f"CREATE MODEL('{test_model_name}', 'gemma3:1b', 'ollama');"
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    transcription_model_name = "test-ollama-first-transcription"
    create_transcription_model_query = f"CREATE MODEL('{transcription_model_name}', 'gemma3:1b', 'ollama');"
    run_cli(duckdb_cli_path, db_path, create_transcription_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE test_audio (
        id INTEGER,
        audio_url VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO test_audio VALUES 
        (1, 'https://example.com/audio1.mp3'),
        (2, 'https://example.com/audio2.mp3');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
        SELECT llm_first(
            {'model_name': '""" + test_model_name + """'},
            {
                'prompt': 'Select the best audio. Return ID only.',
                'context_columns': [
                    {
                        'data': audio_url,
                        'type': 'audio',
                        'transcription_model': '""" + transcription_model_name + """'
                    }
                ]
            }
        ) AS result
        FROM test_audio;
        """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0
    assert (
        "ollama" in result.stderr.lower()
        or "transcription" in result.stderr.lower()
        or "not supported" in result.stderr.lower()
    )
