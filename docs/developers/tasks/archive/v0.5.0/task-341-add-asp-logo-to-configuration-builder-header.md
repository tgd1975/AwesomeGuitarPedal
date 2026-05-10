---
id: TASK-341
title: Add ASP logo to configuration builder header
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
epic: logo-branding
order: 3
---

## Description

The configuration builder at `docs/tools/configuration-builder/` currently has a
text-only `.asp-header`. Add the canonical ASP mark (`asp-mark.svg`) next to the
`<h1>` title, linked back to the GitHub repo root. Also generate and reference a
`favicon.ico`.

Per IDEA-049 decisions: copy the SVG into the tool's directory.

## Acceptance Criteria

- [ ] `asp-mark.svg` copied to `docs/tools/configuration-builder/` and rendered to the left of the `<h1>` title
- [ ] Logo wrapped in an `<a href="https://github.com/...">` linking to the repo root
- [ ] `favicon.ico` generated and referenced via `<link rel="icon">`
- [ ] No layout shift versus current header — logo height matches the H1 line-height

## Test Plan

No automated tests required — change is non-functional (static markup + asset).

Manual verification: open the configuration builder in a browser, confirm the
mark renders, favicon shows, and the link works.

## Notes

- Shares CSS with the profile builder via [docs/tools/asp.css](docs/tools/asp.css). Coordinate with TASK-340 on the `.asp-header` rule.
- **Design conformance is the law.** Use [`docs/design/handoff/logo/asp-mark.svg`](../../../../docs/design/handoff/logo/asp-mark.svg) verbatim — no redraw, recolour, or simplification (handoff README non-negotiable #6). The intended header treatment is shown in [`docs/design/ASP-design-review.html`](../../../../docs/design/ASP-design-review.html); see the EPIC-024 "Design authority" section for the full rule set.
