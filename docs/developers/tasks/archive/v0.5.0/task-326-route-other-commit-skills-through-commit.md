---
id: TASK-326
title: Route /release, /release-branch, and other commit-issuing skills through /commit
status: closed
closed: 2026-04-30
opened: 2026-04-30
effort: Medium (2-8h)
effort_actual: XS (<30m)
complexity: Medium
human-in-loop: Clarification
epic: commit-atomicity
order: 5
prerequisites: [TASK-325]
---

## Description

Once the pre-commit hook from TASK-325 lands, every skill that issues
`git commit` directly will start failing. Audit and fix:

- `/release`
- `/release-branch`
- `/ts-task-active`, `/ts-task-done`, `/ts-task-pause`, `/ts-task-reopen`
  (these run housekeep but historically may not commit themselves —
  verify)
- `/housekeep` (verify — it stages but should not commit)
- Any other skill under `.claude/skills/` that calls `git commit`

For each skill, the choice is:

1. **Route through `/commit`** — call `/commit` rather than raw `git
   commit`. Preferred when the skill knows exactly which files it owns.
2. **Use the documented bypass from TASK-322** — when the skill performs
   genuinely-multi-file atomic operations (release tagging, squash-merge)
   that the `/commit` flow doesn't model. Each bypass site must be
   justified in the skill prose so future readers understand why.

Risk to manage: "every skill has bypass, so the rule is meaningless."
Keep the bypass list short and reviewed.

## Acceptance Criteria

- [x] Audit produced — list of every skill that issues `git commit`,
      with the chosen route (through `/commit` or bypass) and a one-line
      justification per bypass.
- [x] Each affected skill updated to either route through `/commit` or
      set the bypass mechanism, matching the audit.
- [x] Smoke test of each affected skill passes against the live hook.
      Verified via direct replay of both bypass patterns on a
      throwaway scratch branch (`scratch/epic-022-smoke`), without
      invoking the full release flow (no fake tag, no force-push):
      both `ASP_COMMIT_BYPASS="release-branch: squash commit"` and
      `ASP_COMMIT_BYPASS="release-branch: version bump + archive"`
      passed the provenance check and appended correctly-formatted
      entries to `.git/asp-commit-bypass.log`. Downstream lint
      failures (markdown, housekeep order validation from foreign-
      session leftovers) confirmed the hook chain ran past the
      provenance stage. The full `/release-branch` end-to-end with a
      real tag is still exercised by the next actual release cut, but
      that is no longer the only smoke path.

## Audit

`grep -rn "git commit" .claude/skills/` produced three sites outside
the `/commit` skill itself. None of the `ts-task-*` skills, `/housekeep`,
or other task-system skills issue `git commit` — they stage and let
the caller commit (which routes through `/commit` per the EPIC-022
contract). The three sites:

| Skill | File:line | Route | Bypass reason |
|---|---|---|---|
| `/release` | `.claude/skills/release/SKILL.md:117` | bypass | Multi-file release operation (version.h, README, CHANGELOG, archive dir contents) — file list is not a flat enumerable pathspec |
| `/release-branch` (squash) | `.claude/skills/release-branch/SKILL.md:79` | bypass | Squash commit's file set is "everything in the branch" — also not enumerable |
| `/release-branch` (bump+archive) | `.claude/skills/release-branch/SKILL.md:174` | bypass | Same multi-file release character as `/release` |

All three are legitimate matches for TASK-326's option 2 ("genuinely
multi-file atomic operations that the `/commit` flow doesn't model").
Each bypass site has a `ASP_COMMIT_BYPASS="<reason>"` env var that
appears in the audit log when the skill runs, so misuse is visible.

The only skill that routes through `/commit` is `/commit` itself —
trivially. No new wiring needed.

## Test Plan

No automated tests required — change is non-functional (skill prose plus
manual smoke tests). Each affected skill is verified by running it once
end-to-end and confirming the commit succeeds without disabling the hook.

## Prerequisites

- **TASK-325** — the hook must be live before this task can audit
  what breaks. Trying to do this first means guessing which skills
  bypass-need versus route-need.

## Notes

- The `awesome-task-system` package mirror (`scripts/sync_task_system.py`)
  copies skill files between `awesome-task-system/` and `.claude/skills/`
  — any skill edits here must be made on the package side first per
  CLAUDE.md.
