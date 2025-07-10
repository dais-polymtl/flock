# %% [markdown]
# # Comprehensive Product Review Analysis with FlockMTL (Enhanced with Visualizations)

# %% [markdown]
# # Overview
# This notebook demonstrates how to use **FlockMTL** with DuckDB for advanced product review analysis,
# now enhanced with visualizations and improved LLM functions.
#
# Key goals:
# - Perform sentiment analysis of reviews using LLM prompts
# - Filter high-impact reviews for detailed insights
# - Extract key themes from customer feedback
# - Use advanced LLM aggregation functions to summarize and rank reviews
# - Visualize insights for better understanding

# %% [markdown]
# ## Step 1: Install Required Libraries
# DuckDB is a lightweight database engine. We use a specific version for compatibility with FlockMTL.

# %%
%pip install duckdb==1.1.1 matplotlib pandas

import duckdb
import pandas as pd
import matplotlib.pyplot as plt

print(f"DuckDB version: {duckdb.__version__}")

# %% [markdown]
# ## Step 2: Download Kaggle Dataset
# We will download the dataset from Kaggle and load it into our DuckDB database.
# Ensure you have set up your Kaggle API token [check here](https://github.com/dais-polymtl/flockmtl/blob/main/notebooks/README.md) for more details.

# %%
%pip install kaggle

import os
from kaggle.api.kaggle_api_extended import KaggleApi

api = KaggleApi()
api.authenticate()

os.makedirs('data', exist_ok=True)
api.dataset_download_files(
    'arhamrumi/amazon-product-reviews', 
    path='data', 
    unzip=True
)

# %% [markdown]
# ## Step 3: Create a DuckDB Database
# We create an in-memory DuckDB database for this analysis.

# %%
con = duckdb.connect(':memory:')

# %% [markdown]
# ## Step 4: Load and Install FlockMTL Extension
# The FlockMTL extension enables advanced natural language processing with LLMs.

# %%
con.sql("INSTALL flockmtl FROM community")
con.sql("LOAD flockmtl")

# %% [markdown]
# ## Step 5: Configure OpenAI Secret
# Securely add the OpenAI API key to DuckDB's secret manager.

# %%
con.sql("CREATE SECRET (TYPE OPENAI, API_KEY 'your-openai-api-key');")
print("OpenAI secret created.")

# %% [markdown]
# ## Step 6: Add gpt-4o Model as product_review_model
# Add the gpt-4o model for sentiment analysis of product reviews to the GLOBAL scope.
# The GLOBAL scope enables model sharing across databases, allowing the model name to be used in any database.

# %%
con.execute("""
CREATE GLOBAL MODEL(
    'product_review_model',
    'gpt-4o',
    'openai',
    {"tuple_format": "json", "batch_size": 32, "model_parameters": {"temperature": 0.7}}
);
""")

# %% [markdown]
# ## Step 7: Move the model to the LOCAL scope
# Move the model to the LOCAL scope for the current database.
# FlockMTL also supports models with the LOCAL scope, making them accessible only within the current database.

# %%
con.execute("UPDATE MODEL 'product_review_model' TO LOCAL;")

# %% [markdown]
# ## Step 8: Load Product Review Data
# Load product review data from a CSV file into DuckDB.

# %%
csv_path = 'data/Reviews.csv'
# We limit the number of rows to 15 for demonstration purposes
con.execute(f"CREATE TABLE product_reviews AS SELECT * FROM read_csv_auto('{csv_path}') LIMIT 15")
product_reviews_df = con.execute("SELECT * FROM product_reviews").df()

print("Product reviews data loaded successfully.")
product_reviews_df.head()

# %% [markdown]
# ## Step 9: Visualize Ratings Distribution
# Analyze and visualize the distribution of product ratings.

# %%
plt.figure(figsize=(10, 6))
ratings_count = product_reviews_df['Score'].value_counts().sort_index()
plt.bar(ratings_count.index, ratings_count.values, color='skyblue')
plt.title("Distribution of Product Ratings")
plt.xlabel("Star Rating")
plt.ylabel("Number of Reviews")
plt.show()

# %% [markdown]
# ## Step 10: Sentiment Analysis with Custom Prompt
# Analyze review sentiment using a custom prompt.

# %%
sentiment_analysis_prompt = """
Analyze the sentiment of the product review. Provide a JSON response:
{{
    "sentiment": "positive/negative/neutral",
    "explanation": "Brief explanation"
}}

Review: {review}
Star Rating: {rating}
"""
con.execute(f"CREATE PROMPT ('sentiment-analysis', '{sentiment_analysis_prompt}');")

