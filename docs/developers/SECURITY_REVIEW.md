# Security review — incoming-pull supply-chain check

Every time you `git pull`, `git merge`, or `git pull --rebase`, an analyzer
scans the incoming changes for backdoors, exfiltration, privilege escalation,
or sneaky permission widening. The motivation is supply-chain trust: scripts
and skills in this repo can be auto-invoked by Claude Code via the allow-list
in [.claude/settings.json](../../.claude/settings.json), so a malicious change
to a "trusted" script would run with no further prompt.

The report is written to **`.claude/security-review-latest.md`** at the repo
root. It is gitignored — local feedback, not committed history.

## How it is wired

| Piece | Location |
|---|---|
| Pre-merge block | `pre-merge-commit` git hook → [scripts/git-hooks/pre-merge-commit](../../scripts/git-hooks/pre-merge-commit) |
| Pre-rebase block | `pre-rebase` git hook → [scripts/git-hooks/pre-rebase](../../scripts/git-hooks/pre-rebase) |
| Fast-forward warn | `post-merge` git hook → [scripts/git-hooks/post-merge](../../scripts/git-hooks/post-merge) |
| Analyzer | [scripts/security_review_changes.py](../../scripts/security_review_changes.py) |
| Installer | [scripts/install_git_hooks.sh](../../scripts/install_git_hooks.sh) (one-time, idempotent) |
| Report | `.claude/security-review-latest.md` (gitignored) |
| Headless review model | `claude -p` (binary auto-located under `~/.vscode/extensions/anthropic.claude-code-*-linux-x64/` if not on PATH; override with `ASP_CLAUDE_BIN`) |

## One-time setup per clone

```bash
bash scripts/install_git_hooks.sh
```

Symlinks `scripts/git-hooks/*` into `.git/hooks/*`. Re-runs are a no-op. Any
clobbered hook is backed up to `<name>.backup.<timestamp>`.

If symlinks are not supported (some Windows + Git Bash setups), the installer
falls back to copying — in that case re-run after editing the wrapper scripts.
The analyzer itself, [scripts/security_review_changes.py](../../scripts/security_review_changes.py),
is always picked up live.

## Which pull paths are covered

`git pull` resolves to several different operations depending on remote state
and config. Each one fires a different hook:

| Pull shape | Hook fired | Behaviour on HIGH/CRITICAL |
|---|---|---|
| `git pull` (non-fast-forward, merge commit created) | `pre-merge-commit` | **Blocks the merge.** Run `git merge --abort` to bail. |
| `git pull` (fast-forward, no merge commit) | `post-merge` | Cannot block — merge already applied. Prints `git reset --hard ORIG_HEAD` to roll back. |
| `git pull --rebase` | `pre-rebase` | **Blocks the rebase.** Run `git rebase --abort` to bail. |
| `git merge <branch>` | `pre-merge-commit` | Blocks. |
| `git rebase <upstream>` | `pre-rebase` | Blocks. |

## What the analyzer looks at

Only files that could meaningfully change behaviour are inspected — to keep the
review fast and the false-positive rate low. The current "interesting" set:

- Scripts: `*.py`, `*.sh`, `*.bash`, `*.zsh`, `*.js`, `*.mjs`, `*.cjs`, `*.ts`
- Skills: `.claude/skills/**/SKILL.md`
- Hook code: `.claude/hooks/**`, `scripts/git-hooks/**`
- Permissions / config: `.claude/settings.json`, `.claude/settings.local.json`,
  `.vibe/config.toml`
- Env loaded into shell commands: `.envrc`, `.envrc.example`
- Build/install entry points: `Makefile`, `pyproject.toml`, `package.json`

For each interesting file the analyzer runs two layers:

### Layer 1 — static rules (always)

| Rule | Severity | Catches |
|---|---|---|
| `pipe-to-shell` | CRITICAL | `curl … \| sh`, `wget … \| bash`, `… \| python` |
| `base64-to-shell` | CRITICAL | `base64 -d … \| sh` style obfuscation |
| `reverse-shell` | CRITICAL | `nc -e`, `/dev/tcp/`, `bash -i >&` |
| `ssh-key-write` | CRITICAL | appends to `authorized_keys`, `ssh-copy-id` |
| `permissions-allow-added` | CRITICAL/HIGH | new entries in `permissions.allow`. CRITICAL when broad (`*` wildcard); HIGH for narrow `Bash(<binary> …)` entries with explicit binaries |
| `permissions-deny-removed` | HIGH | removed `permissions.deny` entries (loosening) |
| `hook-added` | HIGH | new `hooks.<event>.command` entries in settings |
| `credential-read` | HIGH | reads of `~/.ssh/`, `~/.aws/`, `~/.netrc`, `/etc/passwd`, `.env` |
| `sudo` | HIGH | any `sudo …` |
| `no-verify` | HIGH | `--no-verify`, `--no-gpg-sign` (commit-hook bypass) |
| `chmod-exec` | HIGH | `chmod +x` |
| `shell-eval-py` | HIGH | `subprocess.*(…, shell=True)` |
| `shell-eval-py-2` | HIGH | `os.system`, `os.popen`, `exec(`, `eval(` |
| `shell-eval-sh` | HIGH | bash `eval "…"` |
| `rm-rf-var` | HIGH | `rm -rf $VAR` (variable expansion in destructive paths) |
| `trusted-script-modified` | HIGH | a script referenced by an allow-list entry was modified — its new behaviour will be auto-approved on next call |
| `skill-new-script-ref` | HIGH | a `SKILL.md` started referencing a script it did not reference before |
| `network-egress-*` | MEDIUM | `requests.*`, `urllib`, `fetch(`, raw `curl/wget` to `https?://` |
| `env-var-added` / `env-var-changed` | MEDIUM | settings `env` block diffs |
| `dynamic-import` | MEDIUM | `importlib.import_module`, `__import__` |
| `file-deleted` | LOW | (informational) |

