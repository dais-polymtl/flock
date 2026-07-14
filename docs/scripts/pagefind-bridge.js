/**
 * Mintlify Cmd+K search UI powered by Pagefind.
 * Intercepts search shortcuts and renders Mintlify's modal markup locally,
 * since static exports under a GitHub Pages subpath cannot use Mintlify cloud search.
 */
(function () {
  const config = (() => {
    const el = document.getElementById('pagefind-bridge-config');
    if (el) {
      try {
        return JSON.parse(el.textContent || '{}');
      } catch {
        return {};
      }
    }
    const match = document.documentElement.innerHTML.match(/var b="([^"]*)"/);
    const basePath = match ? match[1] : '';
    return {
      basePath,
      bundlePath: `${basePath}/pagefind/`,
    };
  })();

  const basePath = config.basePath || '';
  const bundlePath = config.bundlePath || `${basePath}/pagefind/`;
  const pagefindModuleUrl = `${bundlePath}pagefind.js`;

  const SEARCH_ICON =
    '<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 18 18" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true" class="absolute left-3 top-1/2 -translate-y-1/2 size-4 text-gray-500 dark:text-gray-400"><path d="M15.25 15.25L11.285 11.285"></path><path d="M7.75 12.75C10.5114 12.75 12.75 10.5114 12.75 7.75C12.75 4.98858 10.5114 2.75 7.75 2.75C4.98858 2.75 2.75 4.98858 2.75 7.75C2.75 10.5114 4.98858 12.75 7.75 12.75Z"></path></svg>';

  const DOC_ICON =
    '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="size-4 text-gray-500 dark:text-gray-400"><path d="M15 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7Z"></path><path d="M14 2v4a2 2 0 0 0 2 2h4"></path></svg>';

  const ITEM_CLASS =
    'flex items-start gap-2 px-2 py-1.5 rounded-[calc(var(--rounded-search,1.25rem)-0.375rem)] cursor-pointer outline-hidden focus:outline-hidden! focus-visible:outline-hidden! last:mb-2 data-highlighted:bg-black/[0.03] dark:data-highlighted:bg-white/5';

  const MODAL_CLASS =
    'origin-center will-change-[transform,opacity] transition-[transform,opacity] duration-[250ms] ease-[cubic-bezier(0.22,1,0.36,1)] flex flex-col mx-auto mt-1.5 max-w-[640px] overflow-hidden bg-background-light dark:bg-background-dark rounded-search ring-1 ring-gray-950/[0.06] dark:ring-white/[0.06] shadow-[0_57px_34px_0_rgba(0,0,0,0.01),0_25px_25px_0_rgba(0,0,0,0.02),0_6px_14px_0_rgba(0,0,0,0.02)]';

  let pagefind = null;
  let pagefindReady = null;
  let overlay = null;
  let activeInput = null;
  let activeList = null;
  let activeResults = [];
  let activeIndex = -1;
  let activeRequest = 0;
  let isOpen = false;

  function ensurePagefind() {
    if (!pagefindReady) {
      pagefindReady = import(pagefindModuleUrl)
        .then(async (mod) => {
          pagefind = mod;
          await pagefind.options({
            basePath: bundlePath,
            baseUrl: basePath || '/',
          });
          await pagefind.init();
          return pagefind;
        })
        .catch((error) => {
          console.error('Pagefind failed to load', error);
          pagefindReady = null;
          throw error;
        });
    }
    return pagefindReady;
  }

  function stripHtml(html) {
    const template = document.createElement('template');
    template.innerHTML = html;
    return template.content.textContent || '';
  }

  function withBasePath(url) {
    if (!url || !basePath) return url;
    if (/^https?:\/\//i.test(url)) return url;
    if (url.startsWith(basePath)) return url;
    if (url.startsWith('/')) return `${basePath}${url}`;
    return `${basePath}/${url}`;
  }

  /**
   * Expand Pagefind page hits into section hits when possible.
   * Pagefind `sub_results` split on headings that have `id` attributes and
   * include deep-link URLs (e.g. `/installation/#browser-limitations`).
   */
  function flattenToSectionHits(pageResults, limit = 8) {
    const items = [];
    for (const page of pageResults) {
      if (items.length >= limit) break;

      const pageTitle = page.meta?.title || page.url;
      const pageExcerpt = page.plain_excerpt || stripHtml(page.excerpt) || '';
      const subs = Array.isArray(page.sub_results) ? page.sub_results : [];

      if (!subs.length) {
        items.push({
          title: pageTitle,
          excerpt: pageExcerpt,
          url: page.url,
        });
        continue;
      }

      for (const sub of subs) {
        if (items.length >= limit) break;
        const hasAnchor = typeof sub.url === 'string' && sub.url.includes('#');
        const sectionTitle = sub.title && hasAnchor ? sub.title : null;
        items.push({
          title: sectionTitle ? `${pageTitle} › ${sectionTitle}` : pageTitle,
          excerpt: sub.plain_excerpt || stripHtml(sub.excerpt) || pageExcerpt,
          url: sub.url || page.url,
        });
      }
    }
    return items;
  }

  function setHighlighted(index) {
    if (!activeList) return;
    const items = activeList.querySelectorAll('[data-pagefind-result]');
    items.forEach((item, i) => {
      if (i === index) {
        item.setAttribute('data-highlighted', '');
      } else {
        item.removeAttribute('data-highlighted');
      }
    });
    activeIndex = index;
    const current = items[index];
    if (current) {
      current.scrollIntoView({ block: 'nearest' });
    }
  }

  function renderEmpty(message) {
    if (!activeList) return;
    activeList.innerHTML = `<div class="px-3 py-6 text-sm text-center text-gray-500 dark:text-gray-400">${message}</div>`;
    activeResults = [];
    activeIndex = -1;
  }

  function renderResults(results) {
    if (!activeList) return;
    if (!results.length) {
      renderEmpty('No results found');
      return;
    }

    activeList.innerHTML = results
      .map((result, index) => {
        const title = result.title || result.url;
        const excerpt = result.excerpt || '';
        const href = withBasePath(result.url);
        return `<a href="${href}" data-pagefind-result data-index="${index}" class="${ITEM_CLASS}" tabindex="-1">
  <span data-component-part="search-item-icon" class="flex h-5 items-center shrink-0" aria-hidden="true">${DOC_ICON}</span>
  <div class="flex min-w-0 flex-col gap-0.5">
    <span data-component-part="search-item-title" class="truncate text-sm font-medium leading-5 tracking-[-0.1px] text-gray-950 dark:text-white [&_mark]:bg-transparent [&_mark]:text-primary dark:[&_mark]:text-primary-light">${title}</span>
    <span class="truncate text-xs leading-4 text-gray-500 dark:text-gray-400 [&_mark]:bg-transparent [&_mark]:text-primary dark:[&_mark]:text-primary-light">${excerpt}</span>
  </div>
</a>`;
      })
      .join('');

    activeResults = results;
    setHighlighted(0);
  }

  async function runSearch(query) {
    if (!activeList) return;
    const requestId = ++activeRequest;

    if (!query.trim()) {
      renderEmpty('Start typing to search the docs');
      return;
    }

    renderEmpty('Searching…');

    try {
      const pf = await ensurePagefind();
      const search = await pf.debouncedSearch(query.trim());
      if (!search || requestId !== activeRequest) return;

      // Fetch a few more pages so section expansion can still fill ~8 slots
      const pageResults = await Promise.all(
        search.results.slice(0, 8).map((result) => result.data()),
      );
      if (requestId !== activeRequest) return;
      renderResults(flattenToSectionHits(pageResults, 8));
    } catch {
      if (requestId !== activeRequest) return;
      renderEmpty('Search is unavailable offline');
    }
  }

  function onInputKeyDown(event) {
    if (!activeList) return;

    const items = activeList.querySelectorAll('[data-pagefind-result]');
    if (event.key === 'ArrowDown') {
      event.preventDefault();
      if (!items.length) return;
      const next = activeIndex < items.length - 1 ? activeIndex + 1 : 0;
      setHighlighted(next);
      return;
    }
    if (event.key === 'ArrowUp') {
      event.preventDefault();
      if (!items.length) return;
      const next = activeIndex > 0 ? activeIndex - 1 : items.length - 1;
      setHighlighted(next);
      return;
    }
    if (event.key === 'Enter') {
      if (activeIndex >= 0 && items[activeIndex]) {
        event.preventDefault();
        items[activeIndex].click();
      }
    }
    if (event.key === 'Escape') {
      event.preventDefault();
      closeSearch();
    }
  }

  function createOverlay() {
    if (overlay) return overlay;

    overlay = document.createElement('div');
    overlay.id = 'pagefind-search-overlay';
    overlay.setAttribute('data-pagefind-overlay', 'true');
    overlay.innerHTML = `<div role="presentation" data-pagefind-backdrop class="fixed inset-0 bg-black/20 dark:bg-black/40 transition-opacity duration-[250ms] ease-[cubic-bezier(0.22,1,0.36,1)]"></div>
<div role="presentation" class="fixed inset-0 z-40 overflow-y-auto p-4 sm:p-6 md:p-12">
  <div data-open="" role="dialog" tabindex="-1" data-component-part="search-modal" aria-label="Search..." class="${MODAL_CLASS}">
    <div data-component-part="search-header" class="pt-1.5 px-1.5 relative z-10 border-b border-transparent h-14 transition">
      <div data-component-part="search-cli-state" class="relative h-full w-full flex items-center pt-1.5 pr-2 pb-1.5 pl-1.5 rounded-2xl border border-gray-200 dark:border-white/10 bg-background-light dark:bg-background-dark">
        ${SEARCH_ICON}
        <input type="search" data-pagefind-input placeholder="Search..." autocomplete="off" spellcheck="false" class="w-full bg-transparent pl-[30px] pr-3 text-base font-medium leading-6 tracking-[-0.2px] text-gray-950 dark:text-white placeholder:text-gray-400 dark:placeholder:text-gray-600 outline-none border-0" />
      </div>
    </div>
    <div tabindex="-1" aria-orientation="vertical" role="listbox" data-component-part="search-list" class="max-h-[calc(100vh-10rem)] overflow-y-auto px-1.5 pt-1.5 select-none"></div>
    <p id="search-keyboard-hints" class="sr-only">Use the up and down arrow keys to select a result, Enter to open it, and Escape to close search.</p>
  </div>
</div>`;

    document.body.appendChild(overlay);

    activeInput = overlay.querySelector('[data-pagefind-input]');
    activeList = overlay.querySelector('[data-component-part="search-list"]');

    activeInput.addEventListener('input', () => runSearch(activeInput.value));
    activeInput.addEventListener('keydown', onInputKeyDown);

    overlay.querySelector('[data-pagefind-backdrop]').addEventListener('click', closeSearch);
    overlay.querySelector('[role="presentation"].z-40').addEventListener('click', (event) => {
      if (event.target === event.currentTarget) closeSearch();
    });

    return overlay;
  }

  function closeMintlifySearch() {
    document.querySelectorAll('[data-component-part="search-modal"]').forEach((modal) => {
      const root = modal.closest('[role="presentation"]')?.parentElement;
      if (root && root !== overlay) {
        root.remove();
      }
    });
  }

  function openSearch() {
    if (isOpen) {
      activeInput?.focus();
      return;
    }

    closeMintlifySearch();
    createOverlay();
    isOpen = true;
    overlay.style.display = '';
    document.documentElement.style.overflow = 'hidden';
    renderEmpty('Start typing to search the docs');
    ensurePagefind().catch(() => renderEmpty('Search is unavailable offline'));
    window.setTimeout(() => activeInput?.focus(), 0);
  }

  function closeSearch() {
    if (!isOpen) return;
    isOpen = false;
    activeRequest += 1;
    activeResults = [];
    activeIndex = -1;
    if (overlay) {
      overlay.style.display = 'none';
    }
    document.documentElement.style.overflow = '';
    if (activeInput) {
      activeInput.value = '';
    }
  }

  function hijackMintlifyModal(modal) {
    if (modal.closest('#pagefind-search-overlay')) return;
    const cliState = modal.querySelector('[data-component-part="search-cli-state"]');
    const list = modal.querySelector('[data-component-part="search-list"]');
    if (!cliState || !list || cliState.querySelector('[data-pagefind-input]')) return;

    activeInput = null;
    activeList = list;

    cliState.innerHTML = `${SEARCH_ICON}<input type="search" data-pagefind-input placeholder="Search..." autocomplete="off" spellcheck="false" class="w-full bg-transparent pl-[30px] pr-3 text-base font-medium leading-6 tracking-[-0.2px] text-gray-950 dark:text-white placeholder:text-gray-400 dark:placeholder:text-gray-600 outline-none border-0" />`;

    activeInput = cliState.querySelector('[data-pagefind-input]');
    activeInput.addEventListener('input', () => runSearch(activeInput.value));
    activeInput.addEventListener('keydown', onInputKeyDown);

    renderEmpty('Start typing to search the docs');
    ensurePagefind().catch(() => renderEmpty('Search is unavailable offline'));
    window.setTimeout(() => activeInput?.focus(), 0);
  }

  function isMintlifyModalOpen(modal) {
    return modal.hasAttribute('data-open') && modal.getAttribute('data-open') !== 'false';
  }

  function scanMintlifyModals() {
    document.querySelectorAll('[data-component-part="search-modal"]').forEach((modal) => {
      if (modal.closest('#pagefind-search-overlay')) return;
      if (isMintlifyModalOpen(modal)) {
        hijackMintlifyModal(modal);
      }
    });
  }

  function isSearchTrigger(target) {
    return target instanceof Element && !!target.closest('#search-bar-entry, #search-bar-entry-mobile');
  }

  const THEME_SELECTED_CLASSES = [
    'text-gray-600',
    'dark:text-gray-200',
    'bg-gray-200/50',
    'dark:bg-gray-900',
  ];
  const THEME_UNSELECTED_CLASSES = [
    'text-gray-400',
    'dark:text-gray-500',
    'hover:text-gray-600',
    'dark:hover:text-gray-300',
  ];

  function themeButtons() {
    return document.querySelectorAll(
      'button[aria-label="Switch to system theme"], button[aria-label="Switch to light theme"], button[aria-label="Switch to dark theme"]',
    );
  }

  function syncThemeToggleUi(mode) {
    const normalized = (mode || '').toLowerCase();
    themeButtons().forEach((button) => {
      const label = button.getAttribute('aria-label') || '';
      const match = label.match(/^Switch to (system|light|dark) theme$/i);
      if (!match) return;
      const isActive = match[1].toLowerCase() === normalized;
      button.setAttribute('aria-pressed', isActive ? 'true' : 'false');
      THEME_SELECTED_CLASSES.forEach((cls) => button.classList.toggle(cls, isActive));
      THEME_UNSELECTED_CLASSES.forEach((cls) => button.classList.toggle(cls, !isActive));
    });
  }

  function applyThemeMode(mode) {
    if (!mode) return;

    const normalized = mode.toLowerCase();
    const html = document.documentElement;
    const resolved =
      normalized === 'system'
        ? window.matchMedia('(prefers-color-scheme: dark)').matches
          ? 'dark'
          : 'light'
        : normalized;

    localStorage.setItem('isDarkMode', normalized);
    html.classList.remove('light', 'dark');
    html.classList.add(resolved);
    html.style.colorScheme = resolved;
    syncThemeToggleUi(normalized);
    window.dispatchEvent(new Event('storage'));
  }

  function themeModeFromTarget(target) {
    if (!(target instanceof Element)) return null;
    const button = target.closest('button');
    if (!button) return null;
    const label = button.getAttribute('aria-label') || '';
    const match = label.match(/^Switch to (system|light|dark) theme$/i);
    return match ? match[1].toLowerCase() : null;
  }

  function handleOpen(event) {
    const themeMode = themeModeFromTarget(event.target);
    if (themeMode) {
      event.preventDefault();
      event.stopPropagation();
      applyThemeMode(themeMode);
      return;
    }

    if (isSearchTrigger(event.target)) {
      event.preventDefault();
      event.stopPropagation();
      openSearch();
      return;
    }

    if ((event.metaKey || event.ctrlKey) && event.key.toLowerCase() === 'k') {
      event.preventDefault();
      event.stopPropagation();
      if (isOpen) {
        closeSearch();
      } else {
        openSearch();
      }
    }
  }

  document.addEventListener('click', handleOpen, true);
  document.addEventListener('keydown', handleOpen, true);

  document.addEventListener(
    'keydown',
    (event) => {
      if (event.key === 'Escape' && isOpen) {
        event.preventDefault();
        event.stopPropagation();
        closeSearch();
      }
    },
    true,
  );

  const modalObserver = new MutationObserver(scanMintlifyModals);
  modalObserver.observe(document.documentElement, {
    subtree: true,
    attributes: true,
    attributeFilter: ['data-open'],
  });

  function boot() {
    try {
      sessionStorage.removeItem('flock-sidebar-scroll');
    } catch {
      // ignore
    }
    const storedTheme = (localStorage.getItem('isDarkMode') || 'system').toLowerCase();
    if (storedTheme === 'system' || storedTheme === 'light' || storedTheme === 'dark') {
      syncThemeToggleUi(storedTheme);
    }
    ensurePagefind().catch(() => {});
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', boot);
  } else {
    boot();
  }
})();
