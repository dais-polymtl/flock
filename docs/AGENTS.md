# Documentation project instructions

## About this project

- This is the Flock documentation site built on [Mintlify](https://mintlify.com)
- Production is hosted on GitHub Pages at https://dais-polymtl.github.io/flock/
- Flock is a DuckDB extension that integrates language models and RAG into SQL analytics
- Pages are MDX files with YAML frontmatter
- Configuration lives in `docs.json`
- Companion site for the VLDB 2025 paper *Beyond Quacking*

## Terminology

- Prefer **Flock** (capital F) for the product name
- Prefer **DuckDB** spelling
- Prefer **prompt** / **model** as first-class SQL resources (`CREATE PROMPT`, `CREATE MODEL`)
- Prefer **map** / **reduce** when describing scalar vs aggregate LLM functions

## Style preferences

- Use active voice and second person ("you")
- Keep sentences concise — one idea per sentence
- Use sentence case for headings
- Code formatting for file names, commands, paths, SQL identifiers, and function names
- Assume readers have loaded Flock per [Installation](/installation); do not repeat the community install SQL in other pages

## Content boundaries

- Document public SQL APIs, providers, and developer setup
- Do not document unpublished internal-only tooling or speculative features as GA

## Navigation groups

Sidebar sections in `docs.json`:

- **Resource Management** — models, prompts, secrets, model parameters
- **Scalar / Aggregate Functions** — map and reduce SQL APIs (hybrid search lives under Scalar)
- **Multimodal** — image and audio inputs
- **Patterns** — cross-cutting SQL patterns (structured output)
- **Operations** — performance tuning and metrics
- **Platform** — WASM / browser deployment
- **Help** — FAQ
- **Contributing** — developer guide
