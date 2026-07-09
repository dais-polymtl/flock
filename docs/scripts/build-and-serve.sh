#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCS_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_DIR="$(cd "${DOCS_DIR}/.." && pwd)"

SITE_DIR="${SITE_DIR:-${REPO_DIR}/docs-site}"
SITE_ZIP="${SITE_ZIP:-${REPO_DIR}/docs-site.zip}"
BASE_PATH="${GITHUB_PAGES_BASE_PATH:-/flock}"
PORT="${PORT:-3000}"
DOCS_TMPDIR="${DOCS_TMPDIR:-/tmp}"

if ! command -v mint >/dev/null 2>&1; then
  echo "error: mint CLI is not installed. Run: npm i -g mint"
  exit 1
fi

echo "==> Exporting Mintlify docs"
cd "${DOCS_DIR}"
TMPDIR="${DOCS_TMPDIR}" mint export --output "${SITE_ZIP}"

echo "==> Unpacking static site to ${SITE_DIR}"
rm -rf "${SITE_DIR}"
mkdir -p "${SITE_DIR}"
unzip -q -o "${SITE_ZIP}" -d "${SITE_DIR}"

echo "==> Preparing GitHub Pages paths (${BASE_PATH})"
GITHUB_PAGES_BASE_PATH="${BASE_PATH}" node "${SCRIPT_DIR}/prepare-github-pages.mjs" "${SITE_DIR}"

echo "==> Building Pagefind index"
npx -y pagefind --site "${SITE_DIR}" --output-subdir pagefind

echo "==> Serving docs"
echo "Open: http://localhost:${PORT}${BASE_PATH}/"
SITE_DIR="${SITE_DIR}" GITHUB_PAGES_BASE_PATH="${BASE_PATH}" PORT="${PORT}" \
  node "${SCRIPT_DIR}/serve-github-pages-site.mjs"
