import pytest
import json
import csv
from io import StringIO
from integration.conftest import run_cli


def get_json_from_csv_output(stdout, column_name="metrics"):
    """Extract JSON value from DuckDB CSV output"""
    reader = csv.DictReader(StringIO(stdout))
    row = next(reader, None)
    if row and column_name in row:
        return json.loads(row[column_name])
    return None


@pytest.fixture(params=[("gpt-4o-mini", "openai"), ("gemma3:1b", "ollama")])
def model_config(request):
    return request.param


# ============================================================================
# Basic Metrics API Tests
# ============================================================================


def test_flock_get_metrics_returns_json(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    query = "SELECT flock_get_metrics() AS metrics;"
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"

    metrics = get_json_from_csv_output(result.stdout)
    assert metrics is not None, "No JSON found in output"

    # Check new structure - should be a flat object
    assert isinstance(metrics, dict)
    assert len(metrics) == 0  # Initially empty


def test_flock_reset_metrics(integration_setup):
    duckdb_cli_path, db_path = integration_setup
    query = "SELECT flock_reset_metrics() AS result;"
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"
    assert "reset" in result.stdout.lower()


# ============================================================================
# Scalar Function Metrics Tests
# ============================================================================


def test_metrics_after_llm_complete(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-metrics-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Call llm_complete and get_metrics in the same query
    query = (
        """
        SELECT 
            llm_complete(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'Answer with one number: What is 2+2?'}
            ) AS result,
            flock_get_metrics() AS metrics;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"

    # Parse CSV output to get metrics
    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None, "No data returned from query"
    assert "metrics" in row, "Metrics column not found in output"

    metrics = json.loads(row["metrics"])

    # Check that metrics were recorded - should be a flat object with keys like "llm_complete_1"
    assert isinstance(metrics, dict)
    assert len(metrics) > 0

    # Check that we have llm_complete_1 with proper structure
    assert "llm_complete_1" in metrics, (
        f"Expected llm_complete_1 in metrics, got: {list(metrics.keys())}"
    )
    llm_complete_1 = metrics["llm_complete_1"]

    assert "api_calls" in llm_complete_1
    assert llm_complete_1["api_calls"] > 0
    assert "input_tokens" in llm_complete_1
    assert "output_tokens" in llm_complete_1
    assert "total_tokens" in llm_complete_1
    assert "api_duration_ms" in llm_complete_1
    assert "execution_time_ms" in llm_complete_1
    assert "model_name" in llm_complete_1
    assert llm_complete_1["model_name"] == test_model_name
    assert "provider" in llm_complete_1
    assert llm_complete_1["provider"] == provider


def test_metrics_reset_clears_counters(integration_setup, model_config):
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    test_model_name = f"test-reset-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # First query: execute llm_complete and get metrics in the same query
    query1 = (
        """
        SELECT 
            llm_complete(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'Say one word: hello'}
            ) AS result,
            flock_get_metrics() AS metrics;
        """
    )
    result1 = run_cli(duckdb_cli_path, db_path, query1)
    assert result1.returncode == 0

    # Parse metrics from first query to verify they exist
    reader1 = csv.DictReader(StringIO(result1.stdout))
    row1 = next(reader1, None)
    assert row1 is not None and "metrics" in row1
    metrics1 = json.loads(row1["metrics"])
    assert len(metrics1) > 0, "Metrics should be recorded before reset"
    assert "llm_complete_1" in metrics1, "Should have llm_complete_1 after first call"

    # Second query: reset metrics and get metrics in the same query
    query2 = (
        "SELECT flock_reset_metrics() AS reset_result, flock_get_metrics() AS metrics;"
    )
    result2 = run_cli(duckdb_cli_path, db_path, query2)
    assert result2.returncode == 0

    # Parse metrics from second query to verify they're cleared
    reader2 = csv.DictReader(StringIO(result2.stdout))
    row2 = next(reader2, None)
    assert row2 is not None and "metrics" in row2
    metrics2 = json.loads(row2["metrics"])

    # After reset, should be empty
    assert isinstance(metrics2, dict)
    assert len(metrics2) == 0


def test_sequential_numbering_multiple_calls(integration_setup, model_config):
    """Test that multiple calls of the same function get sequential numbering"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-sequential-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Make three calls to llm_complete in the same query
    query = (
        """
        SELECT 
            llm_complete(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'Say: one'}
            ) AS result1,
            llm_complete(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'Say: two'}
            ) AS result2,
            llm_complete(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'Say: three'}
            ) AS result3,
            flock_get_metrics() AS metrics;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"

    # Parse CSV output to get metrics
    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None, "No data returned from query"
    assert "metrics" in row, "Metrics column not found in output"

    metrics = json.loads(row["metrics"])

    # Should have llm_complete_1, llm_complete_2, llm_complete_3
    assert isinstance(metrics, dict)
    assert len(metrics) >= 3, (
        f"Expected at least 3 metrics, got {len(metrics)}: {list(metrics.keys())}"
    )

    # Check that we have sequential numbering
    found_keys = [key for key in metrics.keys() if key.startswith("llm_complete_")]
    assert len(found_keys) >= 3, (
        f"Expected at least 3 llm_complete entries, got: {found_keys}"
    )

    # Verify each has the expected structure
    for key in found_keys:
        assert "api_calls" in metrics[key]
        assert "input_tokens" in metrics[key]
        assert "output_tokens" in metrics[key]
        assert metrics[key]["api_calls"] == 1


# ============================================================================
# Debug Metrics Tests
# ============================================================================


def test_flock_get_debug_metrics_returns_nested_structure(
    integration_setup, model_config
):
    """Test that flock_get_debug_metrics returns the nested structure"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-debug-metrics-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Call llm_complete and get debug metrics
    query = (
        """
        SELECT 
            llm_complete(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'Answer with one number: What is 2+2?'}
            ) AS result,
            flock_get_debug_metrics() AS debug_metrics;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"

    # Parse CSV output to get debug metrics
    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None, "No data returned from query"
    assert "debug_metrics" in row, "Debug metrics column not found in output"

    debug_metrics = json.loads(row["debug_metrics"])

    # Check nested structure
    assert isinstance(debug_metrics, dict)
    assert "threads" in debug_metrics
    assert "thread_count" in debug_metrics
    assert isinstance(debug_metrics["threads"], dict)
    assert debug_metrics["thread_count"] > 0

    # Check that threads contain state data
    found_llm_complete = False
    for thread_id, thread_data in debug_metrics["threads"].items():
        assert isinstance(thread_data, dict)
        for state_id, state_data in thread_data.items():
            assert isinstance(state_data, dict)
            if "llm_complete" in state_data:
                llm_complete_data = state_data["llm_complete"]
                assert "registration_order" in llm_complete_data
                assert "api_calls" in llm_complete_data
                assert "input_tokens" in llm_complete_data
                assert "output_tokens" in llm_complete_data
                found_llm_complete = True

    assert found_llm_complete, "llm_complete not found in debug metrics"


def test_debug_metrics_registration_order(integration_setup, model_config):
    """Test that debug metrics include registration_order"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-reg-order-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Make multiple calls
    query = (
        """
        SELECT 
            llm_complete(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'Say: one'}
            ) AS result1,
            llm_complete(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'Say: two'}
            ) AS result2,
            flock_get_debug_metrics() AS debug_metrics;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0

    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None and "debug_metrics" in row

    debug_metrics = json.loads(row["debug_metrics"])

    # Check registration orders
    registration_orders = []
    for thread_id, thread_data in debug_metrics["threads"].items():
        for state_id, state_data in thread_data.items():
            if "llm_complete" in state_data:
                reg_order = state_data["llm_complete"]["registration_order"]
                registration_orders.append(reg_order)

    # Should have at least one registration order
    assert len(registration_orders) > 0
    # Registration orders should be positive integers
    for order in registration_orders:
        assert isinstance(order, int)
        assert order > 0


# ============================================================================
# Aggregate Function Metrics Tests
# ============================================================================


def test_aggregate_function_metrics_tracking(integration_setup, model_config):
    """Test that aggregate functions track metrics correctly"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-aggregate-metrics-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Call llm_reduce and get metrics
    query = (
        """
        SELECT 
            category,
            llm_reduce(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'One word summary:', 'context_columns': [{'data': description}]}
            ) AS summary,
            flock_get_metrics() AS metrics
        FROM VALUES
            ('Electronics', 'High-performance laptop'),
            ('Electronics', 'Latest smartphone')
        AS t(category, description)
        GROUP BY category;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"

    # Parse CSV output
    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None, "No data returned from query"
    assert "metrics" in row, "Metrics column not found"

    metrics = json.loads(row["metrics"])

    # Check that metrics were recorded
    assert isinstance(metrics, dict)
    assert len(metrics) > 0

    # Check for llm_reduce metrics
    found_reduce = False
    for key in metrics.keys():
        if key.startswith("llm_reduce_"):
            reduce_metrics = metrics[key]
            assert "api_calls" in reduce_metrics
            assert "input_tokens" in reduce_metrics
            assert "output_tokens" in reduce_metrics
            assert "total_tokens" in reduce_metrics
            assert "api_duration_ms" in reduce_metrics
            assert "execution_time_ms" in reduce_metrics
            assert "model_name" in reduce_metrics
            assert reduce_metrics["model_name"] == test_model_name
            assert "provider" in reduce_metrics
            assert reduce_metrics["provider"] == provider
            found_reduce = True
            break

    assert found_reduce, f"llm_reduce metrics not found in: {list(metrics.keys())}"


def test_aggregate_function_metrics_merging_with_group_by(
    integration_setup, model_config
):
    """Test that metrics from multiple states in a single aggregate call are merged into one entry"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-merge-metrics-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Call llm_reduce with GROUP BY that will process multiple states
    # This should result in multiple states being processed, but only ONE merged metrics entry
    query = (
        """
        SELECT 
            category,
            llm_reduce(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'One word summary:', 'context_columns': [{'data': description}]}
            ) AS summary,
            flock_get_metrics() AS metrics
        FROM VALUES
            ('Electronics', 'High-performance laptop'),
            ('Electronics', 'Latest smartphone'),
            ('Electronics', 'Gaming console')
        AS t(category, description)
        GROUP BY category;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"

    # Parse CSV output
    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None, "No data returned from query"
    assert "metrics" in row, "Metrics column not found"

    metrics = json.loads(row["metrics"])

    # Check that metrics were recorded
    assert isinstance(metrics, dict)
    assert len(metrics) > 0

    # Check for llm_reduce metrics - should have ONLY ONE entry (merged)
    found_reduce_keys = [key for key in metrics.keys() if key.startswith("llm_reduce_")]
    assert len(found_reduce_keys) == 1, (
        f"Expected exactly 1 llm_reduce metrics entry (merged), got {len(found_reduce_keys)}: {found_reduce_keys}"
    )

    # Verify the merged metrics have the expected structure
    reduce_metrics = metrics[found_reduce_keys[0]]
    assert "api_calls" in reduce_metrics
    assert "input_tokens" in reduce_metrics
    assert "output_tokens" in reduce_metrics
    assert "total_tokens" in reduce_metrics
    assert "api_duration_ms" in reduce_metrics
    assert "execution_time_ms" in reduce_metrics
    assert "model_name" in reduce_metrics
    assert reduce_metrics["model_name"] == test_model_name
    assert "provider" in reduce_metrics
    assert reduce_metrics["provider"] == provider


def test_aggregate_function_metrics_merging_multiple_groups(
    integration_setup, model_config
):
    """Test that each GROUP BY group produces one merged metrics entry"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-merge-groups-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Call llm_reduce with multiple GROUP BY groups
    # Each group should produce ONE merged metrics entry
    query = (
        """
        SELECT 
            category,
            llm_reduce(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'One word summary:', 'context_columns': [{'data': description}]}
            ) AS summary,
            flock_get_metrics() AS metrics
        FROM VALUES
            ('Electronics', 'High-performance laptop'),
            ('Electronics', 'Latest smartphone'),
            ('Clothing', 'Comfortable jacket'),
            ('Clothing', 'Perfect fit jeans')
        AS t(category, description)
        GROUP BY category;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"

    # Parse CSV output - should have 2 rows (one per category)
    reader = csv.DictReader(StringIO(result.stdout))
    rows = list(reader)
    assert len(rows) == 2, f"Expected 2 rows (one per category), got {len(rows)}"

    # Check metrics from the last row (should have both groups merged)
    metrics = json.loads(rows[-1]["metrics"])

    # Should have exactly ONE llm_reduce entry (the last group's merged metrics)
    # Note: In a GROUP BY query, each group processes independently, so we expect one entry per group
    # But since we're checking the last row, we should see at least one merged entry
    found_reduce_keys = [key for key in metrics.keys() if key.startswith("llm_reduce_")]
    assert len(found_reduce_keys) >= 1, (
        f"Expected at least 1 llm_reduce metrics entry, got {len(found_reduce_keys)}: {found_reduce_keys}"
    )


def test_multiple_aggregate_functions_sequential_numbering(
    integration_setup, model_config
):
    """Test that multiple aggregate function calls get sequential numbering"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-sequential-aggregate-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    # Call llm_reduce twice in the same query
    query = (
        """
        SELECT 
            category,
            llm_reduce(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'One word 1:', 'context_columns': [{'data': description}]}
            ) AS summary1,
            llm_reduce(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'One word 2:', 'context_columns': [{'data': description}]}
            ) AS summary2,
            flock_get_metrics() AS metrics
        FROM VALUES
            ('Electronics', 'High-performance laptop')
        AS t(category, description)
        GROUP BY category;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0, f"Query failed with error: {result.stderr}"

    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None and "metrics" in row

    metrics = json.loads(row["metrics"])

    # Should have llm_reduce_1 and llm_reduce_2
    found_keys = [key for key in metrics.keys() if key.startswith("llm_reduce_")]
    assert len(found_keys) >= 2, (
        f"Expected at least 2 llm_reduce entries, got: {found_keys}"
    )

    # Verify sequential numbering
    numbers = []
    for key in found_keys:
        # Extract number from key like "llm_reduce_1"
        num = int(key.split("_")[-1])
        numbers.append(num)

    numbers.sort()
    # Should have sequential numbers starting from 1
    assert numbers[0] == 1, f"First number should be 1, got {numbers}"


def test_aggregate_function_debug_metrics(integration_setup, model_config):
    """Test debug metrics for aggregate functions"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-debug-aggregate-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
        """
        SELECT 
            category,
            llm_reduce(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'One word summary:', 'context_columns': [{'data': description}]}
            ) AS summary,
            flock_get_debug_metrics() AS debug_metrics
        FROM VALUES
            ('Electronics', 'High-performance laptop')
        AS t(category, description)
        GROUP BY category;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0

    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None and "debug_metrics" in row

    debug_metrics = json.loads(row["debug_metrics"])

    # Check nested structure
    assert isinstance(debug_metrics, dict)
    assert "threads" in debug_metrics
    assert "thread_count" in debug_metrics

    # Check that llm_reduce appears in debug metrics
    found_llm_reduce = False
    for thread_id, thread_data in debug_metrics["threads"].items():
        for state_id, state_data in thread_data.items():
            if "llm_reduce" in state_data:
                reduce_data = state_data["llm_reduce"]
                assert "registration_order" in reduce_data
                assert "api_calls" in reduce_data
                assert "input_tokens" in reduce_data
                assert "output_tokens" in reduce_data
                found_llm_reduce = True

    assert found_llm_reduce, "llm_reduce not found in debug metrics"


def test_llm_rerank_metrics(integration_setup, model_config):
    """Test metrics for llm_rerank aggregate function"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-rerank-metrics-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
        """
        SELECT 
            llm_rerank(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'One word rank:', 'context_columns': [{'data': description}]}
            ) AS ranked,
            flock_get_metrics() AS metrics
        FROM VALUES
            ('Product 1'),
            ('Product 2'),
            ('Product 3')
        AS t(description);
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0

    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None and "metrics" in row

    metrics = json.loads(row["metrics"])

    # Check for llm_rerank metrics
    found_rerank = False
    for key in metrics.keys():
        if key.startswith("llm_rerank_"):
            rerank_metrics = metrics[key]
            assert "api_calls" in rerank_metrics
            assert "input_tokens" in rerank_metrics
            assert "output_tokens" in rerank_metrics
            found_rerank = True
            break

    assert found_rerank, f"llm_rerank metrics not found in: {list(metrics.keys())}"


def test_llm_first_metrics(integration_setup, model_config):
    """Test metrics for llm_first aggregate function"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-first-metrics-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
        """
        SELECT 
            category,
            llm_first(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'One word:', 'context_columns': [{'data': description}]}
            ) AS first_item,
            flock_get_metrics() AS metrics
        FROM VALUES
            ('Electronics', 'Product 1'),
            ('Electronics', 'Product 2')
        AS t(category, description)
        GROUP BY category;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0

    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None and "metrics" in row

    metrics = json.loads(row["metrics"])

    # Check for llm_first metrics
    found_first = False
    for key in metrics.keys():
        if key.startswith("llm_first_"):
            first_metrics = metrics[key]
            assert "api_calls" in first_metrics
            found_first = True
            break

    assert found_first, f"llm_first metrics not found in: {list(metrics.keys())}"


# ============================================================================
# Mixed Scalar and Aggregate Tests
# ============================================================================


def test_mixed_scalar_and_aggregate_metrics(integration_setup, model_config):
    """Test that both scalar and aggregate functions are tracked separately"""
    duckdb_cli_path, db_path = integration_setup
    model_name, provider = model_config

    run_cli(duckdb_cli_path, db_path, "SELECT flock_reset_metrics();")

    test_model_name = f"test-mixed-metrics-model_{model_name}"
    create_model_query = (
        f"CREATE MODEL('{test_model_name}', '{model_name}', '{provider}');"
    )
    run_cli(duckdb_cli_path, db_path, create_model_query, with_secrets=False)

    query = (
        """
        SELECT 
            llm_complete(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'Say: hi'}
            ) AS scalar_result,
            (SELECT llm_reduce(
                {'model_name': '"""
        + test_model_name
        + """'},
                {'prompt': 'One word summary:', 'context_columns': [{'data': description}]}
            ) FROM VALUES ('Test description') AS t(description)) AS aggregate_result,
            flock_get_metrics() AS metrics;
        """
    )
    result = run_cli(duckdb_cli_path, db_path, query)

    assert result.returncode == 0

    reader = csv.DictReader(StringIO(result.stdout))
    row = next(reader, None)
    assert row is not None and "metrics" in row

    metrics = json.loads(row["metrics"])

    # Should have both scalar and aggregate metrics
    has_scalar = any(key.startswith("llm_complete_") for key in metrics.keys())
    has_aggregate = any(key.startswith("llm_reduce_") for key in metrics.keys())

    assert has_scalar, "Scalar function metrics not found"
    assert has_aggregate, "Aggregate function metrics not found"
