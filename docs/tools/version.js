/**
 * ASP — Shared version stamp for web surfaces.
 *
 * Single source-of-truth for the version string displayed by the simulator
 * and the two builders. The /release skill rewrites ASP_VERSION on every
 * version bump (in lockstep with include/version.h, package.json,
 * app/pubspec.yaml, awesome-task-system/VERSION). Do not hand-edit.
 *
 * Usage: drop a `<span class="asp-version-stamp"></span>` anywhere on the
 * page and load this file. The stamp is filled in on DOMContentLoaded.
 */
(function () {
  'use strict';

  const ASP_VERSION = 'v0.4.1';

  window.ASP_VERSION = ASP_VERSION;

  function fill() {
    const nodes = document.querySelectorAll('.asp-version-stamp');
    nodes.forEach((n) => { n.textContent = ASP_VERSION; });
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', fill);
  } else {
    fill();
  }
})();
