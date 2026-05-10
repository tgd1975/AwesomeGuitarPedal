# Claude Recap — background transcript analysis

A self-improvement loop for the Claude Code workflow: every so often, a headless
Claude session reads recent conversation transcripts and writes a short recap
into [.claude-recap.md](../../.claude-recap.md) at the repo root, calling out
repeated manual steps, skipped skills, CLAUDE.md rule violations, and ideas for
new automation.

The recap file is **gitignored** — it is local feedback, not committed history.

## How it is wired

| Piece | Location |
|---|---|
| Trigger | `SessionStart` hook in [.claude/settings.json](../../.claude/settings.json) |
| Script | [scripts/claude_recap.py](../../scripts/claude_recap.py) |
| Output | `.claude-recap.md` (review), `.claude-recap.log` (script log), `.claude-recap.lock` (mutex) — all at repo root, all gitignored |
| Transcripts read | `~/.claude/projects/-home-tobias-Dokumente-Projekte-AwesomeStudioPedal/*.jsonl` |
| Headless model | `claude -p --model sonnet` (binary auto-located under `~/.vscode/extensions/anthropic.claude-code-*-linux-x64/`) |

The hook fires at every Claude Code session start and launches the script in
the background with `nohup … &`. The session itself is not blocked — the script
runs detached and writes its own log.

## What the script does

1. Acquires `.claude-recap.lock` (skips if another run is in progress).
2. Reads the `<!-- last_run: ... -->` marker from `.claude-recap.md`.
3. Throttles: if the last run was less than **6 hours** ago, exits cheaply.
   Target cadence is ~2 runs/day, so pressing the trigger multiple times a
   day is harmless.
4. Lists transcripts modified after `last_run`. If more than **20** accumulated,
   keeps the most recent 20 and drops older ones — they will not reappear in
   future runs (this is intentional; the very first run after a long gap can
   skip backlog).
5. For each transcript, distils the JSONL into a small markdown summary —
   user messages, tool-call headers (name + brief input), assistant text
   replies (truncated), tool errors. Bulky tool *results* are stripped, since
   they account for most volume and rarely matter for review questions.
6. Invokes `claude -p --model sonnet --max-budget-usd 1.00 --no-session-persistence`
   with a prompt that pre-extracts the summaries and instructs the agent to
   append a new dated section to `.claude-recap.md`.
7. Cleans up the temp summary directory.

The full prompt the headless agent receives is in `build_prompt()` inside the
script.

## What it looks for

The prompt asks the agent to flag:

1. **Repeated manual command sequences** that could become a new skill or script.
2. **Existing skills that were skipped** when they should have fired (e.g.
   ad-hoc `bluetoothctl` instead of `/ble-reset`).
3. **Skills or scripts that errored** or were corrected mid-use.
4. **CLAUDE.md rule violations** — `git add -A`, missing `/ts-task-active`
   before edits, silent `--no-verify`, etc.
5. **Permission prompts the user keeps approving** — candidates for the
   settings.json allow list.
6. **Hook ideas** — automated behaviours that would have helped.

Each finding is one line, references the transcript by short ID
(e.g. `004fb34f`), and quotes or paraphrases briefly. Output is capped well
under 200 lines per run.

## Output format

`.claude-recap.md` accumulates dated sections, newest at the bottom:

```markdown
# Claude Recap

Automated post-session analysis. This file is gitignored.

<!-- last_run: 2026-04-29T08:15:42+00:00 -->

## 2026-04-29T08:15:42+00:00

Analysed 7 transcript(s).

### New skill / automation candidates
- 004fb34f — "ran clang-format then make test-host three times" → could be a /precommit-check skill.

### Existing skills — gaps or misuse
- 9a1c2e80 — used raw `adb shell uiautomator dump` instead of /ui-dump.

### Notes
- 1f3d… — pre-commit hook bypassed with --no-verify on a non-empty .cpp diff (rule violation).
```

The agent maintains exactly one `<!-- last_run: ... -->` marker, updated to the
current ISO timestamp on every successful run. That marker is also how the
script throttles and finds new transcripts.

## Permissions

The `permissions.allow` block in [.claude/settings.json](../../.claude/settings.json)
grants the headless agent everything it needs:

- Read `~/.claude/projects/*` (transcripts) and `/tmp/claude-recap-*/**`
  (pre-extracted summaries).
- Read/Write/Edit `.claude-recap.md`.
- Run `python3 scripts/claude_recap.py` (so a session can re-trigger it).

If the headless agent hits a permission denial, the prompt instructs it not to
retry — instead it appends a `### Missing permissions` section listing the
exact `Bash(...)` patterns to add, then stops. The agent is explicitly told
**not** to edit `settings.json` itself.

## Disabling or pausing

| Goal | How |
|---|---|
| Skip one run | `touch .claude-recap.lock` before starting Claude Code (and remove it later). |
| Throttle harder | Edit `MIN_INTERVAL` in [scripts/claude_recap.py](../../scripts/claude_recap.py). |
| Disable entirely | Remove the `SessionStart` block from [.claude/settings.json](../../.claude/settings.json). |
| Re-run on demand | `python3 scripts/claude_recap.py` (subject to the 6-hour throttle). |

## Troubleshooting

- **No review appears after a session.** Check `.claude-recap.log` — the
  script logs every run, including throttle skips and missing-binary errors.
- **`ERROR: no claude binary found`.** The script looks for the bundled
  VSCode-extension binary under `~/.vscode/extensions/anthropic.claude-code-*-linux-x64/resources/native-binary/claude`.
  If you use a different install, adjust `find_claude_binary()`.
- **Stale lock file.** If a previous run crashed, `.claude-recap.lock` may
  linger and block subsequent runs. Delete it.
- **Budget exceeded.** Each headless run is capped at $1.00 via
  `--max-budget-usd`. If the agent terminates early, the `last_run` marker
  may not have been updated — the next run will re-analyse the same window.
