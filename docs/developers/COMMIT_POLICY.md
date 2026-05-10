# Commit policy — atomic /commit, mandatory path

This page is the rationale home for the project's commit rule. The rule
itself (one paragraph) lives in [CLAUDE.md](../../CLAUDE.md) under
"Commits go through /commit — always". This page owns the *why*.

If you only need the operational rule, read CLAUDE.md and stop. If a
hook fails or a bypass is needed and the reason is not obvious, this
page is the long answer.

## The rule

Every commit on this repo must flow through the `/commit` skill, which
uses git's pathspec form:

```bash
git commit -m "..." -- <file> [<file> …]
```

Never run `git add` followed by `git commit`. Never run `git commit`
without an explicit pathspec list. The pre-commit hook enforces this
mechanically — the rule is not advisory.

## The race that pathspec eliminates

Multiple Claude Code sessions often run against this repo in parallel.
Each session has its own conversation, its own scope, and its own set
of working-tree edits — but they all share one filesystem and one git
index.

Two-step commits expose that shared index:

1. Session A runs `git add fileA.cpp`. Real index now contains `fileA.cpp`.
2. Session B runs `git add fileB.md`. Real index now contains both
   `fileA.cpp` and `fileB.md`.
3. Session B runs `git commit`. The pre-commit hook fails (e.g.
   markdownlint flags `fileB.md`). Commit aborts. Real index still
   contains both files.
4. Session A returns from a long-running task and runs `git status`.
   It sees `fileB.md` staged alongside its own `fileA.cpp`. From
   Session A's perspective, the foreign file appeared out of nowhere.
5. Whatever Session A does next — commit-all, reset, "clean up the
   stray file" — clobbers Session B's in-flight work.

Pathspec form removes step 2's premise. `git commit -m "..." -- fileB.md`
builds a **temporary index** containing only `fileB.md`, runs the
pre-commit hook against that temporary index, and only updates the real
index if the commit succeeds. Hook failure leaves the real index
untouched. Session A's `fileA.cpp` stays staged where Session A left
it; Session B's `fileB.md` stays as a working-tree modification with no
index entry.

The blast radius of a hook failure is now scoped to one commit attempt
in one session. The shared index never sees foreign files.

## Why the hook is load-bearing, not advisory

A prose-only rule ("please use `/commit`") fails the moment a developer
or agent reaches for muscle-memory `git commit -am`. One session
forgetting the rule is enough to corrupt the index for everyone.

The pre-commit hook implements the rule as a check: every commit must
present a valid provenance signal proving it came from `/commit`.
Commits without the signal are rejected before they touch the
repository. Bypassing requires a deliberate, audited action
(`ASP_COMMIT_BYPASS=<reason>`), so accidental bypass is impossible and
intentional bypass is logged for review.

This is the "Pillar 3" of the design — without enforcement, pillars 1
(`/commit` uses pathspec) and 2 (CLAUDE.md mandates `/commit`) only
constrain the conscientious.

## Provenance signal — token file at `.git/asp-commit-token`

`/commit` writes a single-line token file at `.git/asp-commit-token`
immediately before invoking `git commit -m "..." -- <files>`. The
pre-commit hook reads the file, validates it, and deletes it before
allowing the commit to proceed.

**File format** (single line, space-separated):

```
<pid> <nonce> <unix-ts>
```

- `<pid>` — PID of the `/commit` invocation (the parent of the
  `git commit` process the hook runs under).
- `<nonce>` — 16+ hex chars from `/dev/urandom`, defends against
  replay if a stale token survives.
- `<unix-ts>` — seconds since epoch when the token was written.
  Tokens older than 60 s are stale and rejected.

**Hook validation** (in order):

1. If `.git/asp-commit-token` does not exist → reject (unless bypass is
   set, see below).
2. Parse the three fields. Malformed → reject.
3. If `now - <unix-ts> > 60` → reject and remove the stale file.
4. If `<pid>` is not an ancestor of the hook process → reject. (Linux:
   walk `/proc/<pid>/status` `PPid` chain. Other platforms: best-effort
   `getppid()` comparison.)
5. On success, delete the token file. Commit proceeds.

**Why a file, not an env var or a commit trailer**:

- *Env var*: inherits to child processes; a hook that itself spawns
  `git commit` (e.g. an auto-amend hook) would see the var and falsely
  authorise its own commit. Hard to scope.
- *Commit trailer*: pollutes every commit message forever, trivially
  forgeable, and audit data ends up in the wrong place (we want
  bypasses logged, not normal commits).
- *Plain marker file* (no PID/nonce/TTL): vulnerable to stale-token
  authorisation after a `/commit` crash, and to cross-session race
  conditions. The metadata is what makes this approach actually
  scoped to one commit.

## Bypass — `ASP_COMMIT_BYPASS=<reason>`

Set `ASP_COMMIT_BYPASS` to a **non-empty reason string** in the
environment of `git commit`:

```bash
ASP_COMMIT_BYPASS="rebase conflict resolution" git commit -m "..."
```

Empty or unset means no bypass — the provenance check applies. With a
non-empty reason, the hook appends one line to
`.git/asp-commit-bypass.log`:

