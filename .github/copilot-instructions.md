# Copilot Instructions for Flock

## Overview

**Flock** is a C++ DuckDB extension that integrates LLMs (Large Language Models) and RAG (Retrieval-Augmented Generation) into DuckDB through declarative SQL queries. It supports OpenAI, Azure, and Ollama providers and enables semantic functions such as `llm_complete`, `llm_filter`, `llm_embedding`, and hybrid search directly from SQL.

- **Language**: C++17
- **Build system**: CMake (3.5+) with DuckDB's extension CI tools (`extension-ci-tools/`)
- **Dependency manager**: vcpkg (managed via `vcpkg.json`)
- **Key dependencies**: `nlohmann-json`, `curl`, `gtest` (see `vcpkg.json`)
- **DuckDB version targeted**: v1.4.4 (see `MainDistributionPipeline.yml`)

## Repository Layout

```
.
├── CMakeLists.txt              # Top-level CMake (builds static + loadable extension, unit tests)
├── extension_config.cmake      # DuckDB extension load config (references this repo)
├── vcpkg.json                  # vcpkg dependency manifest
├── Makefile                    # Convenience targets (lf_setup, lf_setup_dev)
├── scripts/
│   ├── build_and_run.sh        # Interactive guided build + run script
│   ├── build_project.sh        # Non-interactive build script
│   └── setup_vcpkg.sh          # vcpkg bootstrap
├── src/
│   ├── flock_extension.cpp     # Extension entry point (LoadInternal, FlockExtension::Load)
│   ├── include/flock/          # Public headers
│   ├── core/                   # Config, common utilities
│   ├── functions/              # Scalar and aggregate SQL functions
│   ├── model_manager/          # Provider integrations (OpenAI, Azure, Ollama)
│   ├── prompt_manager/         # Prompt management
│   ├── secret_manager/         # API key/secret handling
│   ├── registry/               # Model/prompt registries
│   ├── metrics/                # Metrics collection
│   └── custom_parser/          # Custom SQL parser extension
├── test/
│   ├── unit/                   # C++ unit tests (GTest), built via CMake
│   └── integration/            # Python integration tests (pytest, uv)
├── duckdb/                     # DuckDB source submodule
├── extension-ci-tools/         # DuckDB extension CI/build helpers submodule
├── .clang-format               # clang-format config (LLVM style, IndentWidth=4, ColumnLimit=0)
├── .cmake-format               # cmake-format config
└── .pre-commit-config.yaml     # Pre-commit hooks: clang-format v18.1.8, cmake-format v0.6.13
```

## Building

Always ensure submodules are initialised before building:

```bash
git submodule update --init --recursive
```

### Setup vcpkg (first time or after clean)

```bash
bash scripts/setup_vcpkg.sh
export VCPKG_TOOLCHAIN_PATH="$(pwd)/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

### Release build

```bash
mkdir -p build/release
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DEXTENSION_STATIC_BUILD=1 \
      -DVCPKG_BUILD=1 \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_TOOLCHAIN_PATH" \
      -DVCPKG_MANIFEST_DIR="$(pwd)" \
      -DDUCKDB_EXTENSION_CONFIGS="$(pwd)/extension_config.cmake" \
      -S duckdb -B build/release
cmake --build build/release --config Release
```

Use `-G "Unix Makefiles"` if Ninja is not available. The DuckDB binary will be at `build/release/duckdb`.

### Debug build

Replace `Release` with `Debug` and `build/release` with `build/debug` in the commands above. Debug builds enable AddressSanitizer automatically.

## Running Unit Tests

Unit tests use GTest and are built as part of the CMake build. After building:

```bash
cd build/release   # or build/debug
ctest --output-on-failure
```

Or run the test binary directly: `./flock_tests`

## Running Integration Tests

Integration tests use Python/pytest and are in `test/integration/`. They require a running DuckDB binary with the Flock extension loaded and provider credentials set in a `.env` file (see `test/integration/.env-example`).

```bash
cd test/integration
uv sync          # install Python deps (requires uv)
uv run pytest
```

## Code Style & Linting

- **C++**: `clang-format` v18.1.8 (config in `.clang-format`, LLVM-based, indent=4, no column limit)
- **CMake**: `cmake-format` v0.6.13 (config in `.cmake-format`)
- Run pre-commit on staged files: `pre-commit run` or `pre-commit run --all-files`
- Install dev tools: `make lf_setup_dev`

Always run `clang-format` on modified C++ files before committing. The CI pipeline enforces both `format` and `tidy` checks (`code-quality-check` job in `MainDistributionPipeline.yml`).

## CI Pipeline

Defined in `.github/workflows/MainDistributionPipeline.yml`:

- **duckdb-stable-build**: Builds extension binaries for all platforms using DuckDB v1.4.4 CI tools.
- **code-quality-check**: Runs `clang-format` and `clang-tidy` checks.

Triggered on push to `main`/`dev` when `src/`, `test/`, `CMakeLists.txt`, or workflow files change, and on `workflow_dispatch`.

## Key Notes

- The extension entry point is `src/flock_extension.cpp` → `FlockExtension::Load` → `LoadInternal`.
- All SQL functions are registered via `flock::Config::Configure(loader)` in `src/core/config/`.
- New scalar functions go in `src/functions/scalar/`; aggregate functions in `src/functions/aggregate/`.
- Provider implementations live in `src/model_manager/providers/`.
- Public headers for the extension are under `src/include/flock/`.
- The `duckdb/` and `extension-ci-tools/` directories are git submodules — do not modify them.
