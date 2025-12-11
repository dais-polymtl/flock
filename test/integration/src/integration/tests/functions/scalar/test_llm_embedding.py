import pytest
from integration.conftest import run_cli


@pytest.fixture(
    params=[("text-embedding-3-small", "openai"), ("all-minilm", "ollama")]
)
def model_config(request):
    """Fixture to test with different embedding models."""
    return request.param


def test_llm_embedding_basic_functionality(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
            """
        SELECT llm_embedding(
            {'model_name': '"""
            + test_model_name
            + """'},
        {'context_columns': [{'data': 'This is a test document for embedding generation.'}]}
    ) AS embedding;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "embedding" in result.stdout.lower()
    # Check that the result contains array-like structure
    assert "[" in result.stdout and "]" in result.stdout
    assert "," in result.stdout  # Should have multiple values in the array


def test_llm_embedding_with_multiple_text_fields(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-multi-field_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
            """
        SELECT llm_embedding(
            {'model_name': '"""
            + test_model_name
            + """'},
        {'context_columns': [{'data': 'Product Title'}, {'data': 'Product description here'}, {'data': 'Electronics'}]}
    ) AS embedding;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "embedding" in result.stdout.lower()
    # Check that the result contains array-like structure
    assert "[" in result.stdout and "]" in result.stdout


def test_llm_embedding_with_input_columns(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-input_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE documents (
        id INTEGER,
        title VARCHAR,
        content VARCHAR,
        category VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO documents
                        VALUES (1, 'AI Research Paper', 'This paper discusses advances in artificial intelligence',
                                'Research'),
                               (2, 'Cooking Recipe', 'Instructions for making delicious pasta', 'Food'),
                               (3, 'Travel Guide', 'A comprehensive guide to visiting Paris', 'Travel'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
            """
        SELECT 
            title,
            llm_embedding(
                {'model_name': '"""
            + test_model_name
            + """'},
            {'context_columns': [{'data': title || '. ' || content}]}
        ) AS document_embedding
    FROM documents 
    WHERE id <= 2;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 3, (
        f"Expected at least 3 lines (header + 2 data), got {len(lines)}"
    )
    # Both rows should have embeddings
    assert result.stdout.count("[") >= 2
    assert result.stdout.count("]") >= 2


def test_llm_embedding_batch_processing(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-batch_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE product_descriptions (
        id INTEGER,
        product_name VARCHAR,
        description VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO product_descriptions
                        VALUES (1, 'Laptop Pro', 'High-performance laptop for professionals'),
                               (2, 'Smart Phone X', 'Latest smartphone with advanced camera'),
                               (3, 'Wireless Headphones', 'Premium wireless headphones with noise cancellation'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
            """
        SELECT 
            product_name,
            llm_embedding(
                {'model_name': '"""
            + test_model_name
            + """', 'batch_size': 2},
            {'context_columns': [{'data': product_name || ': ' || description}]}
        ) AS product_embedding
    FROM product_descriptions;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 4, (
        f"Expected at least 4 lines (header + 3 data), got {len(lines)}"
    )
    # All three rows should have embeddings
    assert result.stdout.count("[") >= 3
    assert result.stdout.count("]") >= 3


def test_llm_embedding_error_handling_invalid_model(integration_setup):
    duckdb_cli_path, db_path = integration_setup

    query = """
    SELECT llm_embedding(
        {'model_name': 'non-existent-embedding-model'},
        {'context_columns': [{'data': 'Test text for embedding'}]}
    ) AS embedding;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert (
            result.returncode != 0
            or "error" in result.stderr.lower()
            or "Error" in result.stdout
    )


def test_llm_embedding_error_handling_empty_text(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-empty_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
            """
        SELECT llm_embedding(
            {'model_name': '"""
            + test_model_name
            + """'},
        {'context_columns': [{'data': ''}]}
    ) AS embedding;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    # Some providers might handle empty text gracefully, others might error
    # We just check that it doesn't crash the system
    assert "embedding" in result.stdout.lower() or result.returncode != 0


def test_llm_embedding_with_special_characters(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-unicode_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE special_text (
        id INTEGER,
        text VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO special_text
                        VALUES (1, 'Café résumé naïve'),
                               (2, 'Price: $100.99 (50% off!)'),
                               (3, 'Hello 世界 🌍 Testing emoji and unicode'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
            """
        SELECT 
            llm_embedding(
                {'model_name': '"""
            + test_model_name
            + """'},
            {'context_columns': [{'data': text}]}
        ) AS text_embedding
    FROM special_text
    WHERE id = 1;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "[" in result.stdout and "]" in result.stdout


def test_llm_embedding_with_model_params(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-params_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
            """
        SELECT llm_embedding(
            {'model_name': '"""
            + test_model_name
            + """', 'batch_size': 1},
        {'context_columns': [{'data': 'This is a test document for parameter testing.'}]}
    ) AS embedding;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "[" in result.stdout and "]" in result.stdout


def test_llm_embedding_document_similarity_use_case(integration_setup, model_config):
    """Test a realistic use case: embedding documents for similarity search."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-similarity_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE knowledge_base (
        id INTEGER,
        title VARCHAR,
        content VARCHAR,
        section VARCHAR
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO knowledge_base
                        VALUES (1, 'Machine Learning Basics',
                                'Introduction to supervised and unsupervised learning algorithms', 'AI'),
                               (2, 'Data Preprocessing', 'Techniques for cleaning and preparing data for analysis',
                                'Data Science'),
                               (3, 'Neural Networks', 'Deep learning architectures and their applications', 'AI'); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
            """
        SELECT 
            title,
            section,
            llm_embedding(
                {'model_name': '"""
            + test_model_name
            + """'},
            {'context_columns': [{'data': title}, {'data': content}, {'data': section}]}
        ) AS content_embedding
    FROM knowledge_base
    WHERE section = 'AI';
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 3, (
        f"Expected at least 3 lines (header + 2 data), got {len(lines)}"
    )
    # Should have embeddings for both AI-related documents
    assert result.stdout.count("[") >= 2


def test_llm_embedding_concatenated_fields(integration_setup, model_config):
    """Test embedding generation with multiple concatenated fields."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-concat_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE products (
        id INTEGER,
        name VARCHAR,
        description VARCHAR,
        category VARCHAR,
        price DECIMAL(10,2)
    );
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    insert_data_query = """
                        INSERT INTO products
                        VALUES (1, 'Gaming Laptop', 'High-performance laptop for gaming and creative work',
                                'Electronics', 1299.99),
                               (2, 'Office Chair', 'Ergonomic office chair with lumbar support', 'Furniture', 299.99); \
                        """
    run_cli(duckdb_cli_path, db_path, insert_data_query)

    query = (
            """
        SELECT 
            name,
            llm_embedding(
                {'model_name': '"""
            + test_model_name
            + """'},
            {'context_columns': [{'data': name}, {'data': description}, {'data': category}]}
        ) AS product_embedding
    FROM products;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 3, (
        f"Expected at least 3 lines (header + 2 data), got {len(lines)}"
    )
    assert result.stdout.count("[") >= 2
    assert result.stdout.count("]") >= 2


def _llm_embedding_performance_large_dataset(integration_setup, model_config):
    """Performance test with larger dataset (commented out for regular testing)."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-perf_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    create_table_query = """
    CREATE OR REPLACE TABLE large_text_dataset AS
    SELECT
        i as id,
        'Document ' || i as title,
        'This is the content of document number ' || i || '. It contains important information about topic ' || (i % 5) as content
    FROM range(1, 21) t(i);
    """
    run_cli(duckdb_cli_path, db_path, create_table_query)

    query = (
            """
        SELECT
            title,
            llm_embedding(
                {'model_name': '"""
            + test_model_name
            + """', 'batch_size': 5},
            {'context_columns': [{'data': title || '. ' || content}]}
        ) AS document_embedding
    FROM large_text_dataset
    LIMIT 10;
    """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    lines = result.stdout.strip().split("\n")
    assert len(lines) >= 11, (
        f"Expected at least 11 lines (header + 10 data), got {len(lines)}"
    )
    # Should have embeddings for all 10 documents
    assert result.stdout.count("[") >= 10


def test_llm_embedding_error_handling_malformed_input(integration_setup, model_config):
    """Test error handling for malformed input structures."""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-embedding-malformed_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Test with missing required arguments
    query = """
    SELECT llm_embedding(
        {'model_name': 'test-embedding-malformed'}
    ) AS embedding;
    """
    result = run_cli(duckdb_cli_path, db_path, query)

    assert (
            result.returncode != 0
            or "error" in result.stderr.lower()
            or "Error" in result.stdout
    )
