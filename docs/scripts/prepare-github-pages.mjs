#!/usr/bin/env node
/**
 * Rewrite root-absolute asset and navigation paths for GitHub Pages project sites.
 *
 * GitHub Pages serves dais-polymtl/flock at https://dais-polymtl.github.io/flock/
 * Mintlify's static export uses root-absolute paths (e.g. /_next/static/...).
 */

import fs from 'node:fs/promises';
import path from 'node:path';

const siteDir = process.argv[2];
const basePath = (process.env.GITHUB_PAGES_BASE_PATH ?? '/flock').replace(/\/$/, '');

if (!siteDir) {
  console.error('Usage: node prepare-github-pages.mjs <site-directory>');
  process.exit(1);
}

const TEXT_EXTENSIONS = new Set(['.html', '.js', '.css', '.json', '.xml', '.txt', '.md']);

function rewriteContent(content) {
  let updated = content.replace(/var b=""/g, `var b="${basePath}"`);

  // Attributes like href="/..." and src="/..."
  updated = updated.replace(
    /(href|src|content|action)=(["'])\/(?!\/|flock\/)/g,
    (match, attr, quote) => `${attr}=${quote}${basePath}/`,
  );

  // Quoted root-absolute paths in JS/CSS/JSON (e.g. "/_next/static/...")
  updated = updated.replace(
    /(["'])\/(?!\/|flock(?:\/|"))/g,
    (match, quote) => `${quote}${basePath}/`,
  );

  return updated;
}

async function walk(dir) {
  const entries = await fs.readdir(dir, { withFileTypes: true });
  for (const entry of entries) {
    const fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      await walk(fullPath);
      continue;
    }

    const ext = path.extname(entry.name).toLowerCase();
    if (!TEXT_EXTENSIONS.has(ext)) {
      continue;
    }

    const original = await fs.readFile(fullPath, 'utf8');
    const rewritten = rewriteContent(original);
    if (rewritten !== original) {
      await fs.writeFile(fullPath, rewritten, 'utf8');
    }
  }
}

await walk(path.resolve(siteDir));
await fs.writeFile(path.join(path.resolve(siteDir), '.nojekyll'), '');

console.log(`Prepared GitHub Pages site in ${path.resolve(siteDir)} with base path ${basePath}`);
