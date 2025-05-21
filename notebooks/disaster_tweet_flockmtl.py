# %% [markdown]
# # NLP with Disaster Tweets: Advanced Cleaning with FlockMTL
# 
# This notebook demonstrates advanced text processing of disaster-related tweets using the **FlockMTL** extension for DuckDB. The pipeline includes cleaning, feature extraction, and data visualization.

# %% [markdown]
# ## Step 1: Install and Import Libraries
# 
# Install required Python libraries and import the necessary modules.

# %%
%pip install duckdb==1.1.1 matplotlib pandas kaggle

import duckdb
import pandas as pd
import matplotlib.pyplot as plt
import os
from kaggle.api.kaggle_api_extended import KaggleApi
from collections import Counter

# %% [markdown]
# ## Step 2: Download Dataset from Kaggle
# 
# Download and extract the disaster tweets dataset from Kaggle. Ensure you have set up your Kaggle API token. [Learn how to set up the Kaggle API token](https://github.com/dais-polymtl/flockmtl/blob/main/notebooks/README.md).

# %%
api = KaggleApi()
api.authenticate()

os.makedirs('data', exist_ok=True)
api.dataset_download_files(
    'vbmokin/nlp-with-disaster-tweets-cleaning-data', 
    path='data', 
    unzip=True
)

# %% [markdown]
# ## Step 3: Set Up DuckDB and Load FlockMTL
# 
# Initialize an in-memory DuckDB connection and load the FlockMTL extension.

# %%
# Initialize DuckDB
con = duckdb.connect(':memory:')

# Load FlockMTL extension
con.sql("INSTALL flockmtl FROM community")
con.sql("LOAD flockmtl")

# %% [markdown]
# ## Step 4: Load and Preview Dataset
# 
# Load the dataset into DuckDB and display the first few rows to understand the structure.

# %%
csv_path = 'data/test_data_cleaning.csv'
# We limit the number of rows to 15 for demonstration purposes
con.execute(f"CREATE TABLE disaster_tweets AS SELECT * FROM read_csv_auto('{csv_path}') LIMIT 15")
con.execute("SELECT * FROM disaster_tweets LIMIT 5").df()

# %% [markdown]
# ## Step 5: Define API Key for OpenAI Integration
# 
# Set up the OpenAI API key to enable advanced NLP processing using FlockMTL.

# %%
con.sql("CREATE SECRET (TYPE OPENAI, API_KEY 'your-openai-api-key');")

# %% [markdown]
# ## Step 6: Extract Location from Tweets
# 
# Define a prompt to extract the country from the tweet text or location field.

# %%
# Define location extraction prompt
con.execute("""
CREATE PROMPT ('location-extraction', '
Extract the country from the tweet text or location. 
Output JSON with:
{
    "country": "Country name"
}
If nothing is found, return "Unknown".
');
""")

# Process location extraction
query = """
CREATE TABLE location_extraction AS
SELECT 
    id,
    text AS tweet_text,
    location AS existing_location,
    llm_complete_json(
        {'model_name': 'gpt-4o'},
        {'prompt_name': 'location-extraction'},
        {'tweet_text': text, 'existing_location': location}
    ).country AS extracted_country
FROM disaster_tweets;
"""
con.execute(query)
con.execute("SELECT * FROM location_extraction LIMIT 5").df()

# %% [markdown]
# ## Step 7: Extract Text Features
# 
# Define a prompt to extract key features such as URL presence, hashtags, and numeric content.

# %%
# Define text feature extraction prompt
con.execute("""
CREATE PROMPT ('text-features', '
Analyze the tweet text and extract detailed features:
{
    "has_url": true/false,
    "has_hashtag": true/false,
    "contains_numbers": true/false
}
');
""")

# Process text feature extraction
query = """
CREATE TABLE text_features AS
SELECT 
    id,
    text_features_json.has_url,
    text_features_json.has_hashtag,
    text_features_json.contains_numbers
FROM (
    SELECT 
        id,
        text AS tweet_text,
        llm_complete_json(
            {'model_name': 'gpt-4o'},
            {'prompt_name': 'text-features'},
            {'tweet_text': text}
        ) AS text_features_json
    FROM disaster_tweets
);
"""
con.execute(query)
con.execute("SELECT * FROM text_features LIMIT 5").df()

# %% [markdown]
# ## Step 8: Extract Keywords and Sentiment
# 
# Define a prompt to extract significant keywords and sentiment from the tweets.

# %%
# Define keyword extraction prompt
con.execute("""
CREATE PROMPT ('keyword-extraction', '
Extract significant keywords from the tweet and classify sentiment:
{
    "primary_keywords": ["keyword1", "keyword2"],
    "sentiment": "positive/negative/neutral"
}
');
""")

