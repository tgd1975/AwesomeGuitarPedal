---
id: TASK-323
title: Convert /commit skill to pathspec form
status: closed
closed: 2026-04-30
opened: 2026-04-30
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Medium
human-in-loop: Clarification
epic: commit-atomicity
order: 2
---

## Description

Update the `/commit` skill at `.claude/skills/commit/SKILL.md` so it
always commits via pathspec form:

```bash
git commit <file1> <file2> -m "..."
```

Never `git add <files> && git commit -m "..."`. Pathspec form is atomic
from git's perspective — git builds a temporary index containing only the
named files, runs the pre-commit hook against that, and only updates the
real index if the commit succeeds. Hook failure leaves the real index
untouched, which means foreign staged files from parallel sessions are
preserved and the next commit attempt starts from a clean state.

This is Pillar 1 of IDEA-051. Self-contained edit to the skill — no hook
changes, no policy changes. The hook still tolerates raw `git commit`
until TASK-325 lands.

## Acceptance Criteria

- [x] `/commit` skill instructs the agent to use `git commit <files> -m`
      (pathspec form) and explicitly forbids `git add` followed by
      `git commit`.
- [x] Existing CLAUDE.md "Pre-commit hook failures on unrelated changes"
      protocol still works — pathspec form does not stage anything, so
      the three-check unrelated-failure logic is unchanged.
- [x] Skill text mentions the parallel-session safety property (foreign
      staged files preserved on hook failure) so future readers
      understand why pathspec is mandatory.

## Test Plan

No automated tests required — change is non-functional (skill prose).
Manual verification: in a fresh checkout with one foreign file already
staged, run `/commit` on a different file and confirm the foreign staged
file is still staged after the commit succeeds, and still staged after a
deliberate hook failure.

## Notes

- Does not depend on TASK-322 (provenance signal can be added later when
  the hook starts checking for it).
- The pathspec form does not support `git add -p` partial hunks. Partial-
  hunk policy is decided in TASK-327; for now `/commit` simply does not
  support it.
