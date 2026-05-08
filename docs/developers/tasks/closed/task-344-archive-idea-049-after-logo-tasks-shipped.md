---
id: TASK-344
title: Archive IDEA-049 after logo tasks shipped
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: XS (<30m)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
epic: logo-branding
order: 6
prerequisites: [TASK-339, TASK-340, TASK-341, TASK-342, TASK-343, TASK-345]
---

## Description

Once the six concrete logo-rollout tasks (web simulator, profile builder,
configuration builder, mobile AppBar, social preview, Android launcher icon)
ship, the parent IDEA-049 has no remaining open questions. Archive it via
`/ts-idea-archive` so the open-ideas list reflects only undecided work.

## Acceptance Criteria

- [ ] All six prerequisite tasks closed
- [ ] `/ts-idea-archive IDEA-049` run, moving the file to `docs/developers/ideas/archived/`
- [ ] Ideas OVERVIEW.md regenerated and committed with the archive

## Test Plan

No automated tests required — administrative housekeeping only.

## Prerequisites

- **TASK-339** — Web simulator logo lands the canonical mark+favicon pattern the other web tools follow
- **TASK-340** — Profile builder applies the same pattern
- **TASK-341** — Configuration builder applies the same pattern
- **TASK-342** — Mobile AppBar shows the mark
- **TASK-343** — GitHub social preview configured
- **TASK-345** — Android launcher icon swapped to ASP mark

## Notes

- Source idea: [docs/developers/ideas/open/idea-049-logo-more-prominent.md](docs/developers/ideas/open/idea-049-logo-more-prominent.md).
- "Redesigning the logo" and "building a brand guide" are explicitly out of scope per IDEA-049; do not expand here. The brand guide already exists at [`docs/design/`](../../../../docs/design/) and is the law for the prerequisite tasks — see EPIC-024 "Design authority". This task is administrative only.
