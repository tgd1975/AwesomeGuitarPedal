---
id: TASK-322
title: Decide commit-provenance signal and bypass mechanism for /commit
status: closed
closed: 2026-04-30
opened: 2026-04-30
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Senior
human-in-loop: Main
epic: commit-atomicity
order: 1
---

## Description

Resolve two open questions from IDEA-051 that block the rest of the epic:

1. **Provenance signal** — what does `/commit` set so the pre-commit hook
   can prove "this commit came from /commit, not raw git"? Candidates:
   - Env var with short-lived token (simple, but env vars inherit across
     processes — risk of leaking provenance to unrelated commits).
   - Commit trailer (visible in history, but pollutes commit messages).
   - File marker in `.git/` (cleanly scoped, but needs cleanup on crash).
2. **Bypass mechanism** — how does a developer legitimately bypass the
   `/commit`-only rule for manual repo surgery, recovery, rebases?
   `ASP_COMMIT_BYPASS=1` env var is the strawman from IDEA-051 — confirm
   or replace, and define who is allowed to use it.

Output is a short decision document captured in the task body (an ADR-style
note) that TASK-323 / TASK-325 can reference. No code changes here.

## Acceptance Criteria

- [x] Provenance signal chosen, with one-paragraph rationale covering
      inheritance/leak risk, crash-cleanup behaviour, and visibility.
- [x] Bypass mechanism named, with the exact env var or flag, the
      intended use cases, and how misuse will be detected (e.g. logged
      to a file the user reviews periodically).
- [x] Decisions written into this task body under a `## Decision` section
      so subsequent tasks can cite it.

## Test Plan

No automated tests required — change is non-functional (decision document only).

## Notes

- Sequencing: this is the first task in EPIC-022. TASK-323 (pathspec
  conversion) does not strictly depend on it, but TASK-324 (CLAUDE.md +
  COMMIT_POLICY.md) and TASK-325 (hook enforcement) both do.
- Risk: choosing an env var that leaks across child processes (e.g. a
  hook that itself spawns `git commit`) creates false positives. The
  decision should explicitly address this.

## Decision

ADR-style note. Authoritative for TASK-323 / TASK-324 / TASK-325 / TASK-326.

### D1. Provenance signal — token file at `.git/asp-commit-token`

`/commit` writes a single-line token file at `.git/asp-commit-token`
immediately before invoking `git commit <files> -m "..."`. The pre-commit
hook reads the file, validates it, and deletes it before allowing the
commit to proceed. Absence (or invalidity) of the token means the commit
did not come from `/commit`.

**File format** (single line, space-separated):

```
<pid> <nonce> <unix-ts>
```

