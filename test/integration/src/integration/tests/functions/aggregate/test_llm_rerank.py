import pytest
from integration.conftest import run_cli


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("llama3.2", "ollama")])
def model_config(request):
    """Fixture to test with different models."""
    return request.param


def test_llm_rerank_basic_functionality(integration_setup, model_config):
    """Test basic llm_rerank functionality without GROUP BY."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-rerank-model', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE search_results (
        id INTEGER,
        title VARCHAR,
        content VARCHAR,
        relevance_score DECIMAL
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO search_results VALUES 
    (1, 'Python Programming Guide', 'Complete guide to learning Python programming language', 0.85),
    (2, 'JavaScript Fundamentals', 'Introduction to JavaScript and web development', 0.72),
    (3, 'Python Data Science', 'Using Python for data analysis and machine learning', 0.91),
    (4, 'Web Development with JavaScript', 'Building modern web applications', 0.68);
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        llm_rerank(
            {'model_name': 'test-rerank-model'},
            {'prompt': 'Rank these search results by relevance to Python programming. Return results in order of relevance.'},
            {'title': title, 'content': content, 'score': relevance_score}
        ) AS reranked_results
    FROM search_results;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "reranked_results" in result.stdout.lower()
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 2, "Expected at least header and one result row"


def test_llm_rerank_with_group_by(integration_setup, model_config):
    """Test llm_rerank with GROUP BY clause."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-rerank-group', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE product_listings (
        id INTEGER,
        category VARCHAR,
        product_name VARCHAR,
        price DECIMAL,
        rating DECIMAL,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO product_listings VALUES 
    (1, 'Electronics', 'Smartphone Pro', 999.99, 4.5, 'Latest flagship smartphone with advanced features'),
    (2, 'Electronics', 'Budget Phone', 199.99, 3.8, 'Affordable smartphone for basic needs'),
    (3, 'Electronics', 'Premium Tablet', 799.99, 4.7, 'High-end tablet for productivity and entertainment'),
    (4, 'Books', 'Programming Guide', 49.99, 4.9, 'Comprehensive programming tutorial'),
    (5, 'Books', 'Mystery Novel', 12.99, 4.2, 'Thrilling mystery story with unexpected twists'),
    (6, 'Books', 'Science Textbook', 89.99, 4.1, 'University-level science reference book');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT * FROM duckdb_secrets();
    SELECT 
        category,
        llm_rerank(
            {'model_name': 'test-rerank-group'},
            {'prompt': 'Rank these products by overall value (considering price, rating, and features). Return the best value products first.'},
            {'name': product_name, 'price': price, 'rating': rating, 'description': description}
        ) AS ranked_products
    FROM product_listings 
    GROUP BY category
    ORDER BY category;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    # Should have header + 2 category groups (Books, Electronics)
    assert len(lines) >= 3, f"Expected at least 3 lines, got {len(lines)}"
    assert "electronics" in result.stdout.lower() or "books" in result.stdout.lower()


def test_llm_rerank_with_batch_processing(integration_setup, model_config):
    """Test llm_rerank with batch size configuration."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-rerank-batch', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE job_candidates (
        id INTEGER,
        name VARCHAR,
        experience_years INTEGER,
        skills VARCHAR,
        education VARCHAR,
        salary_expectation INTEGER
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO job_candidates VALUES 
    (1, 'Alice Johnson', 5, 'Python, Machine Learning, SQL', 'MS Computer Science', 85000),
    (2, 'Bob Smith', 8, 'Java, Spring Boot, Microservices', 'BS Computer Engineering', 95000),
    (3, 'Carol Davis', 3, 'JavaScript, React, Node.js', 'BS Information Technology', 75000),
    (4, 'David Wilson', 10, 'C++, System Design, Architecture', 'PhD Computer Science', 120000),
    (5, 'Eva Brown', 6, 'DevOps, Kubernetes, AWS', 'BS Software Engineering', 90000);
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        llm_rerank(
            {'model_name': 'test-rerank-batch', 'batch_size': 3},
            {'prompt': 'Rank these candidates for a senior software engineer position. Consider experience, skills, and value for money.'},
            {'name': name, 'experience': experience_years, 'skills': skills, 'education': education, 'salary': salary_expectation}
        ) AS ranked_candidates
    FROM job_candidates;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "ranked_candidates" in result.stdout.lower()


def test_llm_rerank_with_model_parameters(integration_setup, model_config):
    """Test llm_rerank with custom model parameters."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-rerank-params', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE restaurant_options (
        id INTEGER,
        name VARCHAR,
        cuisine VARCHAR,
        rating DECIMAL,
        price_range VARCHAR,
        distance_km DECIMAL
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO restaurant_options VALUES 
    (1, 'Italian Bistro', 'Italian', 4.6, '$$', 2.3),
    (2, 'Sushi Palace', 'Japanese', 4.8, '$$$', 5.1),
    (3, 'Local Diner', 'American', 4.1, '$', 0.8),
    (4, 'French Restaurant', 'French', 4.9, '$$$$', 7.2);
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        llm_rerank(
            {'model_name': 'test-rerank-params', 'tuple_format': 'Markdown', 'model_parameters': '{"temperature": 0.1}'},
            {'prompt': 'Rank these restaurants for a casual dinner considering rating, price, and distance. Prioritize nearby options with good value.'},
            {'name': name, 'cuisine': cuisine, 'rating': rating, 'price': price_range, 'distance': distance_km}
        ) AS ranked_restaurants
    FROM restaurant_options;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "ranked_restaurants" in result.stdout.lower()


