---
id: TASK-305
title: Add category field to ideas and surface it in OVERVIEW
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: Medium (2-8h)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: Clarification
---

## Description

Ideas in `docs/developers/ideas/{open,archived}/` currently carry only
`id`, `title`, `description`, and `status` in their frontmatter. Browsing
the 30+ open ideas in `OVERVIEW.md` is a flat list with no grouping —
hardware ideas, content/marketing, tooling, and product features are all
mixed together.

This task adds a `category:` frontmatter field to every idea (open and
archived), surfaces it as a column in the generated `OVERVIEW.md`, and
populates it across the existing corpus by clustering the ideas into
~5 categories.

## Acceptance Criteria

- [ ] `scripts/update_idea_overview.py` (canonical source under
  `awesome-task-system/scripts/`) reads a `category` field from idea
  frontmatter and renders it as a column in both the Open and Archived
  tables in `OVERVIEW.md`. Missing category falls back to a sensible
  default (e.g. empty cell or `—`).
- [ ] Every open and archived idea has a `category:` value in its
  frontmatter. The set of categories is small (~5) and agreed with the
  user before the mass edit lands.
- [ ] `OVERVIEW.md` regenerates cleanly via `python scripts/housekeep.py
  --apply` (or whatever path housekeep takes for ideas) and the new
  column shows up.

## Test Plan

**Host tests** (`make test-host` → Python tests under
`awesome-task-system/scripts/tests/`):

- Extend `test_update_idea_overview.py` to cover:
  - An idea with `category:` is rendered with the category in the table.
  - An idea without `category:` falls back to the default cell content
    without crashing.
  - Both Open and Archived tables include the new column.

No on-device tests — pure Python + Markdown change.

## Notes

- Per CLAUDE.md, edit the **package** copies under
  `awesome-task-system/scripts/` and `awesome-task-system/scripts/tests/`,
  then run `python scripts/sync_task_system.py --apply` to mirror to the
  live copies. The pre-commit hook rejects divergence.
- Before mass-applying categories, propose the candidate ~5 categories
  to the user with a one-line rationale each, and let them confirm or
  edit the set.
- Once categories are agreed, write them in batches (e.g. one batch per
  category) so the diff stays reviewable.
- Consider whether to also update the `ts-idea-new` skill template so
  newly scaffolded ideas prompt for / include `category:` from the start
  — strongly recommended, otherwise the field will rot.