- `<pid>` — PID of the `/commit` invocation (the parent process that will
  run `git commit`). The hook compares against `getppid()` (the PID of
  the `git commit` process that is the hook's parent).
- `<nonce>` — 16+ hex chars from `/dev/urandom` (or platform equivalent).
  Defends against replay if a stale token survives.
- `<unix-ts>` — seconds-since-epoch when `/commit` wrote the file. Tokens
  older than 60 s are treated as stale and rejected.

**Hook validation**:

1. If `.git/asp-commit-token` does not exist → reject (unless bypass is
   set; see D2).
2. Parse the three fields. Malformed → reject.
3. If `now - <unix-ts> > 60` → reject and remove the stale file.
4. If `<pid>` is not an ancestor of the hook process (walk
   `/proc/<pid>/status` `PPid` chain on Linux; fall back to comparing
   `<pid>` against `getppid()` and its parent on other platforms) →
   reject. This blocks a token written by a different shell from
   authorising an unrelated `git commit`.
5. On success, delete the token file and exit 0.

**Rationale** (covers the three required dimensions):

- *Inheritance / leak risk*: a file in `.git/` does not inherit through
  `fork`/`exec` the way an env var does. A nested `git commit` spawned
  by a hook script will not see a token unless `/commit` wrote one for
  exactly that invocation. The PID-ancestry check prevents the
  remaining edge case where two `/commit` calls race: each writes a
  token, but only the one whose parent PID matches the live hook
  succeeds.
- *Crash cleanup*: if `/commit` crashes after writing the token but
  before invoking `git commit`, the token is stale. The 60-second TTL
  bounds the damage — a manual `git commit` more than a minute later
  is correctly rejected as un-authorised. For belt-and-braces, the
  hook deletes any stale token it observes.
- *Visibility*: the token never enters the commit message or the
  repository. It exists for the duration of one commit and disappears.
  Auditability is provided separately by the bypass log (D2), which is
  the only case where opting out matters.

**Why not the alternatives**:

- *Env var (`ASP_COMMIT_VIA_SKILL=1`)*: env vars inherit to child
  processes. A pre-commit hook that itself runs `git commit` (e.g. a
  housekeeping hook that auto-amends) would see the var and falsely
  authorise its own commit. Hard to reason about, hard to scope.
- *Commit trailer (`Commit-Via: skill`)*: pollutes every commit message
  forever; trivially forgeable by a manual `git commit -m "... \n
  Commit-Via: skill"`; provides no real security guarantee. The audit
  trail is also wrong-place — bypass attempts, not normal commits, are
  what we want logged.
- *Plain marker file* (no PID/nonce/TTL): vulnerable to stale-token
  authorisation after a crash and to cross-session race conditions.
  The metadata is what makes this approach actually scoped.

### D2. Bypass mechanism — `ASP_COMMIT_BYPASS=<reason>` with mandatory reason and audit log

A developer (or recovery tool) bypasses the `/commit`-only rule by
setting `ASP_COMMIT_BYPASS` to a **non-empty reason string** before
running `git commit`. The hook treats unset or empty as "no bypass" and
falls through to the provenance check (D1).

**Behaviour**:

- `ASP_COMMIT_BYPASS=` (empty) or unset → no bypass; provenance check
  applies as in D1.
- `ASP_COMMIT_BYPASS=<non-empty>` → bypass granted; the hook appends
  one line to `.git/asp-commit-bypass.log`:

  ```
  <iso-8601 timestamp>\t<reason>\t<commit-sha-or-pending>\t<branch>\t<user>
  ```

  The hook does not yet know the commit SHA (pre-commit runs before
  the commit object exists) — log `pending` and let `post-commit` (a
  one-liner installed alongside) backfill the SHA on the matching log
  line.

**Intended use cases** (legitimate bypasses):

- Interactive rebases (`git rebase -i`) — git invokes `git commit`
  internally during conflict resolution; the developer cannot route
  that through `/commit`.
- Recovery from a broken `/commit` skill itself (chicken-and-egg: you
  cannot fix `/commit` via `/commit` if `/commit` is broken).
- Manual repo surgery where the developer has a specific reason that
  doesn't fit the skill's flow.

**Misuse detection**:

- The audit log (`.git/asp-commit-bypass.log`) is the primary signal.
  It is per-checkout (in `.git/`, not the working tree) and not
  versioned, so it accumulates across normal use without polluting the
  repo.
- A weekly review (manual, or a future `/triage` skill) flags
  unexplained or repeated bypass entries. High-frequency bypasses are
  a smell — they mean either `/commit` is missing a feature, or
  someone has fallen back to muscle-memory `git commit`.
- Empty / unset is the default, so the *absence* of a log entry is
  also a positive signal (every commit on this branch flowed through
  `/commit`).

**Who is allowed to use it**: anyone — there is no enforcement gate
beyond "you must supply a reason". The point of the audit log is to
make bypass *visible*, not to make it *forbidden*. Silent bypass is
what the epic eliminates.

**Why not the alternatives**:

- *Plain `ASP_COMMIT_BYPASS=1`*: no reason captured; the audit log
  would be useless (every entry says "1"). Forcing a reason string
  costs nothing and makes the log self-documenting.
- *Reuse `--no-verify`*: overloaded with legitimate non-bypass uses
  (skipping a known-broken hook on a doc-only commit, per CLAUDE.md's
  three-check protocol). The hook has no way to distinguish "I am
  bypassing /commit" from "I am bypassing this specific check", so
  conflating them produces false log entries.

### D3. Out of scope (deferred)

- Cross-platform PID-ancestry check on Windows / macOS — D1's Linux
  `/proc` walk is the primary path. Windows and macOS fall back to
  the `getppid()`-only check, which is weaker but still blocks the
  common case (env-var-style leak through a single fork). TASK-325
  may add platform shims if test coverage demands it.
- Automatic bypass-log review/alerting. Manual review is good enough
  for one-developer scale; a future skill can read the log if usage
  grows.
- Provenance for `/release*` skills — TASK-326 routes those through
  `/commit`, so they inherit D1 automatically. No separate provenance
  channel needed.
