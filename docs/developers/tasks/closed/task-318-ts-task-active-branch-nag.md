---
id: TASK-318
title: `/ts-task-active` nags when current branch ≠ epic's `branch:`
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: Medium (2-8h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: epic-suggested-branch
order: 3
prerequisites: [TASK-316]
---

## Description

Add a soft branch-mismatch check to `/ts-task-active`. When activating a
task whose epic carries a `branch:` field, compare the current git
branch to that value. If they match, do nothing. If they differ, prompt
the user with two options:

- **[s]witch** — run `git checkout <epic-branch>`. If the branch does
  not exist locally, ask whether to create it (`git checkout -b
  <epic-branch>`). After switching, continue activating the task.
- **[c]ontinue** — keep working on the current branch and **rewrite the
  epic's `branch:` field to the current branch**. This is the explicit
  "I am intentionally co-locating this epic on this branch" signal. The
  rewrite is staged so it rides along with the next commit.

The check runs **after** the existing `/check-branch` (which warns about
`main`), not as a replacement. The two warnings compose: on `main` with
a mismatched epic, the user sees both, in order.

If the task has no `epic:` field, or the epic has no `branch:` field,
the check is silently skipped.

## Acceptance Criteria

- [ ] `awesome-task-system/skills/ts-task-active/SKILL.md` documents the
      new step: after `/check-branch`, look up the task's epic, read the
      epic's `branch:`, compare to the current branch, and prompt only
      on mismatch.
- [ ] On mismatch, `[s]witch` runs `git checkout <epic-branch>` (and
      offers `-b` if the branch is missing locally) before completing
      the activation.
- [ ] On mismatch, `[c]ontinue` rewrites the epic file's `branch:` line
      to the current branch and leaves the change unstaged (housekeep
      will pick it up; the user commits it with the activation).
- [ ] When the task's epic has no `branch:` field, or the task has no
      `epic:` field, the skill behaves exactly as today (no prompt, no
      diff to the epic file).
- [ ] Manual verification: activate a task on a non-matching branch,
      pick `[c]`, observe the epic's `branch:` updated to the current
      branch and the regenerated OVERVIEW/EPICS/KANBAN.

## Test Plan

No automated tests required — skill instruction change for an
agent-driven workflow. Manual verification covers the three cases:

1. Task with epic's `branch:` matching current branch → no prompt.
2. Task with epic's `branch:` differing from current → prompt; pick
   `[s]` and confirm checkout; pick `[c]` and confirm epic rewrite.
3. Task with no epic, or epic with no `branch:` → no prompt, no diff.

## Prerequisites

- **TASK-316** — defines the `branch:` field that this skill reads.

## Documentation

- `awesome-task-system/skills/ts-task-active/SKILL.md` — insert the new
  step after step 0 (`/check-branch`) and before step 1.
- Sync to live copy.

## Notes

- The skill is markdown instructions for the agent, not a script. The
  "rewrite epic frontmatter" step is the agent using `Edit` on the
  epic file, then letting housekeep pick up the change — no new
  Python.
- Compose-with-`/check-branch` rule: do **not** suppress the
  `/check-branch` warning when the epic's `branch:` is also `main`
  (which would be unusual but possible). The two checks answer
  different questions.
- This task does not introduce a pre-commit hook. The pre-commit
  variant from IDEA-030 stays out of scope until we see whether the
  activation-time nag alone is enough.
