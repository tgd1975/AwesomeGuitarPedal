---
id: EPIC-022
name: commit-atomicity
title: Atomic /commit and mandatory commit path
status: open
opened: 2026-04-30
closed:
assigned:
branch: feature/config-extensions
---

Eliminate the parallel-session staging race by making `/commit` use git's
pathspec form (atomic from git's perspective, no shared-index risk) and
requiring all commits to flow through `/commit`, enforced by the pre-commit
hook. Foreign staged files from other concurrent sessions become
irrelevant — pathspec commits build a temporary index containing only the
named files and never touch the real index on hook failure.

Seeded by IDEA-051 (Atomic /commit via pathspec and mandatory hook-enforced
/commit path).

The epic's three pillars must all hold for the design to work:

1. `/commit` always invokes `git commit <files> -m "..."` (pathspec form).
2. CLAUDE.md mandates `/commit` as the only commit path.
3. The pre-commit hook rejects any commit that did not flow through
   `/commit`, making bypass deliberate rather than accidental.

Several open questions in IDEA-051 (provenance signal, bypass mechanism,
`/release*` interaction, partial-hunk policy, docs home) are resolved in
the early tasks of this epic before the load-bearing hook lands.

## Tasks

Tasks are listed automatically in the Task Epics section of
`docs/developers/tasks/OVERVIEW.md` and in `EPICS.md` / `KANBAN.md`.
