#!/usr/bin/env node
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';

const repoRoot = path.resolve(path.dirname(new URL(import.meta.url).pathname), '../..');
const siteRoot = process.env.SITE_DIR
  ? path.resolve(process.env.SITE_DIR)
  : path.join(repoRoot, 'docs-site');
const basePath = (process.env.GITHUB_PAGES_BASE_PATH || '/flock').replace(/\/$/, '');
const port = Number(process.env.PORT || 3000);

const mimeTypes = {
  '.html': 'text/html',
  '.css': 'text/css',
  '.js': 'application/javascript',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.gif': 'image/gif',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2',
  '.ttf': 'font/ttf',
  '.webp': 'image/webp',
  '.wasm': 'application/wasm',
  '.txt': 'text/plain',
  '.xml': 'application/xml',
  '.pf_meta': 'application/octet-stream',
};

function safePath(decodedPath) {
  const normalized = decodedPath.replace(/^\/+/, '');
  const candidate = path.resolve(siteRoot, normalized);
  if (!candidate.startsWith(siteRoot + path.sep) && candidate !== siteRoot) return null;
  return candidate;
}

function tryFile(filePath) {
  if (fs.existsSync(filePath) && fs.statSync(filePath).isFile()) return filePath;
  return null;
}

const server = http.createServer((req, res) => {
  try {
    const rawPath = (req.url || '/').split('?')[0].split('#')[0] || '/';
    const decodedPath = decodeURIComponent(rawPath);

    if (decodedPath === '/' || decodedPath === '') {
      res.writeHead(302, { Location: `${basePath}/` });
      res.end();
      return;
    }

    if (!decodedPath.startsWith(basePath)) {
      res.writeHead(404, { 'Content-Type': 'text/plain' });
      res.end(`Not Found. Open ${basePath}/`);
      return;
    }

    const relativePath = decodedPath.slice(basePath.length) || '/';
    const resolved = safePath(relativePath);
    if (!resolved) {
      res.writeHead(400, { 'Content-Type': 'text/plain' });
      res.end('Bad request');
      return;
    }

    const filePath =
      tryFile(resolved) ||
      tryFile(path.join(resolved, 'index.html')) ||
      tryFile(`${resolved}.html`);

    if (!filePath) {
      res.writeHead(404, { 'Content-Type': 'text/plain' });
      res.end('Not Found');
      return;
    }

    res.writeHead(200, {
      'Content-Type': mimeTypes[path.extname(filePath).toLowerCase()] || 'application/octet-stream',
    });
    fs.createReadStream(filePath).pipe(res);
  } catch {
    res.writeHead(500, { 'Content-Type': 'text/plain' });
    res.end('Server error');
  }
});

server.listen(port, () => {
  console.log(`Serving ${siteRoot} at http://localhost:${port}${basePath}/`);
});
