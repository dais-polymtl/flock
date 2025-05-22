<a id="readme-top"></a>

<br />

<div align="center">
  <a href="https://dais-polymtl.github.io/flockmtl/">
    <img src="docs/static/img/logo-dark.svg" alt="Logo" height="80">
  </a>
  <br /><br />
  <p align="center">
    A DBMS extension that deeply integrates LLM and RAG capabilities directly into OLAP systems to seamlessly mix analytics with semantic analysis.
    <br />
    <a href="https://dais-polymtl.github.io/flockmtl/docs/what-is-flockmtl"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://dais-polymtl.github.io/flockmtl/">Landing Page</a>
    |
    <a href="https://github.com/dais-polymtl/flockmtl/issues/new?labels=bug&template=bug-report.md">Report Bug</a>
    |
    <a href="https://github.com/dais-polymtl/flockmtl/issues/new?labels=enhancement&template=feature-request.md">Request Feature</a>
  </p>
</div>

<details>
  <summary>
    <h2>Table of Contents</h2>
  </summary>
  <ol>
    <li><a href="#-about-the-project">About The Project</a></li>
    <li><a href="#-features">Features</a></li>
    <li>
      <a href="#-getting-started">Getting Started</a>
      <ul>
        <li><a href="#-prerequisites">Prerequisites</a></li>
        <li><a href="#⚙-installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#-usage">Usage</a></li>
    <li><a href="#-roadmap">Roadmap</a></li>
    <li><a href="#-feedback-and-issues">Feedback and Issues</a></li>
    <li><a href="#-license">License</a></li>
    <li><a href="#-acknowledgments">Acknowledgments</a></li>
  </ol>
</details>

## 📜 About The Project

FlockMTL is an advanced **DuckDB** extension that seamlessly integrates analytics with semantic analysis through declarative SQL queries. Designed for modern data analysis needs, FlockMTL empowers users to work with structured and unstructured data, combining OLAP workflows with the capabilities of **LLMs** (Large Language Models) and **RAG** (Retrieval-Augmented Generation) pipelines.

<p align="right"><a href="#readme-top">🔝 back to top</a></p>

## 🔥 Features

- **Declarative SQL Interface**: Perform text generation, classification, summarization, filtering, and embedding generation using SQL queries.
- **Multi-Provider Support**: Easily integrate with OpenAI, Azure, and Ollama for your AI needs.
- **End-to-End RAG Pipelines**: Enable retrieval and augmentation workflows for enhanced analytics.
- **Map and Reduce Functions**: Intuitive APIs for combining semantic tasks and data analytics directly in DuckDB.

<p align="right"><a href="#readme-top">🔝 back to top</a></p>

## 🚀 Getting Started

### 📝 Prerequisites

1. **DuckDB**: Version 1.1.1 or later. Install it from the official [DuckDB installation guide](https://duckdb.org/docs/installation/).
2. **Supported Providers**: Ensure you have credentials or API keys for at least one of the supported providers:
   - OpenAI
   - Azure
   - Ollama
3. **Supported OS**:
   - Linux
   - macOS
   - Windows

<p align="right"><a href="#readme-top">🔝 back to top</a></p>

### ⚙️ Installation

FlockMTL is a **Community Extension** available directly from DuckDB's community catalog.

1. Install the extension:
   ```sql
   INSTALL flockmtl FROM community;
   ```
2. Load the extension:
   ```sql
   LOAD flockmtl;
   ```

<p align="right"><a href="#readme-top">🔝 back to top</a></p>

## 💻 Usage

### 🔧 Example Query

Using FlockMTL, you can run semantic analysis tasks directly in DuckDB. For example:

```sql
SELECT llm_complete(
    {'model_name': 'description-model'},
    {'prompt': 'Summarize this data point: '},
    {'data_point': data_column}
) AS summary
FROM your_table;
```

Explore more usage examples in the [documentation](https://dais-polymtl.github.io/flockmtl/docs/supported-providers/openai).

<p align="right"><a href="#readme-top">🔝 back to top</a></p>

## 🛣️ Roadmap

Our roadmap outlines upcoming features and improvements. Stay updated by checking out our [detailed plan](https://github.com/dais-polymtl/flockmtl/issues/39).

<p align="right"><a href="#readme-top">🔝 back to top</a></p>

## 🛠️ Feedback and Issues

We value your feedback! If you’d like to report an issue or suggest a new feature, please use the links below:

- <a href="https://github.com/dais-polymtl/flockmtl/issues/new?labels=bug&template=bug-report.md">Report a Bug</a>
- <a href="https://github.com/dais-polymtl/flockmtl/issues/new?labels=enhancement&template=feature-request.md">Request a Feature</a>

For contributing code or other contributions, please refer to our dedicated [Contribution Guidelines](#).

<p align="right"><a href="#readme-top">🔝 back to top</a></p>

## 📝 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

<p align="right"><a href="#readme-top">🔝 back to top</a></p>

## ✨ Team

This project is under active development by the [**Data & AI Systems Laboratory (DAIS Lab)**](https://github.com/dais-polymtl) at [**Polytechnique Montréal**](https://www.polymtl.ca/).

<p align="right"><a href="#readme-top">🔝 back to top</a></p>
