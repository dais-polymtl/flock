#!/usr/bin/env node
/**
 * Prepare a Mintlify static export for GitHub Pages project-site hosting.
 * - Rewrites root-absolute paths for /flock
 * - Copies and injects the Pagefind search bridge
 */

import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const siteDir = process.argv[2];
const basePath = (process.env.GITHUB_PAGES_BASE_PATH ?? '/flock').replace(/\/$/, '');
const scriptsDir = path.dirname(fileURLToPath(import.meta.url));

if (!siteDir) {
  console.error('Usage: node prepare-github-pages.mjs <site-directory>');
  process.exit(1);
}

const TEXT_EXTENSIONS = new Set(['.html']);
const SKIP_PATHS = ['/pagefind-bridge.js'];
const resolvedSiteDir = path.resolve(siteDir);

const bridgeConfig = JSON.stringify({
  basePath,
  bundlePath: `${basePath}/pagefind/`,
});

const bridgeInjection = `<script id="pagefind-bridge-config" type="application/json">${bridgeConfig}</script>
<script defer src="${basePath}/pagefind-bridge.js"></script>`;

function rewriteContent(content) {
  let updated = content.replace(/var b=""/g, `var b="${basePath}"`);

  // Attributes like href="/..." and src="/..."
  updated = updated.replace(
    /(href|src|content|action)=(["'])\/(?!\/|flock\/)/g,
    (match, attr, quote) => `${attr}=${quote}${basePath}/`,
  );

  if (updated.includes('</body>') && !updated.includes('pagefind-bridge.js')) {
    updated = updated.replace('</body>', `${bridgeInjection}\n</body>`);
  }

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
    const relPath = path.relative(resolvedSiteDir, fullPath).split(path.sep).join('/');
    if (SKIP_PATHS.some((skip) => relPath.endsWith(skip.replace(/^\//, '')))) {
      continue;
    }
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

await fs.copyFile(
  path.join(scriptsDir, 'pagefind-bridge.js'),
  path.join(resolvedSiteDir, 'pagefind-bridge.js'),
);
await walk(resolvedSiteDir);
await fs.writeFile(path.join(resolvedSiteDir, '.nojekyll'), '');

console.log(`Prepared GitHub Pages site in ${resolvedSiteDir} with base path ${basePath}`);
