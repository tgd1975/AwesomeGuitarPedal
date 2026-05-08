---
id: TASK-329
title: Drop auto-git-add for untracked files from commit-pathspec.sh
status: closed
closed: 2026-04-30
opened: 2026-04-30
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: commit-atomicity
order: 7
---

## Description

`scripts/commit-pathspec.sh` currently includes a "convenience" step
that runs `git add -- "$f"` for every pathspec entry that
`git ls-files --error-unmatch` reports as untracked. This was added in
TASK-325 to handle the case where a user creates a new file via the
`Write` tool and immediately wants to `/commit` it — pathspec form
otherwise rejects untracked paths with `error: pathspec '<file>' did
not match any file(s) known to git`.

The convenience masks parallel-session races. **Discovered while
executing EPIC-022**: two parallel sessions both closed TASK-319. The
second session's `commit-pathspec.sh` ran with a stale view of the
index — a foreign-session commit had just landed and added the
closed-task file to the tree, but the second session had not refreshed.
`git ls-files --error-unmatch` reported the path as untracked from the
second session's perspective, so the wrapper helpfully `git add`ed it.
Both sessions' commits then "successfully" added the same path, leaving
TASK-319's file present in `active/`, `open/`, *and* `closed/` across
the two commits' combined effect on HEAD.

The original "did not match" error pathspec produces would have been a
**better** signal: it forces the agent to refresh and discover the
foreign change, rather than silently overwriting it.

Drop the auto-`git add` from the wrapper. Skill prose in
`.claude/skills/commit/SKILL.md` already lists "untracked new files
need `git add` first" as a step the agent does before invoking the
wrapper — this just enforces it by removing the silent fallback.

## Acceptance Criteria

- [x] `scripts/commit-pathspec.sh` no longer runs `git add` for any
      reason. The pathspec is passed straight to `git commit -m "..."
      -- <files>`. Untracked entries fail with exit 2 + clear stderr.
- [x] `.claude/skills/commit/SKILL.md` updated: the "Add untracked
      files" step is now mandatory prose (Step 2) that the agent must
      execute before the wrapper, not optional fallback behaviour.
- [x] `scripts/tests/test_commit_pathspec.py` added — verifies the
      wrapper rejects untracked-pathspec invocations with exit 2 and
      a clear error message, and confirms the index stays untouched
      on the rejection path.
- [x] Smoke test: invoked the wrapper on a newly-created (untracked)
      file in this repo (not just the test fixture) — wrapper exited
      2 with the expected stderr and the file remained untracked
      (`?? scripts/asp-smoke-untracked.tmp`).

## Test Plan

**Host tests** (`make test-host`): no — bash wrapper change.

**Script-level tests** (`python -m unittest scripts.tests.test_commit_provenance`):

- New test cases:
  - Wrapper exits non-zero when given an untracked pathspec entry.
  - Wrapper succeeds with tracked-modified pathspec entries.
  - Wrapper does not invoke `git add` on its own (verify by mocking
    or by running in a throwaway git repo and asserting the index is
    unchanged after the wrapper runs).

## Prerequisites

This task amends decisions made in TASK-323 and TASK-325, but does not
formally depend on them — they are already closed.

## Notes

- Reversibility: this is a small change to the wrapper. If the
  parallel-session benefit doesn't materialise (e.g. the failure mode
  was rare and the friction is daily), the auto-add can be restored
  in a follow-up task with the same shape.
- The "fail fast on untracked" behaviour is consistent with the rest
  of the EPIC-022 design philosophy: surface foreign-session activity
  rather than paper over it. See COMMIT_POLICY.md.
- Trigger to re-evaluate: a new memory should also be saved noting
  that the wrapper's auto-`git add` was an attractive nuisance that
  hid races. This prevents the same convenience from being re-added
  in a future "let's smooth out friction" task.
