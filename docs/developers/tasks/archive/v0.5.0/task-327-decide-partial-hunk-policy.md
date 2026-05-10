---
id: TASK-327
title: Decide partial-hunk policy — unsupported, or /commit --partial escape hatch
status: closed
closed: 2026-04-30
opened: 2026-04-30
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Medium
human-in-loop: Main
epic: commit-atomicity
order: 6
prerequisites: [TASK-325]
---

## Description

Pathspec form is whole-file only — `git add -p` partial hunks are not
expressible as `git commit <files> -m`. IDEA-051 leaves this as an open
question:

- **Option A — unsupported.** Document partial-hunk committing as not
  supported in parallel sessions. Developers who want partial hunks
  must serialise (one Claude session at a time) or split the file
  manually first.
- **Option B — `/commit --partial` escape hatch.** Add a `--partial`
  mode to `/commit` that acquires a `flock` on a project lockfile,
  runs `git add -p`, commits, releases the lock. More work, but
  preserves a workflow some developers rely on.

Decide which, document it, and implement only if Option B.

## Acceptance Criteria

- [x] Decision documented in `docs/developers/COMMIT_POLICY.md` (created
      in TASK-324) with the rationale.
- [x] If Option A: `/commit` skill explicitly states partial-hunk
      committing is not supported and points users at the workaround
      (split the file or serialise sessions).
- [N/A] If Option B: `/commit --partial` implemented, with `flock` on a
      project-local lockfile, and tested end-to-end.

## Decision — Option A (unsupported)

`/commit` does not and will not support partial-hunk commits. The
`/commit` skill's "When NOT to use" section already states this
(added in TASK-323); COMMIT_POLICY.md gets a new "Partial-hunk
commits — not supported" subsection that owns the rationale.

**Rationale**:

- *Demand is hypothetical*. No closed task in this repo has needed
  `git add -p`. The Claude-driven flow generates whole-file edits;
  `/commit` is what we use, and `/commit` is whole-file by design.
  Building Option B before there's a real workflow that needs it is
  speculative work.
- *Option B's machinery is real surface area*. `flock` on a project
  lockfile means: stale-lockfile recovery, crash handling, race
  conditions when two sessions hit the lock simultaneously, the
  question of whether the lock should hold across the entire
  `git add -p` interactive session. Each of these is the kind of
  edge case pathspec was specifically designed to eliminate.
- *Workaround exists*. If a developer genuinely needs to commit some
  hunks of a file but not others, the workaround is to split the
  edit into two passes (commit half, then make the other edits and
  commit) or to serialise sessions for that one task. One-time
  annoyance, not daily friction.
- *Reversibility*. If a real workflow surfaces later that needs
  partial hunks, Option B can be added in a follow-up task — the
  hook design already supports it (the wrapper would just acquire
  the lock, do `git add -p` interactively, then `git commit -m
  "..." --` with no pathspec, since the staged hunks are now in the
  index). The decision today is "not yet", not "never".

## Test Plan

**If Option A chosen:** No automated tests required — change is
non-functional (docs).

**If Option B chosen:** Manual end-to-end verification that two
concurrent `/commit --partial` invocations correctly serialise via
`flock` and neither corrupts the other's commit. Document the
verification in the task closing notes.

## Prerequisites

- **TASK-325** — the hook enforcement context determines whether a
  partial-hunk path needs to provide its own provenance signal or can
  inherit `/commit`'s.

## Notes

- Default expectation: Option A. Partial-hunk committing is rare in
  Claude sessions, and the `flock` machinery in Option B is real
  surface area for bugs (stale lockfiles, crash recovery). Choose B
  only if there is a demonstrated workflow that needs it.