def test_llm_rerank_multiple_criteria(integration_setup, model_config):
    """Test llm_rerank with multiple ranking criteria."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-rerank-multi', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE investment_funds (
        id INTEGER,
        fund_name VARCHAR,
        annual_return DECIMAL,
        risk_rating VARCHAR,
        expense_ratio DECIMAL,
        minimum_investment INTEGER,
        fund_age_years INTEGER
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO investment_funds VALUES 
    (1, 'Growth Fund Alpha', 8.5, 'High', 0.75, 1000, 15),
    (2, 'Balanced Portfolio', 6.2, 'Medium', 0.50, 500, 25),
    (3, 'Conservative Bond Fund', 3.8, 'Low', 0.25, 100, 30),
    (4, 'Tech Innovation Fund', 12.1, 'Very High', 1.20, 5000, 8);
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        llm_rerank(
            {'model_name': 'test-rerank-multi'},
            {'prompt': 'Rank these investment funds for a moderate-risk investor with $2000 to invest. Consider returns, risk, fees, and fund stability.'},
            {'name': fund_name, 'return': annual_return, 'risk': risk_rating, 'fees': expense_ratio, 'minimum': minimum_investment, 'age': fund_age_years}
        ) AS ranked_funds
    FROM investment_funds;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "ranked_funds" in result.stdout.lower()


def test_llm_rerank_empty_table(integration_setup, model_config):
    """Test llm_rerank behavior with empty table."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-rerank-empty', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE empty_items (
        id INTEGER,
        name VARCHAR,
        score DECIMAL
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = """
    SELECT 
        llm_rerank(
            {'model_name': 'test-rerank-empty'},
            {'prompt': 'Rank these items by score'},
            {'name': name, 'score': score}
        ) AS ranked_items
    FROM empty_items;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    # Should return empty result or no rows
    lines = result.stdout.strip().split("\n")
    assert len(lines) <= 2, "Expected at most header line for empty table"


def test_llm_rerank_error_handling_invalid_model(integration_setup):
    """Test error handling with non-existent model."""
    duckdb_cli_path, db_path = integration_setup

    create_table_query = """
    CREATE OR REPLACE TABLE test_data (
        id INTEGER,
        text VARCHAR,
        score DECIMAL
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO test_data VALUES (1, 'Test content', 0.5);
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT llm_rerank(
        {'model_name': 'non-existent-model'},
        {'prompt': 'Rank these items'},
        {'text': text, 'score': score}
    ) AS result
    FROM test_data;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert (
        result.returncode != 0
        or "error" in result.stderr.lower()
        or "Error" in result.stdout
    )


def test_llm_rerank_error_handling_empty_prompt(integration_setup, model_config):
    """Test error handling with empty prompt."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-rerank-empty-prompt', '{model_name}', '{provider}');"
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
    INSERT INTO test_data VALUES (1, 'Test content');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT llm_rerank(
        {'model_name': 'test-rerank-empty-prompt'},
        {'prompt': ''},
        {'text': text}
    ) AS result
    FROM test_data;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0


def test_llm_rerank_error_handling_missing_arguments(integration_setup, model_config):
    """Test error handling with missing required arguments."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-rerank-missing-args', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    # Test with only 2 arguments (should fail since llm_rerank requires 3)
    query = """
    SELECT llm_rerank(
        {'model_name': 'test-rerank-missing-args'},
        {'prompt': 'Test prompt'}
    ) AS result;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0, "Expected error for missing third argument"


def test_llm_rerank_with_special_characters(integration_setup, model_config):
    """Test llm_rerank with special characters and unicode."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-rerank-unicode', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE international_dishes (
        id INTEGER,
        name VARCHAR,
        description VARCHAR,
        price VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
    INSERT INTO international_dishes VALUES 
    (1, 'Crème Brûlée', 'Traditional French dessert with caramelized sugar', '€8.50'),
    (2, 'Sushi 寿司', 'Fresh Japanese fish and rice 🍣', '¥1,200'),
    (3, 'Tacos Especiales', 'Authentic Mexican tacos with special sauce!', '$12.99');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
    SELECT 
        llm_rerank(
            {'model_name': 'test-rerank-unicode'},
            {'prompt': 'Rank these dishes by authenticity and traditional preparation methods.'},
            {'name': name, 'description': description, 'price': price}
        ) AS ranked_dishes
    FROM international_dishes;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "ranked_dishes" in result.stdout.lower()


def _test_llm_rerank_performance_large_dataset(integration_setup, model_config):
    """Performance test with larger dataset - commented out with underscore prefix for optional execution."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    create_model_query = (
        f"CREATE MODEL('test-rerank-perf', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query)

    create_table_query = """
    CREATE OR REPLACE TABLE large_search_results AS
    SELECT
        i as id,
        'Document ' || i as title,
        'Category ' || (i % 5) as category,
        (i % 10) / 10.0 as relevance_score,
        'Content for document ' || i || ' with various information' as content
    FROM range(1, 26) t(i);
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = """
    SELECT
        category,
        llm_rerank(
            {'model_name': 'test-rerank-perf', 'batch_size': 5},
            {'prompt': 'Rank these documents by relevance and content quality within each category.'},
            {'title': title, 'content': content, 'score': relevance_score}
        ) AS ranked_docs
    FROM large_search_results
    GROUP BY category
    ORDER BY category
    LIMIT 3;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 categories), got {len(lines)}"
    )
    assert "category" in result.stdout.lower()
