---
title: Developer Guide
sidebar_position: 11
---

# Developer Guide

This guide is for developers who want to build, extend, or contribute to Flock itself.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Local Development Setup

- **Clone the repository**:

```bash
git clone --recursive https://github.com/dais-polymtl/flock.git
cd flock
```

- **Initialize submodules** (if you forgot `--recursive`):

```bash
git submodule update --init --recursive
```

- **Build and run via helper script**:

```bash
./scripts/build_and_run.sh
```

The interactive script will:

- Check for required tools (CMake, compiler, Ninja/Make, etc.).
- Configure dependencies via `vcpkg`.
- Build Flock (Debug/Release).
- Launch DuckDB with the Flock extension preloaded.

See the root `README.md` for a concise overview of these steps.

## Project Structure (High Level)

- **DuckDB engine**: Vendored under `duckdb/`, used as the host engine.
- **Extension sources**:
  - `src/functions/` – scalar and aggregate LLM functions and helpers.
  - `src/model_manager/` – model registry, providers, and adapters.
  - `src/prompt_manager/` – prompt management and storage.
  - `src/metrics/` – LLM metrics collection and SQL API.
- **Docs site**: Docusaurus site under `docs/` (this documentation).

## Building the Extension Manually

While `./scripts/build_and_run.sh` is the recommended path, you can also build manually:

```bash
mkdir -p build
cd build
cmake .. -G Ninja
ninja
```

The resulting Flock extension library can then be loaded from DuckDB using `LOAD` with the appropriate path.

## Running Tests

Flock comes with both **unit** and **integration** tests:

- C++ unit tests live under `test/unit/`.
- Integration tests (Python + DuckDB) live under `test/integration/`.

Example pattern (from the repo root):

```bash
python -m pytest test/integration
```

Check the repository’s CI configuration for the exact commands used in automation.

## Coding Conventions

When contributing code:

- Follow the surrounding C++ style (namespaces, includes, brace style).
- Avoid introducing new dependencies without a clear reason.
- Prefer small, focused pull requests with clear descriptions.

If in doubt, mirror patterns used in existing functions such as `llm_complete` or the metrics manager.

## Working on Providers & Models

- Provider-specific adapters live under `src/model_manager/providers/adapters/`.
- HTTP and batching logic is centralized in provider handlers under
  `src/include/flock/model_manager/providers/handlers/`.
- New providers should:
  - Integrate with the existing metrics API.
  - Respect the `context_columns` abstraction.
  - Provide clear, actionable error messages when a feature is unsupported.

For examples, see the existing OpenAI, Azure, Ollama, and Anthropic adapters.

## Docs & Developer Experience

The Docusaurus docs live in `docs/`. To work on them:

```bash
cd docs
npm install
npm start
```

This runs the docs site locally with hot reload. When adding new features to Flock, prefer updating or extending:

- `what-is-flock.md` for high-level positioning.
- The relevant function or feature page (e.g., `image-support.md`, `audio-support.md`, `llm-metrics.md`).
- This **Developer Guide** for build, testing, or contribution-related changes.

