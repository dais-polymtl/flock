---
title: Secret Management
sidebar_position: 3
---

# Secret Management

FlockMTL uses [DuckDB's Secrets Manager](https://duckdb.org/docs/configuration/secrets_manager.html) to securely store and manage sensitive information like API keys and credentials. Secrets are typed by service provider and can be temporary (in-memory) or persistent (on-disk).

## 1. Types of Secrets

Supported secret types for FlockMTL:

- **OpenAI**
- **Ollama**
- **Azure**

## 2. Creating a Secret

Secrets can be created with the `CREATE SECRET` SQL command. Temporary secrets are stored in memory, while persistent secrets are stored on disk. If no secret name is provided, DuckDB automatically assigns a default provider name.

### 2.1 OpenAI API Key

```sql
CREATE SECRET (
    TYPE OPENAI
    API_KEY 'your-api-key'
);
```

This creates a secret named `__default_openai`.

### 2.2 Ollama API URL

```sql
CREATE SECRET (
    TYPE OLLAMA
    API_URL '127.0.0.1:11434'
);
```

This creates a secret named `__default_ollama`.

### 2.3 Azure API Configuration

```sql
CREATE SECRET (
    TYPE AZURE
    API_KEY 'your-key-here'
    RESOURCE_NAME 'resource-name'
    API_VERSION 'api-version'
);
```

This creates a secret named `__default_azure`.

## 3. Persistent Secrets

To persist secrets across DuckDB sessions, use `CREATE PERSISTENT SECRET`:

### 3.1 Example for OpenAI (Persistent):

```sql
CREATE PERSISTENT SECRET (
    TYPE OPENAI
    API_KEY 'your-api-key'
);
```

### 3.2 Example for Ollama (Persistent):

```sql
CREATE PERSISTENT SECRET (
    TYPE OLLAMA
    API_URL '127.0.0.1:11434'
);
```

### 3.3 Example for Azure (Persistent):

```sql
CREATE PERSISTENT SECRET (
    TYPE AZURE
    API_KEY 'your-key-here'
    RESOURCE_NAME 'resource-name'
    API_VERSION 'api-version'
);
```

## 4. Deleting Secrets

To delete a secret, use the `DROP SECRET` command. For default provider secrets, the name will follow the pattern `__default_<provider_name>`.

### 4.1 Deleting Temporary Secrets

To delete a temporary secret, use:

```sql
DROP TEMPORARY SECRET your_secret_name;
```

For default provider secrets, the name will be in the format `__default_<provider_name>`, e.g., `__default_openai`, `__default_ollama`, or `__default_azure`.

Example for deleting a default temporary OpenAI secret:

```sql
DROP TEMPORARY SECRET __default_openai;
```

### 4.2 Deleting Persistent Secrets

To delete a persistent secret, use:

```sql
DROP PERSISTENT SECRET your_secret_name;
```

For default provider secrets, the name will be in the format `__default_<provider_name>`, e.g., `__default_openai`, `__default_ollama`, or `__default_azure`.

Example for deleting a default persistent OpenAI secret:

```sql
DROP PERSISTENT SECRET __default_openai;
```

## 5. Listing Secrets

To list all secrets:

```sql
FROM duckdb_secrets();
```
