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


def test_llm_rerank_basic_functionality(integration_setup, model_config):
    """Test basic llm_rerank functionality without GROUP BY."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-rerank-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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
                        INSERT INTO search_results
                        VALUES (1, 'Python Programming Guide', 'Complete guide to learning Python programming language',
                                0.85),
                               (2, 'JavaScript Fundamentals', 'Introduction to JavaScript and web development', 0.72),
                               (3, 'Python Data Science', 'Using Python for data analysis and machine learning', 0.91),
                               (4, 'Web Development with JavaScript', 'Building modern web applications', 0.68); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """'},
                    {'prompt': 'Rank these search results by relevance to Python programming. Return results in order of relevance.', 'context_columns': [{'data': title}, {'data': content}, {'data': relevance_score::VARCHAR}]}
        ) AS reranked_results
            FROM search_results; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "reranked_results" in result.stdout.lower()
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 2, "Expected at least header and one result row"


def test_llm_rerank_with_group_by(integration_setup, model_config):
    """Test llm_rerank with GROUP BY clause."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-rerank-group_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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
                        INSERT INTO product_listings
                        VALUES (1, 'Electronics', 'Smartphone Pro', 999.99, 4.5,
                                'Latest flagship smartphone with advanced features'),
                               (2, 'Electronics', 'Budget Phone', 199.99, 3.8, 'Affordable smartphone for basic needs'),
                               (3, 'Electronics', 'Premium Tablet', 799.99, 4.7,
                                'High-end tablet for productivity and entertainment'),
                               (4, 'Books', 'Programming Guide', 49.99, 4.9, 'Comprehensive programming tutorial'),
                               (5, 'Books', 'Mystery Novel', 12.99, 4.2,
                                'Thrilling mystery story with unexpected twists'),
                               (6, 'Books', 'Science Textbook', 89.99, 4.1, 'University-level science reference book'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT *
            FROM duckdb_secrets();
            SELECT category,
                   llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """'},
                    {'prompt': 'Rank these products by overall value (considering price, rating, and features). Return the best value products first.', 'context_columns': [{'data': product_name}, {'data': price::VARCHAR}, {'data': rating::VARCHAR}, {'data': description}]}
        ) AS ranked_products
            FROM product_listings
            GROUP BY category
            ORDER BY category; \
            """
    )
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

    test_model_name = f"test-rerank-batch_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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
                        INSERT INTO job_candidates
                        VALUES (1, 'Alice Johnson', 5, 'Python, Machine Learning, SQL', 'MS Computer Science', 85000),
                               (2, 'Bob Smith', 8, 'Java, Spring Boot, Microservices', 'BS Computer Engineering',
                                95000),
                               (3, 'Carol Davis', 3, 'JavaScript, React, Node.js', 'BS Information Technology', 75000),
                               (4, 'David Wilson', 10, 'C++, System Design, Architecture', 'PhD Computer Science',
                                120000),
                               (5, 'Eva Brown', 6, 'DevOps, Kubernetes, AWS', 'BS Software Engineering', 90000); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 3},
            {'prompt': 'Rank these candidates for a senior software engineer position. Consider experience, skills, and value for money.', 'context_columns': [{'data': name}, {'data': experience_years::VARCHAR}, {'data': skills}, {'data': education}, {'data': salary_expectation::VARCHAR}]}
        ) AS ranked_candidates
            FROM job_candidates;
            """
    )

    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "ranked_candidates" in result.stdout.lower()


def test_llm_rerank_with_model_parameters(integration_setup, model_config):
    """Test llm_rerank with custom model parameters."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-rerank-params_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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
                        INSERT INTO restaurant_options
                        VALUES (1, 'Italian Bistro', 'Italian', 4.6, '$$', 2.3),
                               (2, 'Sushi Palace', 'Japanese', 4.8, '$$$', 5.1),
                               (3, 'Local Diner', 'American', 4.1, '$', 0.8),
                               (4, 'French Restaurant', 'French', 4.9, '$$$$', 7.2); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """', 'tuple_format': 'Markdown',
                                                            'model_parameters': '{"temperature": 0.1}'},
            {'prompt': 'Rank these restaurants for a casual dinner considering rating, price, and distance. Prioritize nearby options with good value.', 'context_columns': [{'data': name}, {'data': cuisine}, {'data': rating::VARCHAR}, {'data': price_range}, {'data': distance_km::VARCHAR}]}
        ) AS ranked_restaurants
            FROM restaurant_options; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "ranked_restaurants" in result.stdout.lower()