```
<iso-8601 timestamp>\t<reason>\t<commit-sha-or-pending>\t<branch>\t<user>
```

The pre-commit hook does not yet know the commit SHA (the commit
object does not exist until after the hook returns), so it logs
`pending` and a paired post-commit hook backfills the SHA on the
matching log line.

**Legitimate bypasses**:

- **Interactive rebase** (`git rebase -i`). Git invokes `git commit`
  internally during conflict resolution; you can't route those through
  `/commit`. Set `ASP_COMMIT_BYPASS="rebase"` in the rebase shell.
- **Recovery from a broken `/commit`**. Chicken-and-egg: you can't fix
  `/commit` via `/commit` if `/commit` is broken. Set
  `ASP_COMMIT_BYPASS="fixing /commit skill itself"`.
- **Manual repo surgery** with a specific reason that doesn't fit the
  skill flow. State the reason — that's what makes the log useful.

**Misuse detection**: the audit log
(`.git/asp-commit-bypass.log`) is per-checkout (in `.git/`, not the
working tree, not versioned). Review it occasionally — frequent or
unexplained entries mean either `/commit` is missing a feature, or
someone (you) has fallen back to muscle-memory `git commit`. The log
does not block anything; it just makes silence visible.

## Interaction with the existing "unrelated hook failure" protocol

CLAUDE.md's "Pre-commit hook failures on unrelated changes" protocol
still applies, with one wording change: it now compares the failing
check against the **pathspec list**, not against the staged files —
pathspec form does not stage anything.

The three checks remain:

1. Pathspec files only (all files in this commit's pathspec are
   unrelated to the failing check).
2. Pre-existing breakage (the check is broken in the working tree or
   on `main`, not caused by this commit).
3. No silent regression (the pathspec files cannot plausibly affect
   the failing check, even indirectly).

`/commit` runs these checks for you on hook failure and presents the
standard message. Bypass via `--no-verify` after explicit user approval
is unchanged.

## Partial-hunk commits — not supported

`/commit` does not support partial-hunk commits (`git add -p`). The
pathspec form is whole-file by design — `git commit -m "..." -- <file>`
commits the file's full working-tree state, not a subset of hunks.

If you genuinely need to commit some hunks of a file but not others,
the supported workaround is to **split the edit into two passes**:
commit one half through `/commit`, make the remaining edits, commit
the other half through `/commit`. Or serialise sessions for that
specific task so the shared-index race that pathspec was built to
solve isn't in play.

This is deliberate. A `/commit --partial` mode would need `flock`
on a project lockfile (to prevent two concurrent partial-hunk commits
from corrupting each other's index state), stale-lockfile cleanup,
crash recovery, and decisions about whether the lock spans the whole
interactive `git add -p` session. Each is the kind of edge case the
EPIC-022 design specifically eliminates by going whole-file. The
demand is also hypothetical — no closed task in this repo has needed
partial-hunk committing.

Reversibility: if a real workflow surfaces later, Option B can be
added in a follow-up — the hook already supports it (the wrapper
would acquire the lock, run `git add -p` interactively, then
`git commit -m "..." --` with no pathspec since the hunks are
already staged). For now, the decision is "not yet", not "never".

## What this does not solve

- **Rebases and cherry-picks**. Git invokes `git commit` internally
  during these flows. The bypass mechanism is the answer; do not try
  to route rebase commits through `/commit`.
- **Cross-platform PID-ancestry**. The `/proc` walk is Linux-specific.
  Windows and macOS fall back to `getppid()`-only validation, which
  is weaker but still blocks the common env-var-style leak.

## Composing with the CI merge gate

`/commit` and the pre-commit hook are the **local** layer of enforcement.
They block bad commits on the developer's machine. The **server-side**
layer is the CI merge gate on `main` — branch protection requires every
listed check (markdown lint, host unit tests, clang-format, profile
validation, CodeQL, etc.) to be green before a PR can be merged.

A successful `/commit` therefore does **not** imply mergeability:

- The hook checks staged content; the gate checks the whole PR head
  after CI runs every workflow on a real GitHub runner. The runner
  catches things a fast local hook cannot — flaky integration tests,
  cross-platform formatting drift, generated-file staleness.
- Bypassing the local hook with `--no-verify` (per the
  three-check protocol in CLAUDE.md) **does not** bypass the gate. The
  PR still has to clear CI before merging.

The required-checks list and the recipe for adding a new check live in
[DEVELOPMENT_SETUP.md → CI merge gate](DEVELOPMENT_SETUP.md#ci-merge-gate-branch-protection-on-main).
That document is the source of truth; this section is a pointer.

## Pointers

- [CLAUDE.md — "Commits go through /commit — always"](../../CLAUDE.md)
  — the operational rule.
- [.claude/skills/commit/SKILL.md](../../.claude/skills/commit/SKILL.md)
  — the `/commit` skill itself, including the hook-failure protocol.
- TASK-322 (closed) — ADR-style decision record for the provenance
  signal and bypass mechanism.
- IDEA-051 (archived) — the original write-up that seeded EPIC-022.