query = """
CREATE TABLE sentiment_analysis AS
SELECT 
    ProductID AS product_id,
    ID AS review_id,
    Text AS review_text,
    Score AS star_rating,
    llm_complete_json(
        {'model_name': 'product_review_model'},
        {'prompt_name': 'sentiment-analysis'},
        {'review': review_text, 'rating': star_rating}
    ) AS sentiment_json
FROM product_reviews;
"""
con.execute(query)

sentiment_df = con.execute("SELECT * FROM sentiment_analysis").df()
sentiment_df.head()

# %% [markdown]
# ## Step 11: Visualize Sentiment Analysis Results
# Plot the distribution of sentiment labels.

# %%
# Extract sentiment from JSON
sentiment_df['sentiment'] = sentiment_df['sentiment_json'].apply(lambda x: eval(x)['sentiment'])

plt.figure(figsize=(10, 6))
sentiments_count = sentiment_df['sentiment'].value_counts()
plt.pie(sentiments_count, labels=sentiments_count.index, autopct='%1.1f%%', startangle=140, colors=['lightgreen', 'lightcoral', 'gold'])
plt.title("Sentiment Analysis Distribution")
plt.show()

# %% [markdown]
# ## Step 12: LLM Aggregation Functions Demonstration

# %% [markdown]
# ### 12.1 llm_reduce: Summarize Product Reviews by Category

# %%
# Create a prompt for llm_reduce
con.execute("""
CREATE PROMPT ('review-summary', 'Summarize these product reviews, highlighting key insights and overall sentiment.');
""")

# Use llm_reduce to summarize reviews by category
query = """
SELECT ProductID, 
       llm_reduce(
           {'model_name': 'product_review_model'},
           {'prompt_name': 'review-summary'},
           {'review_text': Text, 'rating': Score}
       ) AS category_review_summary
FROM product_reviews
GROUP BY ProductID
"""
category_summaries = con.execute(query).df()
print("Category Review Summaries:")
category_summaries.head()

# %% [markdown]
# ### 12.2 llm_rerank: Rank Reviews by Relevance

# %%
# Create a prompt for llm_rerank
con.execute("""
CREATE PROMPT ('review-ranking', 'Rank these reviews based on their relevance and detail.');
""")

# Use llm_rerank to order reviews
query = """
SELECT 
    llm_rerank(
        {'model_name': 'product_review_model'},
        {'prompt_name': 'review-ranking'},
        {'review_text': Text, 'rating': Score}
    ) AS ranked_reviews
FROM product_reviews
"""
ranked_reviews = con.execute(query).df()
print("Ranked Reviews:")
pd.DataFrame(eval(ranked_reviews.iloc[0]['ranked_reviews']))

# %% [markdown]
# ### 12.3 llm_first: Extract First Key Insight

# %%
# Create a prompt for llm_first
con.execute("""
CREATE PROMPT ('first-review-insight', 'What is the most impactful review for a potential client.');
""")

# Use llm_first to get the first significant insight
query = """
SELECT ProductID, 
       llm_first(
           {'model_name': 'product_review_model'},
           {'prompt_name': 'first-review-insight'},
           {'review_text': Text, 'rating': Score}
       ) AS first_key_insight
FROM product_reviews
GROUP BY ProductID
"""
first_insights = con.execute(query).df()
print("First Key Insights per Product:")
first_insights.head()

# %% [markdown]
# ## Step 13: Filter High-Impact Reviews
# Identify reviews with significant insights.

# %%
is_high_impact_review_prompt = """
Determine if the review is high-impact:
- Sentiment: {sentiment}
- Rating: {rating}
- Length: {review_length}

Output: true/false.
"""
con.execute(f"CREATE PROMPT ('is-high-impact-review', '{is_high_impact_review_prompt}');")

query = """
CREATE TABLE filtered_reviews AS
SELECT 
    *
FROM sentiment_analysis
WHERE 
    llm_filter(
        {'model_name': 'product_review_model'},
        {'prompt_name': 'is-high-impact-review'},
        {'sentiment': sentiment_json, 'rating': star_rating, 'review_length': LENGTH(review_text)}
    );
"""
con.execute(query)

filtered_reviews_df = con.execute("SELECT * FROM filtered_reviews").df()
filtered_reviews_df.head()

# %% [markdown]
# ## Step 14: Visualize High-Impact Reviews
# Compare ratings of high-impact reviews with all reviews.

# %%
plt.figure(figsize=(12, 6))
high_impact_ratings = filtered_reviews_df['star_rating'].value_counts().sort_index()

plt.bar(ratings_count.index, ratings_count.values, alpha=0.5, label="All Reviews", color='skyblue')
plt.bar(high_impact_ratings.index, high_impact_ratings.values, alpha=0.8, label="High-Impact Reviews", color='orange')
plt.title("Comparison of Star Ratings: All vs. High-Impact Reviews")
plt.xlabel("Star Rating")
plt.ylabel("Number of Reviews")
plt.legend()
plt.show()

