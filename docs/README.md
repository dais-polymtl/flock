# Flock documentation

This site is built with [Mintlify](https://mintlify.com) and published to [GitHub Pages](https://dais-polymtl.github.io/flock/).

## Local preview

```bash
npm i -g mint
cd docs
mint dev
```

Open [http://localhost:3000](http://localhost:3000).

## Deploy

Docs deploy automatically when changes land on `main` under `docs/`. The workflow:

1. Runs `mint export` to generate a static site
2. Rewrites paths for the `/flock` GitHub Pages base URL
3. Publishes to GitHub Pages via `actions/deploy-pages`

You can also trigger a deploy manually from the **Website Deploy to GitHub Pages** workflow in GitHub Actions.

### Local export test

```bash
cd docs
mint export --output ../site.zip
mkdir -p ../site && unzip -q ../site.zip -d ../site
node scripts/prepare-github-pages.mjs ../site
```

Then serve `../site` with any static file server and open `http://localhost:3000/flock/`.

## Writing

- Pages are MDX with YAML frontmatter
- Navigation and branding live in `docs.json`
- See `AGENTS.md` for project writing preferences
