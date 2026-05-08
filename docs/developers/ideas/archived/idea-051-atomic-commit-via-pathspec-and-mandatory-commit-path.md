---
id: IDEA-051
title: Atomic /commit via pathspec + mandatory hook-enforced /commit path
description: Eliminate parallel-session staging races by making /commit use pathspec form (atomic, no lock) and requiring all commits to flow through /commit, enforced by the pre-commit hook.
category: 🛠️ tooling
---

## Archive Reason

2026-04-30 — Converted to EPIC-022 (Atomic /commit and mandatory commit path).

Multiple Claude Code sessions often run against this repo in parallel.
Today, the staging area is shared mutable state across sessions:
session A's `git add` and session B's `git add` interleave, and a
last-moment `git add` from one task can spoil the other's commit. The
existing CLAUDE.md "commit only your own work" rule mitigates the
sweep-up case, but does not address the *race window* between staging
and committing.

## The race we are eliminating

1. Task A runs `git add file1`.
2. Before A commits, task B runs `git add file2`.
3. A's `git commit -m "..."` now picks up both files.

A pre-commit hook failure on either side makes it worse — the failing
files stay staged, polluting the next commit attempt.

## The design

Three load-bearing pillars. All three must hold for the design to work.

### 1. Pathspec form inside /commit

`/commit` always invokes:

```bash
git commit file1 file2 -m "..."
```

Never `git add` + `git commit`. Pathspec form is **atomic from git's
perspective**: git builds a temporary index containing only the named
files, runs the pre-commit hook against it, and only updates the real
index if the commit succeeds. Hook failure rolls back cleanly — the
real index is never modified. Foreign staged files (from other
sessions) are preserved untouched.

This single change eliminates the need for any flock or critical
section.

### 2. Mandatory /commit path

CLAUDE.md states unambiguously: **never run `git add` + `git commit`
directly. Always use `/commit`.** Developer docs explain the *why*
(parallel-session safety, atomic commits, no rogue staging).

### 3. Pre-commit hook enforcement (paramount, not optional)

Prose alone is not load-bearing. The pre-commit hook **rejects any
commit that did not flow through `/commit`** — e.g. by checking for a
one-shot env var or token that `/commit` sets and clears. The hook
names the rule and points at `/commit` in its rejection message.

Without the hook, rules 1 and 2 are polite suggestions that the next
session forgets. With the hook, bypass becomes impossible without
deliberate override.

## Why this is one idea, not many

By the verb-object test: all three pillars share the same
verb-object pair — *make /commit the safe, atomic commit path*. They
form a single multi-step plan, not three separate ideas.

## What this does not eliminate

- **`git commit -a`** — bypasses pathspec scoping. Forbidden by
  policy; the enforcement hook should also reject `-a` regardless of
  source.
- **`git add -p` / partial-hunk commits** — pathspec is whole-file
  only. Rare in Claude sessions; if needed, a documented escape hatch
  is required (acquire-lock-then-stage-then-commit, or just don't do
  it).
- **Two `/commit` calls hitting `.git/index.lock` simultaneously** —
  resolved by git's own internal lock-file mechanism (one waits and
  retries). No work needed on our side.

## Open questions

- **Hook bypass mechanism for legitimate cases.** Manual repo
  surgery, recovery scenarios, or rebases may need to bypass. An
  env var (`ASP_COMMIT_BYPASS=1`)? A flag? Who is allowed to use it,
  and how is misuse caught?
- **What signal does `/commit` use to prove "this commit came from
  me"?** Env var (simple, but inheritable across processes), commit
  trailer (visible in history but ugly), file marker in `.git/`
  (cleanly scoped, but needs cleanup). Probably env var with a
  short-lived token.
- **Interaction with `/release`, `/release-branch`, and other skills
  that commit.** Do they go through `/commit`, or do they get their
  own bypass route? If bypass — who else? Risk of "every skill has
  bypass, so the rule is meaningless."
- **Partial-hunk escape hatch.** Should `/commit` grow a `--partial`
  mode that acquires a flock, runs `git add -p`, commits, releases?
  Or is partial-hunk committing simply not supported in parallel
  sessions?
- **Documentation home for the *why*.** New page under `docs/developers/`?
  Section in `DEVELOPMENT_SETUP.md`? CLAUDE.md owns the operational
  rule, but the rationale needs a home where developers can find it.

## Related

- Surfaced during a session debugging the parallel-Claude commit race
  — see chat that produced IDEA-050. The two ideas share an origin
  but address different problems (IDEA-050: version auto-memory;
  IDEA-051: atomic commits).
- Existing CLAUDE.md sections this builds on: "Parallel sessions —
  commit only your own work" and "Pre-commit hook failures on
  unrelated changes". This idea makes both rules genuinely
  enforceable instead of advisory.
- The `/commit` skill at [.claude/skills/commit/SKILL.md](../../../../.claude/skills/commit/SKILL.md)
  is the implementation surface — project-local, not part of
  awesome-task-system, no two-sided sync.
