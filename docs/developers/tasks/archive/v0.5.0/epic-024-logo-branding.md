---
id: EPIC-024
name: logo-branding
title: Logo and branding consistency across apps and tools
status: open
opened: 2026-05-01
closed:
assigned:
branch: feature/logo-branding
---

Bring the ASP mark to every user-facing surface — web simulator, profile
builder, configuration builder, mobile app AppBar, Android launcher icon,
and the GitHub social preview — so the project reads as one product
across tools.

Seeded by IDEA-049 (Make the logo more prominent across apps and tools).

## Design authority

The design system in [`docs/design/`](../../../design/) is the law for
every task in this epic. Specifically:

- **Asset source** — [`docs/design/handoff/logo/`](../../../design/handoff/logo/)
  holds the four canonical files: `asp-mark.svg` (primary mark, 64×64),
  `asp-monogram.svg` (compact / app-icon use), `asp-wordmark.svg`, and
  `asp-lockup.svg` (mark + wordmark horizontal). Tasks must use these
  verbatim — never redraw, recolour, or simplify.
- **Non-negotiables** — see
  [`docs/design/handoff/README.md`](../../../design/handoff/README.md#non-negotiables).
  Most relevant here: rule 6 ("Top-Down Build mark — do not redraw,
  reinterpret, or simplify"), rule 2 (amber `#F0B030` is the only
  saturated colour), and rule 5 (CamelCase wordmark only).
- **Visual reference** — [`docs/design/ASP-design-review.html`](../../../design/ASP-design-review.html)
  shows every header / AppBar / launcher mock-up at the intended size,
  spacing, and treatment. When a task says "header logo", that file is
  what it must look like.
- **Component spec** — [`docs/design/handoff/components/COMPONENTS.md`](../../../design/handoff/components/COMPONENTS.md)
  for dimensions and behaviour of recurring components (header, AppBar).

If a task implementation appears to require deviating from the guide,
**stop and ask** — either the guide needs updating first (it's a
snapshot of a canonical project — see the handoff README's "Questions"
section), or the task scope is wrong. Do not silently invent a variant.

## Tasks

Tasks are listed automatically in the Task Epics section of
`docs/developers/tasks/OVERVIEW.md` and in `EPICS.md` / `KANBAN.md`.
