---
id: EPIC-021
name: epic-suggested-branch
title: Epic-suggested branch with soft enforcement
status: open
opened: 2026-04-29
closed:
assigned:
branch: feature/config-extensions
---

Each epic declares a suggested branch. When a task is activated, the
task-system nudges the user if the current git branch does not match
its epic's suggested branch — soft enforcement, not a hard gate. The
goal is smaller, thematically focused releases instead of long-lived
branches that pile up unrelated epics.

Seeded by IDEA-030 (Epic-Suggested Branch with Soft Enforcement).

First slice (this epic):

- Add an optional `branch:` field to epic frontmatter.
- `/ts-epic-new` auto-fills `feature/<epic-slug>` (sanitized, short).
- `/ts-task-active` nags when the current branch differs from the
  task's epic's `branch:`. The user can [s]witch (`git checkout`) or
  [c]ontinue — `[c]ontinue` rewrites the epic's `branch:` to the
  current branch.

Out of scope for now: pre-commit warning, multi-branch arrays /
`branch_group:`, task-level overrides, backfilling existing epics
(field is opt-in).

## Tasks

Tasks belonging to this epic reference it via
`epic: epic-suggested-branch` in their frontmatter.