Static rules run on **added lines only** — context lines and deletions are not
re-flagged.

### Layer 2 — semantic review (Claude, best-effort)

If the `claude` CLI is reachable, the full diff is passed to a headless
`claude -p` invocation with a security-review prompt. This catches things the
static rules miss: obfuscated payloads, semantically dangerous logic, sneaky
indirection. Output is captured verbatim into the report.

If the binary is not reachable, the analyzer notes the skip in the report and
proceeds with static rules only. Static rules are always sufficient to block
the merge — the semantic layer is augmentation, not the line of defence.

## Severity → action

The analyzer aggregates all findings and emits one **max severity**:

| Max | Hook behaviour |
|---|---|
| CRITICAL | Pre-merge / pre-rebase: exit 1 → merge/rebase aborted by git. Post-merge: shouts to stderr with rollback command. |
| HIGH | Same as CRITICAL. |
| MEDIUM, LOW, CLEAN | Exit 0 → operation proceeds. Findings still written to the report. |

## Bypass / escape hatches

| Need | Env var |
|---|---|
| Skip the entire review | `ASP_SKIP_SECURITY_REVIEW=1 git pull` |
| Skip only the semantic Claude layer | `ASP_SKIP_CLAUDE_REVIEW=1 git pull` |
| Use a specific `claude` binary | `ASP_CLAUDE_BIN=/path/to/claude` |

Use `ASP_SKIP_SECURITY_REVIEW` only when you know the diff is fine and the
`claude` API is unreachable, or when the analyzer itself is broken — never as
a routine workflow.

## Running manually

You can run the analyzer outside a hook against any two refs:

```bash
python3 scripts/security_review_changes.py <old> <new> --label "preview"
```

Useful before a deliberate merge of an unfamiliar branch:

```bash
git fetch origin some-branch
python3 scripts/security_review_changes.py HEAD FETCH_HEAD --label "preview"
```

The `--non-blocking` flag exits 0 even on HIGH/CRITICAL — only used by the
post-merge hook (where the merge has already happened) and rarely needed
manually.

## Tuning

### Add a new rule

Append a tuple to `SHELL_PATTERNS` in
[scripts/security_review_changes.py](../../scripts/security_review_changes.py):

```python
SHELL_PATTERNS: list[tuple[str, str, str]] = [
    ...
    ("HIGH", "rule-name", r"<regex>"),
]
```

The regex is matched against added lines only, with comments stripped. One
finding per rule per file (so a noisy rule doesn't drown the report).

### Special-case a structured file

`.claude/settings.json` is parsed with `scan_settings_json` — diffs the
`permissions.allow`, `permissions.deny`, `hooks`, and `env` keys structurally
rather than line-by-line. Add a similar function and route it from `main()` if
another structured file (e.g. `pyproject.toml`) needs the same treatment.

### Self-flagging

Adding a new entry to `permissions.allow` will itself be flagged HIGH the next
time someone pulls the change into a fresh clone. That is expected — the user
sees what they're agreeing to once, then subsequent pulls are silent. The same
applies to new git hooks and new skill scripts.

## Known limits

- **Static rules are pattern-based.** Determined attackers can split a payload
  across files, encode it, or hide it behind `exec()` of dynamically loaded
  content. The semantic layer covers some of that; the rest is what code review
  is for.
- **The hooks live in `.git/hooks/`**, which is per-clone. A fresh clone has no
  hooks until the user runs `scripts/install_git_hooks.sh`. There is no way to
  ship git hooks via the repo itself — git deliberately rejects that.
- **The analyzer trusts itself.** A change to `scripts/security_review_changes.py`
  modifies the analyzer that is about to run on it. The hooks always run the
  *current* working-tree version after the merge for `post-merge`, but for
  `pre-merge-commit` and `pre-rebase` the analyzer that runs is the one that
  was on disk *before* the merge — so a malicious update to the analyzer
  itself takes one extra step to land. Review changes to this file with extra
  scrutiny.

## Related

- [CLAUDE_RECAP.md](CLAUDE_RECAP.md) — sibling system for self-improvement
  recaps of Claude transcripts (different purpose, similar wiring shape).
- [BRANCH_PROTECTION_CONCEPT.md](BRANCH_PROTECTION_CONCEPT.md) — the broader
  trust-and-gate story for this repo.
