---
id: TASK-313
title: Add emoji icons to idea category column in OVERVIEW
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
---

## Description

Categories were added to the ideas OVERVIEW in TASK-305 but render as
plain text. Add an emoji prefix per known category so the table is
glance-scannable. Unknown / missing categories fall back to the bare
text (or em dash) without an icon.

Mapping:

- hardware → 🔧
- firmware → ⚡
- apps → 📱
- tooling → 🛠️
- docs → 📖
- outreach → 📣

## Acceptance Criteria

- [ ] `update_idea_overview.py` renders known categories with their
  emoji prefix in both Open and Archived tables.
- [ ] Unknown categories render as bare text; missing category renders
  as `—` (existing behaviour preserved).
- [ ] Tests in `test_update_idea_overview.py` cover the icon path and
  the unknown-category fallback.

## Test Plan

**Host tests** (`make test-host` → Python tests):

- Add cases asserting that a known category includes the emoji.
- Add a case asserting an unknown category is rendered without an
  emoji.

## Notes

Edit the package copy under `awesome-task-system/scripts/`, then sync.