# %% [markdown]
# ## Step 15: Extract Key Themes from Reviews
# Identify recurring themes in customer feedback.

# %%
extract_themes_prompt = """
Extract key themes from the review data. Output:
{{
    "themes": ["theme1", "theme2", "theme3"]
}}
"""
con.execute(f"CREATE PROMPT ('extract-themes', '{extract_themes_prompt}');")

query = """
CREATE TABLE themes_extracted AS
SELECT 
    product_id,
    review_id,
    review_text,
    star_rating,
    llm_complete_json(
        {'model_name': 'product_review_model'},
        {'prompt_name': 'extract-themes'},
        {'sentiment_json': sentiment_json}
    ) AS themes
FROM filtered_reviews;
"""
con.execute(query)

themes_df = con.execute("SELECT * FROM themes_extracted").df()
themes_df['themes'] = themes_df['themes'].apply(lambda x: eval(x)['themes'])
all_themes = themes_df['themes'].explode()
theme_counts = all_themes.value_counts()

plt.figure(figsize=(12, 6))
theme_counts.plot(kind='bar', color='purple')
plt.title("Most Common Themes in Reviews")
plt.xlabel("Theme")
plt.ylabel("Count")
plt.tight_layout()
plt.show()

# %% [markdown]
# ## Step 16: Analyze the Lowest Reviews
# Focus on reviews with the lowest ratings.

# %%
lowest_reviews_df = product_reviews_df[product_reviews_df['Score'] <= 2]

# %% [markdown]
# ## Step 17: Sentiment and Explanation for Low-Rated Reviews
# Analyze sentiment and get explanations for low ratings.

# %%
low_rating_reviews_prompt = """
Analyze the sentiment of the low-rated product review. Provide a JSON response:
{{
    "sentiment": "positive/negative/neutral",
    "explanation": "Detailed reason for the low rating"
}}

Review: {review}
Star Rating: {rating}
"""

con.execute(f"CREATE PROMPT ('low-rating-sentiment-analysis', '{low_rating_reviews_prompt}');")

query_low_rating = """
CREATE TABLE low_rating_sentiment AS
SELECT 
    ProductID AS product_id,
    ID AS review_id,
    Text AS review_text,
    Score AS star_rating,
    llm_complete_json(
        {'model_name': 'product_review_model'},
        {'prompt_name': 'low-rating-sentiment-analysis'},
        {'review': review_text, 'rating': star_rating}
    ) AS sentiment_json
FROM product_reviews
WHERE Score <= 2;
"""
con.execute(query_low_rating)

low_rating_sentiment_df = con.execute("SELECT * FROM low_rating_sentiment").df()
low_rating_sentiment_df.head()

# %% [markdown]
# ## Step 18: Suggestions for Product Improvement
# Generate product improvement suggestions.

# %%
improvement_suggestion_prompt = """
Provide suggestions to improve the product based on the low-rated review. Output:
{{
    "suggestions": ["suggestion 1", "suggestion 2", "suggestion 3"]
}}

Review: {review}
Star Rating: {rating}
"""

con.execute(f"CREATE PROMPT ('product-improvement-suggestions', '{improvement_suggestion_prompt}');")

query_suggestions = """
CREATE TABLE improvement_suggestions AS
SELECT 
    product_id,
    review_id,
    review_text,
    star_rating,
    llm_complete_json(
        {'model_name': 'product_review_model'},
        {'prompt_name': 'product-improvement-suggestions'},
        {'review': review_text, 'rating': star_rating}
    ) AS suggestions
FROM low_rating_sentiment;
"""
con.execute(query_suggestions)

suggestions_df = con.execute("SELECT * FROM improvement_suggestions").df()
print("Product Improvement Suggestions:")
suggestions_df.head()

# %% [markdown]
# ## Step 19: Visualize Low Ratings and Suggestions
# Visualize low ratings and explore improvement suggestions.

# %%
plt.figure(figsize=(10, 6))
low_rating_counts = lowest_reviews_df['Score'].value_counts().sort_index()

low_rating_counts.plot(kind='bar', color='coral')
plt.xticks(rotation=0, ha='center')
plt.title("Distribution of Low Ratings (1-2 stars)")
plt.xlabel("Star Rating")
plt.ylabel("Number of Reviews")
plt.show()

# Display improvement suggestions
print("\nCommon Improvement Suggestions:")
for product_suggestion in suggestions_df['suggestions']:
    suggestions = eval(product_suggestion)['suggestions']
    for suggestion in suggestions:
        print(f"- {suggestion}")
