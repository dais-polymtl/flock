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
3. Indexes the site with [Pagefind](https://pagefind.app/) (free, client-side search)
4. Injects a Pagefind bridge for Cmd+K search (Mintlify cloud search is unavailable on static export)
5. Publishes to GitHub Pages via `actions/deploy-pages`

You can also trigger a deploy manually from the **Website Deploy to GitHub Pages** workflow in GitHub Actions.

### Local export test

```bash
cd docs
TMPDIR=/tmp mint export --output ../docs-site.zip
mkdir -p ../docs-site && unzip -q ../docs-site.zip -d ../docs-site
node scripts/prepare-github-pages.mjs ../docs-site
npx -y pagefind --site ../docs-site --output-subdir pagefind
```

### Build and serve in one command

```bash
./docs/scripts/build-and-serve.sh
```

Optional environment variables:
- `PORT` (default `3000`)
- `GITHUB_PAGES_BASE_PATH` (default `/flock`)
- `SITE_DIR` (default `./docs-site`)
- `SITE_ZIP` (default `./docs-site.zip`)
- `DOCS_TMPDIR` (default `/tmp`)

Then serve `../docs-site` and open `http://localhost:3000/flock/`.

**Note:** Static exports cannot use Mintlify cloud search (the export shows "Login into CLI to enable search"). The injected bridge intercepts Cmd+K and the search bar, opens Mintlify-styled search UI, and runs queries through Pagefind. Use `mint dev` for the full hosted Mintlify preview experience locally.

If your shell points `TMPDIR` to a small ramdisk (for example `/private/ramdisk`), `mint export` can fail with `ENOSPC` even when your main disk has free space. Prefixing with `TMPDIR=/tmp` avoids that by using disk-backed temp storage.

## Writing

- Pages are MDX with YAML frontmatter
- Navigation and branding live in `docs.json`
- See `AGENTS.md` for project writing preferences