# Process keyword extraction
query = """
CREATE TABLE tweet_keywords AS
SELECT 
    id,
    keywords_json.primary_keywords,
    keywords_json.sentiment
FROM (
    SELECT 
        id,
        text AS tweet_text,
        llm_complete_json(
            {'model_name': 'gpt-4o'},
            {'prompt_name': 'keyword-extraction'},
            {'tweet_text': text}
        ) AS keywords_json
    FROM disaster_tweets
);
"""
con.execute(query)
con.execute("SELECT * FROM tweet_keywords LIMIT 5").df()

# %% [markdown]
# ## Step 9: Combine and Clean Data
# 
# Integrate extracted features into a consolidated dataset.

# %%
query = """
CREATE TABLE enriched_tweets AS
SELECT 
    dt.id,
    dt.text,
    le.extracted_country AS country,
    tf.has_url,
    tf.has_hashtag,
    tf.contains_numbers,
    kw.primary_keywords,
    kw.sentiment
FROM 
    disaster_tweets dt
LEFT JOIN location_extraction le ON dt.id = le.id
LEFT JOIN text_features tf ON dt.id = tf.id
LEFT JOIN tweet_keywords kw ON dt.id = kw.id;
"""
con.execute(query)
final_df = con.execute("SELECT * FROM enriched_tweets").df()

# Process primary keywords
final_df['primary_keywords'] = final_df['primary_keywords'].apply(lambda x: ', '.join(eval(x)) if x else '')
final_df.dropna(inplace=True)
final_df.to_csv('cleaned_disaster_tweets.csv', index=False)

# %% [markdown]
# ## Step 10: Visualize Data Insights
# 
# Generate various visualizations to explore tweet characteristics.

# %% [markdown]
# ### Top 10 Countries in Dataset

# %%
plt.figure(figsize=(12, 6))
country_counts = final_df['country'].value_counts().head(10)
country_counts.plot(kind='bar', color='skyblue')
plt.title('Top 10 Countries in Dataset')
plt.xlabel('Country')
plt.ylabel('Number of Tweets')
plt.xticks(rotation=45, ha='right')
plt.tight_layout()
plt.show()

# %% [markdown]
# ### Sentiment Distribution in Tweets

# %%
sentiment_counts = final_df['sentiment'].value_counts()
plt.figure(figsize=(8, 6))
sentiment_counts.plot(kind='bar', color=['#8c564b', '#e377c2', '#7f7f7f'])
plt.title('Sentiment Distribution')
plt.xlabel('Sentiment')
plt.ylabel('Number of Tweets')
plt.tight_layout()
plt.show()

# %% [markdown]
# ### Hashtag Presence

# %%
hashtag_counts = final_df['has_hashtag'].value_counts(normalize=True) * 100
labels = ['Has Hashtag' if x else 'No Hashtag' for x in url_counts.index.tolist()]

plt.figure(figsize=(6, 6))
plt.pie(hashtag_counts, labels=['Has Hashtag', 'No Hashtag'], autopct='%1.1f%%', startangle=90)
plt.title('Hashtag Presence in Tweets')
plt.axis('equal')
plt.show()

# %% [markdown]
# ### URL Presence

# %%
url_counts = final_df['has_url'].value_counts(normalize=True) * 100
labels = ['Has URL' if x else 'No URL' for x in url_counts.index.tolist()]

plt.figure(figsize=(6, 6))
plt.pie(url_counts, labels=labels, autopct='%1.1f%%', startangle=90)
plt.title('URL Presence in Tweets')
plt.axis('equal')
plt.show()
# %% [markdown]
# ### Top 10 Keywords in Tweets

# %%
keyword_counter = Counter()
final_df['primary_keywords'].str.split(', ').dropna().apply(keyword_counter.update)
top_keywords = pd.DataFrame(keyword_counter.most_common(10), columns=['Keyword', 'Frequency'])

plt.figure(figsize=(12, 6))
plt.bar(top_keywords['Keyword'], top_keywords['Frequency'], color='lightblue')
plt.title('Top 10 Keywords in Tweets')
plt.xlabel('Keyword')
plt.ylabel('Frequency')
plt.xticks(rotation=45, ha='right')
plt.tight_layout()
plt.show()

# %% [markdown]
# ### Numeric Content Presence

# %%
number_counts = final_df['contains_numbers'].value_counts(normalize=True) * 100
plt.figure(figsize=(6, 6))
plt.pie(number_counts, labels=['Contains Numbers', 'No Numbers'], autopct='%1.1f%%', startangle=90)
plt.title('Numeric Content Presence in Tweets')
plt.axis('equal')
plt.show()
