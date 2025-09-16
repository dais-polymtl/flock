# Getting Started With Flock

Flock as a DuckDB extension is designed to simplify the integration of Large Language Models (LLMs) into your data
workflows. This guide will help you get started with Flock, covering installation, setup, and basic usage.

import TOCInline from '@theme/TOCInline';

<TOCInline toc={toc} />

## Install DuckDB

To install Duckdb, it's an easy process you need just to
visit [DuckDB Installation Page](https://duckdb.org/docs/installation/) and choose the installation options that
represent your environment, by specifying:

- **Version**: Stable or Preview.
- **Environment**: CLI, Python, R, etc.
- **Platform**: Linux, MacOS or Windows.
- **Download Method**: Direct or Package Manager.

After installing DuckDB, you can verify the installation and get started by following
the [DuckDB CLI Overview](https://duckdb.org/docs/stable/clients/cli/overview.html/).

## Install Flock Extension

At this stage you should have a running DuckDB instance. To install Flock, run the following SQL commands in your DuckDB
instance:

```sql
INSTALL
flock FROM community;
LOAD
flock;
```

This will install the Flock extension and load it into your DuckDB environment.

## Set Up API Keys for Providers

To use Flock functions, you need to set up API keys for the providers you plan to use. Flock supports multiple providers
such as **OpenAI**, **Azure**, and **Ollama**.

Refer to the following sections for detailed instructions on setting up API keys for each provider.

import DocCard from '@site/src/components/global/DocCard';
import { RiOpenaiFill } from "react-icons/ri";
import { VscAzure } from "react-icons/vsc";
import { SiOllama } from "react-icons/si";

<DocCard
Icon={VscAzure}
title="Azure"
link="/flock/docs/getting-started/azure"
/>
<DocCard
Icon={SiOllama}
title="Ollama"
link="/flock/docs/getting-started/ollama"
/>
<DocCard
Icon={RiOpenaiFill}
title="OpenAI"
link="/flock/docs/getting-started/openai"
/>
