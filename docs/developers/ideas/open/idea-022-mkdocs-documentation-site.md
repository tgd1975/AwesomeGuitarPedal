---
id: IDEA-022
title: MkDocs Documentation Site
category: docs
description: Introduce MkDocs as a unified documentation platform for both developer-facing and public-facing content — replacing a growing flat collection of Markdown files with a navigable, searchable, CI-rendered site.
---

## Problem

The project's documentation has grown organically across many directories:

- `docs/builders/` — build guide, BOM
- `docs/developers/` — architecture, tasks, ideas, API
- `docs/media/` — diagrams, schematics

This works fine at small scale. But several in-flight ideas will significantly increase the
document count:

- **IDEA-015/016** — marketing material, journalist articles, builder articles, musician
  articles, developer articles (6+ long-form documents)
- **IDEA-019** — WireViz YAML sources, rendered SVGs, wiring reference pages
- **IDEA-020** — SysML v2 model files and rendered output
- **IDEA-021** — task-system documentation

A flat `docs/` tree of Markdown files has no navigation structure, no search, and no
distinction between what is "public" (a builder reading on GitHub) and what is "internal"
(a developer browsing the repo). As the document count grows past ~40 files, discoverability
breaks down.

---

## Proposed Solution

Introduce **MkDocs** with the **Material theme** as the docs platform.

MkDocs fits the existing docs-as-code posture of the project:

- Source stays as plain Markdown files — no migration of content required.
- Rendered as a static site in CI; deployed to GitHub Pages (or kept as build artefact).
- `mkdocs.yml` nav structure makes the information architecture explicit and reviewable in
  Git like any other file.
- Material theme adds search, tabbed navigation, callout blocks (`> **NOTE**`), and
  code highlighting with zero configuration overhead.
- IDEA-019 already names MkDocs as the target docs platform for wiring diagrams — adopting
  it here closes that open dependency.

### Information architecture (proposed)

```
nav:
  Builders:
    - Build Guide
    - Bill of Materials
    - Wiring Reference (IDEA-019)
  Users:
    - Profile Configuration
    - Firmware Flashing
  Articles:            ← IDEA-015/016 content
    - For Musicians
    - For Builders
    - For Developers
    - Feature Overview
  Developers:
    - Architecture
    - API Reference (Doxygen)
    - Ideas
    - Tasks
    - Contributing
```

### What stays out of the site

- Raw task files (`docs/developers/tasks/open/*.md`) — these remain repo-internal files,
  not published to the site.
- Idea documents (`docs/developers/ideas/`) — likewise internal; not published.

---

## Decisions

### 1. Deployment target — GitHub Pages

**What this means:** When CI runs on `main`, it builds the MkDocs site and pushes the
rendered HTML to the `gh-pages` branch. GitHub then serves it at a public URL
(e.g. `https://<org>.github.io/AwesomeStudioPedal/`). Anyone — builders, musicians,
journalists — can read the docs without cloning the repo or installing anything.

The alternative would be to only build the site in CI to verify it renders without errors,
but never publish it anywhere. That catches broken links and missing files but produces no
user-facing output.

**Decision: deploy to GitHub Pages.** The articles from IDEA-015/016 only have value if
external readers can reach them. A local-only build would defeat the purpose.

Implication: the repo needs a `gh-pages` branch and a GitHub Actions workflow
(`docs.yml`) that runs `mkdocs gh-deploy` on every push to `main`.

### 2. Versioning — single `latest`

No versioned docs per firmware release. The `mike` plugin and a version switcher add
maintenance overhead that is not justified at the current project scale. The site always
reflects the `main` branch. If versioning becomes necessary later (e.g. when shipping
tagged firmware releases to a wider audience), it can be added incrementally.

### 3. Doxygen integration — embedded sub-site

The generated Doxygen HTML is copied into `site/api/` during the MkDocs build so it is
served under the same domain. A top-level nav entry ("API Reference") links into it.
No iframe; the Doxygen output is a first-class part of the site, just in a sub-directory
that MkDocs does not own.

Build order: `doxygen` runs first, output lands in `docs/api/` (gitignored), then
`mkdocs build` copies everything under `docs/` including `docs/api/` into `site/`.

---

## Scope of changes

Introducing MkDocs is not just adding `mkdocs.yml`. Every layer of the project toolchain
that currently assumes "docs = Markdown files in `docs/`" needs to be updated.

### CI / GitHub Actions

| File | Change |
|---|---|
| `.github/workflows/docs.yml` | New workflow: build MkDocs + Doxygen, deploy to `gh-pages` on push to `main`; build-only (no deploy) on PRs to catch broken links. |
| `.github/workflows/ci.yml` | Add `mkdocs build --strict` step so broken nav or missing pages fail the PR build. |

### Pre-commit hooks

| Hook | Change |
|---|---|
| Markdown lint | Scope to `docs/` source files only; exclude `site/` (generated) and `docs/api/` (Doxygen). |
| Link checker | Add `mkdocs build --strict` or `markdown-link-check` scoped to source files. Replace any ad-hoc link-check scripts. |
| Stale SVG check | Already planned in IDEA-019; ensure the WireViz regeneration hook runs before the MkDocs build hook. |

### Makefile targets

| Target | Change |
|---|---|
| `make docs` | Run `doxygen` then `mkdocs build`. |
| `make docs-serve` | Run `mkdocs serve` for local preview with hot-reload. |
| `make docs-deploy` | Run `mkdocs gh-deploy` (CI uses this; developers can run manually). |

### Skills (`.claude/skills/`)

| Skill | Change |
|---|---|
| `fix-markdown` | Exclude `site/` and `docs/api/` from scan. |
| `lint` | Add MkDocs build check (`mkdocs build --strict`) to the lint suite. |
| `doc-check` | Rewrite: currently checks individual Markdown files; extend to validate `mkdocs.yml` nav completeness (every file in nav must exist; every public doc must appear in nav). |
| `release` | After tagging, trigger `make docs-deploy` so the published site reflects the release. |

### Scripts (`scripts/`)

| Script | Change |
|---|---|
| Any script that enumerates `docs/**/*.md` | Update glob to exclude `site/` and `docs/api/`. |
| `scripts/check-links.sh` (if it exists) | Replace with `mkdocs build --strict` or remove. |

### `.gitignore`

Add:

```
site/
docs/api/
```

### `devcontainer.json`

Add `mkdocs`, `mkdocs-material`, and `mkdocs-minify-plugin` to the post-create install
step so the container is ready to serve docs out of the box.

---

## Dependencies

| Idea | Dependency |
|---|---|
| IDEA-019 (WireViz) | Names MkDocs as the rendering platform — this idea delivers it. |
| IDEA-015/016 (Articles) | Long-form content needs navigation to be usable at all for external readers. |
| IDEA-020 (SysML v2) | Rendered SysML output needs a home in the site. |