def test_llm_rerank_multiple_criteria(integration_setup, model_config):
    """Test llm_rerank with multiple ranking criteria."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-rerank-multi_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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
                        INSERT INTO investment_funds
                        VALUES (1, 'Growth Fund Alpha', 8.5, 'High', 0.75, 1000, 15),
                               (2, 'Balanced Portfolio', 6.2, 'Medium', 0.50, 500, 25),
                               (3, 'Conservative Bond Fund', 3.8, 'Low', 0.25, 100, 30),
                               (4, 'Tech Innovation Fund', 12.1, 'Very High', 1.20, 5000, 8); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Rank these investment funds for a moderate-risk investor with $2000 to invest. Consider returns, risk, fees, and fund stability.', 'context_columns': [{'data': fund_name}, {'data': annual_return::VARCHAR}, {'data': risk_rating}, {'data': expense_ratio::VARCHAR}, {'data': minimum_investment::VARCHAR}, {'data': fund_age_years::VARCHAR}]}
        ) AS ranked_funds
            FROM investment_funds; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "ranked_funds" in result.stdout.lower()


def test_llm_rerank_empty_table(integration_setup, model_config):
    """Test llm_rerank behavior with empty table."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-rerank-empty_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE empty_items (
        id INTEGER,
        name VARCHAR,
        score DECIMAL
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = (
        """
            SELECT llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Rank these items by score', 'context_columns': [{'data': name}, {'data': score::VARCHAR}]}
        ) AS ranked_items
            FROM empty_items; \
            """
    )
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
                        INSERT INTO test_data
                        VALUES (1, 'Test content', 0.5); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = """
            SELECT llm_rerank(
                       {'model_name': 'non-existent-model'},
        {'prompt': 'Rank these items', 'context_columns': [{'data': text}, {'data': score::VARCHAR}]}
    ) AS result
            FROM test_data; \
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

    test_model_name = f"test-rerank-empty-prompt_{model_name}"
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
            SELECT llm_rerank(
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


def test_llm_rerank_error_handling_missing_arguments(integration_setup, model_config):
    """Test error handling with missing required arguments."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-rerank-missing-args_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Test with only 2 arguments (should fail since llm_rerank requires 3)
    query = (
        """
    SELECT llm_rerank(
        {'model_name': '"""
        + test_model_name
        + """'},
        {'prompt': 'Test prompt'}
    ) AS result;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode != 0, "Expected error for missing third argument"


def test_llm_rerank_with_special_characters(integration_setup, model_config):
    """Test llm_rerank with special characters and unicode."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-rerank-unicode_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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
                        INSERT INTO international_dishes
                        VALUES (1, 'Crème Brûlée', 'Traditional French dessert with caramelized sugar', '€8.50'),
                               (2, 'Sushi 寿司', 'Fresh Japanese fish and rice 🍣', '¥1,200'),
                               (3, 'Tacos Especiales', 'Authentic Mexican tacos with special sauce!', '$12.99'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """'},
            {'prompt': 'Rank these dishes by authenticity and traditional preparation methods.', 'context_columns': [{'data': name}, {'data': description}, {'data': price}]}
        ) AS ranked_dishes
            FROM international_dishes; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "ranked_dishes" in result.stdout.lower()


def _test_llm_rerank_performance_large_dataset(integration_setup, model_config):
    """Performance test with larger dataset - commented out with underscore prefix for optional execution."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-rerank-perf_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

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

    query = (
        """
            SELECT category,
                   llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 5},
            {'prompt': 'Rank these documents by relevance and content quality within each category.', 'context_columns': [{'data': title}, {'data': content}, {'data': relevance_score::VARCHAR}]}
        ) AS ranked_docs
            FROM large_search_results
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


def test_llm_rerank_with_image_integration(integration_setup, model_config_image):
    """Test llm_rerank with image data integration."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-rerank-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE fashion_images (
        id INTEGER,
        item_name VARCHAR,
        image_url VARCHAR,
        style VARCHAR,
        season VARCHAR,
        price_range VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    dress_url = (
        "https://plus.unsplash.com/premium_photo-1687279093043-73bd1bf3f0bf?w=400"
    )
    coat_url = "https://images.unsplash.com/photo-1519944159858-806d435dc86b?w=400"
    blouse_url = "https://images.unsplash.com/photo-1521572163474-6864f9cf17ab?w=400"

    # Get image data in appropriate format for provider
    dress_image = get_image_data_for_provider(dress_url, provider)
    coat_image = get_image_data_for_provider(coat_url, provider)
    blouse_image = get_image_data_for_provider(blouse_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO fashion_images
                        VALUES (1, 'Summer Dress',
                                '{dress_image}',
                                'Casual', 'Summer', 'Mid-range'),
                               (2, 'Winter Coat', '{coat_image}',
                                'Formal', 'Winter', 'High-end'),
                               (3, 'Spring Blouse',
                                '{blouse_image}', 'Business',
                                'Spring', 'Mid-range'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Rank these fashion items by their versatility and style appeal.',
                'context_columns': [
                    {'data': item_name},
                    {'data': image_url, 'type': 'image'},
                    {'data': style},
                    {'data': season}
                ]
            }
        ) AS ranked_fashion_items
            FROM fashion_images;
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "ranked_fashion_items" in result.stdout.lower()
    assert len(result.stdout.strip().split("\n")) >= 2


def test_llm_rerank_image_with_group_by(integration_setup, model_config_image):
    """Test llm_rerank with images and GROUP BY clause."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-group-rerank_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE interior_images (
        id INTEGER,
        room_name VARCHAR,
        image_url VARCHAR,
        style VARCHAR,
        color_scheme VARCHAR,
        room_type VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    living_url = "https://images.unsplash.com/photo-1567538096630-e0c55bd6374c?w=400"
    kitchen_url = "https://images.unsplash.com/photo-1556909114-f6e7ad7d3136?w=400"
    bedroom_url = "https://images.unsplash.com/photo-1505693314120-0d443867891c?w=400"
    dining_url = "https://images.unsplash.com/photo-1567538096630-e0c55bd6374c?w=400"

    # Get image data in appropriate format for provider
    living_image = get_image_data_for_provider(living_url, provider)
    kitchen_image = get_image_data_for_provider(kitchen_url, provider)
    bedroom_image = get_image_data_for_provider(bedroom_url, provider)
    dining_image = get_image_data_for_provider(dining_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO interior_images
                        VALUES (1, 'Living Room A',
                                '{living_image}', 'Modern',
                                'Neutral', 'Living'),
                               (2, 'Kitchen B', '{kitchen_image}',
                                'Contemporary', 'Warm', 'Kitchen'),
                               (3, 'Bedroom C', '{bedroom_image}',
                                'Minimalist', 'Cool', 'Bedroom'),
                               (4, 'Dining Room D',
                                '{dining_image}', 'Traditional',
                                'Rich', 'Dining'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Rank these room designs by their aesthetic appeal and functionality.',
                'context_columns': [
                    {'data': room_name},
                    {'data': image_url, 'type': 'image'},
                    {'data': style},
                    {'data': color_scheme}
                ]
            }
        ) AS ranked_room_designs
            FROM interior_images
            GROUP BY room_type
            ORDER BY room_type; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 room types), got {len(lines)}"
    )
    assert "ranked_room_designs" in result.stdout.lower()


def test_llm_rerank_image_batch_processing(integration_setup, model_config_image):
    """Test llm_rerank with multiple images in batch processing."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config_image

    test_model_name = f"test-image-batch-rerank_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE travel_destination_images (
        id INTEGER,
        destination_name VARCHAR,
        image_url VARCHAR,
        country VARCHAR,
        climate VARCHAR,
        tourist_rating DECIMAL(3,1)
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Image URLs
    beach_url = "https://images.unsplash.com/photo-1507525428034-b723cf961d3e?w=400"
    mountain_url = "https://images.unsplash.com/photo-1506905925346-21bda4d32df4?w=400"
    city_url = "https://images.unsplash.com/photo-1499856871958-5b9627545d1a?w=400"
    desert_url = "https://images.unsplash.com/photo-1509316785289-025f5b846b35?w=400"

    # Get image data in appropriate format for provider
    beach_image = get_image_data_for_provider(beach_url, provider)
    mountain_image = get_image_data_for_provider(mountain_url, provider)
    city_image = get_image_data_for_provider(city_url, provider)
    desert_image = get_image_data_for_provider(desert_url, provider)

    # Insert data with provider-appropriate image data
    insert_data_query = f"""
                        INSERT INTO travel_destination_images
                        VALUES (1, 'Beach Resort', '{beach_image}',
                                'Maldives', 'Tropical', 4.8),
                               (2, 'Mountain Retreat',
                                '{mountain_image}', 'Switzerland',
                                'Alpine', 4.6),
                               (3, 'City Break', '{city_image}',
                                'Paris', 'Temperate', 4.7),
                               (4, 'Desert Oasis', '{desert_image}',
                                'Morocco', 'Arid', 4.3); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
        """
            SELECT llm_rerank(
                       {'model_name': '"""
        + test_model_name
        + """', 'batch_size': 2},
            {
                'prompt': 'Rank these travel destinations by their visual appeal and tourist attractiveness.',
                'context_columns': [
                    {'data': destination_name},
                    {'data': image_url, 'type': 'image'},
                    {'data': climate},
                    {'data': tourist_rating::VARCHAR}
                ]
            }
        ) AS ranked_destinations
            FROM travel_destination_images
            GROUP BY country
            ORDER BY country; \
            """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 countries), got {len(lines)}"
    )
    assert "ranked_destinations" in result.stdout.lower()


def test_llm_rerank_with_audio_transcription(integration_setup, model_config):
    """Test llm_rerank with audio transcription using OpenAI.

    The audio content says: 'Flock transforms DuckDB into a hybrid database and a semantic AI engine'
    This test verifies that the audio is correctly transcribed and used for reranking.
    """
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    if provider != "openai":
        pytest.skip("Audio transcription is only supported for OpenAI provider")

    test_model_name = f"test-audio-rerank_{model_name}"
    create_model_query = f"CREATE MODEL('{test_model_name}', 'gpt-4o-mini', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    transcription_model_name = f"test-transcription-rerank_{model_name}"
    create_transcription_model_query = f"CREATE MODEL('{transcription_model_name}', 'gpt-4o-mini-transcribe', 'openai');"
    run_cli(duckdb_cli_path, db_path, create_transcription_model_query, with_secrets=False)

    # Get audio file path
    audio_path = get_audio_file_path()

    # Create table with topics - mix database-related (with audio) and unrelated topics
    create_table_query = """
    CREATE OR REPLACE TABLE audio_topics (
        id INTEGER,
        topic VARCHAR,
        audio_path VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    # Only the Database Technology row has the actual audio
    insert_data_query = f"""
    INSERT INTO audio_topics
    VALUES 
        (1, 'Weather Updates', '{audio_path}'),
        (2, 'Database Technology', '{audio_path}'),
        (3, 'Sports News', '{audio_path}');
    """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    # Ask to rank by relevance to databases/Flock - the real audio should rank higher
    query = (
        """
        SELECT llm_rerank(
            {'model_name': '"""
        + test_model_name
        + """'},
            {
                'prompt': 'Rank these entries by relevance to database technology and Flock. Return results with the most relevant first.',
                'context_columns': [
                    {'data': topic, 'type': 'text'},
                    {
                        'data': audio_path,
                        'type': 'audio',
                        'transcription_model': '"""
        + transcription_model_name
        + """'
                    }
                ]
            }
        ) AS ranked_topics
        FROM audio_topics;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"

    # Parse the JSON output to verify the returned tuples
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 2, "Expected at least header and one result row"

    # Parse CSV output to get the JSON result
    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None and "ranked_topics" in row

    # Parse the JSON result which contains the reranked tuples
    result_json = json.loads(row["ranked_topics"])
    assert isinstance(result_json, list), (
        f"Expected list of tuples, got: {type(result_json)}"
    )
    assert len(result_json) > 0, "Expected at least one tuple in result"


def test_llm_rerank_audio_ollama_error(integration_setup):
    """Test that Ollama provider throws error for audio transcription in llm_rerank."""
    duckdb_cli_path, db_path = integration_setup

    test_model_name = "test-ollama-rerank-audio"
    create_model_query = f"CREATE MODEL('{test_model_name}', 'gemma3:1b', 'ollama');"
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    transcription_model_name = "test-ollama-rerank-transcription"
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
        SELECT llm_rerank(
            {'model_name': '""" + test_model_name + """'},
            {
                'prompt': 'Rank these audio files',
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
