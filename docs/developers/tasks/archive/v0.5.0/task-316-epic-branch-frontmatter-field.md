---
id: TASK-316
title: Add optional `branch:` field to epic frontmatter
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
epic: epic-suggested-branch
order: 1
---

## Description

Introduce an optional `branch:` field on epic frontmatter that names the
suggested git branch for tasks belonging to that epic. The field is the
data foundation for the soft-enforcement nag in TASK-318. It must be
opt-in: existing epics without the field continue to work unchanged.

The value is a single git branch name (e.g. `feature/long-double-press`).
Multi-branch arrays and `branch_group:` are out of scope for this slice.

## Acceptance Criteria

- [ ] `ts-epic-new` skill documents the new optional `branch:` field in
      its template (frontmatter schema only — auto-fill behaviour is
      TASK-317).
- [ ] `awesome-task-system/scripts/housekeep.py` parses epic files with
      a `branch:` field without warning or rejecting them, and preserves
      the field across file moves.
- [ ] Adding `branch: feature/foo` to one epic and running
      `python scripts/housekeep.py --apply` is a no-op for OVERVIEW /
      EPICS / KANBAN content (the field is data, not display).
- [ ] EPIC-021's own frontmatter already carries
      `branch: feature/config-extensions`, so this task verifies that
      example round-trips through housekeep.

## Test Plan

No automated tests required — change is non-functional (frontmatter
schema and skill docs only). Verification is manual:

- Edit `epic-021-epic-suggested-branch.md` to add/remove the `branch:`
  field; run `python scripts/housekeep.py --apply`; confirm no diff
  beyond the edit and no warnings.

## Documentation

- `awesome-task-system/skills/ts-epic-new/SKILL.md` — add `branch:` to
  the epic file template with a one-line description.
- `awesome-task-system/TASK_SYSTEM.md` (if it documents epic
  frontmatter) — add the field there too.
- Sync to live copies via `python scripts/sync_task_system.py --apply`.

## Notes

- Source-of-truth rule: edit `awesome-task-system/`, then sync. The
  pre-commit hook enforces parity.
- Frontmatter parser is generic — most likely no parser change needed.
  Confirm by reading `parse_frontmatter` in `housekeep.py`.
