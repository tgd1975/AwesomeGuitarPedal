---
id: TASK-339
title: Add ASP logo to web simulator header
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: No
epic: logo-branding
order: 1
---

## Description

The web simulator at `docs/simulator/index.html` currently has a text-only
header. Add the canonical ASP mark (`asp-mark.svg`) next to the `<h1>` title,
linked back to the GitHub repo root. Also generate and reference a `favicon.ico`
so browser tabs show the project mark.

Per IDEA-049 decisions: copy the SVG into the simulator directory rather than
relative-import it (one source, multiple copies — agreed trade-off for
simplicity over DRY).

## Acceptance Criteria

- [ ] `asp-mark.svg` copied to `docs/simulator/` and rendered to the left of the `<h1>` title
- [ ] Logo wrapped in an `<a href="https://github.com/...">` linking to the repo root
- [ ] `favicon.ico` generated from `asp-mark.svg` and referenced via `<link rel="icon">`
- [ ] Logo height matches the H1 line-height — no layout shift versus current header

## Test Plan

No automated tests required — change is non-functional (static markup + asset).

Manual verification: open `docs/simulator/index.html` in a browser, confirm the
mark renders next to the title, the favicon shows in the tab, and clicking the
mark navigates to the GitHub repo.

## Notes

- **Design conformance is the law.** Use [`docs/design/handoff/logo/asp-mark.svg`](../../../../docs/design/handoff/logo/asp-mark.svg) verbatim — no redraw, recolour, or simplification (handoff README non-negotiable #6). The intended header treatment is shown in [`docs/design/ASP-design-review.html`](../../../../docs/design/ASP-design-review.html); see the EPIC-024 "Design authority" section for the full rule set.
- Use `convert` (ImageMagick) or an online SVG → ICO tool for the favicon — verify the tool is installed before scripting it; if not, ask.
- Header markup lives in [docs/simulator/index.html](docs/simulator/index.html); shared styles in [docs/simulator/styles.css](docs/simulator/styles.css).
