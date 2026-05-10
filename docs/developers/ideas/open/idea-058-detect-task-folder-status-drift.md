---
id: IDEA-058
title: Detect and repair inconsistent task-folder/status states left by parallel sessions
description: When a parallel session lands a task file in one folder but with a status that says it should live elsewhere, the next agent silently papers over it. Decide where to detect this and what to do about it.
category: 🛠️ tooling
---

## The bug we hit

While executing TASK-369 the agent invoked `/ts-task-active TASK-369`. The
file was already in `docs/developers/tasks/active/` (committed there by a
parallel session) but its frontmatter still said `status: open`. The skill's
mode-detection branches on **folder**, not on the (folder, status) pair, so
it never noticed the mismatch. The agent then edited `status: open → active`
manually and ran housekeep, which planned `MOVE open/ → active/` because it
was working from a stale view of the index. The git index ended up with a
phantom `RD active/ → open/` rename plus an untracked working-copy in
`active/`. Status:

```text
RD docs/developers/tasks/active/task-369-…md -> docs/developers/tasks/open/task-369-…md
?? docs/developers/tasks/active/task-369-…md
```

Cleaning that up cost a `git reset HEAD` plus a re-edit and a re-housekeep.
The user never saw the inconsistency; the agent figured it out by reading
git status three times and re-reading HEAD.

## Why this is a class, not an incident

Parallel sessions race on:

- **Task-folder moves.** Session A runs `/ts-task-active TASK-N` and edits
  the status; session B simultaneously runs `git mv` via housekeep. One of
  them lands the rename without the matching status edit (or vice-versa)
  and the next agent inherits an inconsistent file.
- **Frontmatter edits.** Session A writes `status: closed` and runs
  housekeep; session B has cached an older copy in its working tree and
  commits the stale status alongside other unrelated work.
- **Idea-NNN collisions.** Three `idea-057-*.md` files now sit side by
  side in `docs/developers/ideas/open/` because three sessions all picked
  the same next-free ID. (Out of scope here — that is the
  next-free-ID-allocation race, not the folder/status drift race — but
  the same parallel-session model produces both.)

The folder/status drift specifically lands as: file in folder X with
`status: Y` where Y disagrees with X. Today nothing detects this. The
`/ts-task-*` skills assume the folder is correct and edit the status to
match; `housekeep.py` assumes the status is correct and moves the folder
to match. Each side trusts the other, and a parallel-session race breaks
the assumption.

## Open questions

The right design depends on answers to these:

1. **Where to detect.** SessionStart hook (cheap, runs once per
   conversation, would have caught this), inside each `/ts-task-*` skill
   (per-invocation, scoped, but redundant), inside `housekeep.py --apply`
   (already runs on every task-system change, would catch it as a
   side-effect), or as a separate `housekeep.py --check-drift` mode the
   pre-commit hook can call. Each has trade-offs.
2. **What "inconsistent" means.** The clean rule is *folder ↔ status must
   match exactly* (`active/` ⇔ `status: active`, etc.). But a paused task
   that finished its blockers should be in `paused/` with stale-but-still-
   active prerequisites — is that drift or expected? The legacy
   "active-with-closed-prerequisites" case from the pre-paused-folder
   model adds another wrinkle.
3. **Repair vs. report.** Auto-fix on detection (silent), report and
   stop (loud, blocks work), or report and offer a one-line repair
   command (compromise). The "obey lint rules / never silently soften"
   posture from the project's feedback memory leans loud.
4. **Whose work is the repair?** If session B detects drift caused by
   session A and repairs it, the repair lands in session B's commit —
   but session A's intent might have been incomplete (e.g. they were
   mid-rebase). The "commit only your own work" rule wants session B to
   leave it alone; the "don't paper over inconsistencies" rule wants
   session B to fix it. Pick.
5. **Does the same logic catch the next-free-ID collision class?** The
   three `idea-057-*` files imply the next-free-ID allocator (currently
   "scan OVERVIEW.md, add 1") is racy too. A unified "task-system
   consistency check" might subsume both, or they might want separate
   solutions.

## Rough sketches (not decisions)

- **Cheapest:** add a `housekeep.py --check-drift` mode that walks every
  task file and asserts `folder == status`. Pre-commit hook calls it.
  Drift becomes a hard failure on commit. No SessionStart, no skill
  changes — but doesn't catch drift mid-session, only at commit time.
- **Most surgical:** make `/ts-task-active` (and siblings) read both
  the folder and the status before branching. If they disagree, surface
  the mismatch and ask the user which one is authoritative. Adds 5–10
  lines per skill but requires four-way coordination.
- **Most invasive:** SessionStart hook scans the whole task tree and
  reports drift to the agent at the start of every conversation. Catches
  everything but is the heaviest option and adds startup latency.

The right answer is probably "cheapest" plus a tiny addition to
`/ts-task-*` so they don't *create* drift (they read the file, see the
folder/status pair, and refuse to edit the status field if the folder
already disagrees — pointing the agent at `/ts-task-active` or `/ts-task-done`
instead).

## Success criterion

A parallel-session race that produces drift is detected at the latest
on the next pre-commit, and the next `/ts-task-*` invocation in any
session refuses to compound the drift. Neither requires the user to
notice anything by hand.
