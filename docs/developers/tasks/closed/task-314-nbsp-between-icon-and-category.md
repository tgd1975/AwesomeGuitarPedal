---
id: TASK-314
title: Use non-breaking space between category icon and name
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: XS (<30m)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: No
---

## Description

The Category column in `docs/developers/ideas/OVERVIEW.md` currently
joins the emoji and the category name with a regular space, so a
narrow viewport can wrap the name to a new line and orphan the icon.
Switch to a non-breaking space (U+00A0) so the pair always renders
together.

## Acceptance Criteria

- [ ] `format_category` joins icon and name with U+00A0.
- [ ] Existing tests are updated to reflect the NBSP and continue to pass.

## Test Plan

**Host tests** — update assertions in
`test_update_idea_overview.py` to match the new separator.
