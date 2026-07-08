# Flock documentation

This site is built with [Mintlify](https://mintlify.com).

## Local preview

```bash
npm i -g mint
cd docs
mint dev
```

Open [http://localhost:3000](http://localhost:3000).

## Deploy

Connect the repository to Mintlify with the [GitHub app](https://dashboard.mintlify.com/settings/organization/github-app).

Point the docs root at the `docs/` directory. Changes on the default branch deploy automatically.

## Writing

- Pages are MDX with YAML frontmatter
- Navigation and branding live in `docs.json`
- See `AGENTS.md` for project writing preferences
