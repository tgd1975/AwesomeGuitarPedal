#!/usr/bin/env python3
"""Run a headless Claude Code recap of recent session transcripts.

Triggered by the SessionStart hook. Reads transcripts in
~/.claude/projects/<project-dir>/ that were modified after the last_run
marker in .claude-recap.md, then invokes `claude -p` with a prompt asking
the agent to analyse those transcripts and append findings to
.claude-recap.md.

Runs in the background; output is appended to .claude-recap.log.
"""
from __future__ import annotations

import datetime as _dt
import glob
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
REVIEW_FILE = REPO_ROOT / ".claude-recap.md"
LOG_FILE = REPO_ROOT / ".claude-recap.log"
LOCK_FILE = REPO_ROOT / ".claude-recap.lock"

PROJECT_TRANSCRIPT_DIR = (
    Path.home()
    / ".claude"
    / "projects"
    / "-home-tobias-Dokumente-Projekte-AwesomeStudioPedal"
)

LAST_RUN_RE = re.compile(r"<!--\s*last_run:\s*([0-9T:.\-+Z]+)\s*-->")

# Cap per run so we never blow the budget or timeout. If more transcripts
# accumulated than this (e.g. on the very first run), only the most recent
# MAX_TRANSCRIPTS are analysed and the rest are skipped — the next run will
# still see them as "new" only if they were modified after the last_run
# marker, so older ones are effectively dropped on purpose.
MAX_TRANSCRIPTS = 20

# Minimum gap between successive analyses. Pressing + multiple times a day
# is fine — the script returns cheaply until this much time has elapsed
# since the last successful run. ~2 runs/day is the target cadence.
MIN_INTERVAL = _dt.timedelta(hours=6)


def find_claude_binary() -> str | None:
    """Locate the bundled VSCode-extension claude binary (newest version)."""
    pattern = str(
        Path.home()
        / ".vscode"
        / "extensions"
        / "anthropic.claude-code-*-linux-x64"
        / "resources"
        / "native-binary"
        / "claude"
    )
    candidates = sorted(glob.glob(pattern))
    return candidates[-1] if candidates else None


def read_last_run() -> _dt.datetime | None:
    if not REVIEW_FILE.exists():
        return None
    text = REVIEW_FILE.read_text(encoding="utf-8", errors="replace")
    match = LAST_RUN_RE.search(text)
    if not match:
        return None
    raw = match.group(1).replace("Z", "+00:00")
    try:
        return _dt.datetime.fromisoformat(raw)
    except ValueError:
        return None


def _truncate(s: str, limit: int) -> str:
    if len(s) <= limit:
        return s
    return s[:limit] + f"…[truncated {len(s) - limit} chars]"


def summarize_transcript(path: Path, out_dir: Path) -> Path | None:
    """Distill a JSONL transcript into a small markdown summary.

    Keeps user messages, assistant text replies (truncated), and tool-call
    headers (tool name + brief input). Drops bulky tool *results* — that is
    where most transcript volume lives and it is rarely useful for the
    review questions.
    """
    out = out_dir / (path.stem + ".md")
    user_msgs: list[str] = []
    asst_text: list[str] = []
    tool_calls: list[str] = []
    errors: list[str] = []
    try:
        with path.open("r", encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    rec = json.loads(line)
                except json.JSONDecodeError:
                    continue
                t = rec.get("type")
                msg = rec.get("message") or {}
                content = msg.get("content")
                if t == "user" and isinstance(content, list):
                    for block in content:
                        if isinstance(block, dict) and block.get("type") == "text":
                            txt = (block.get("text") or "").strip()
                            if txt:
                                user_msgs.append(_truncate(txt, 600))
                elif t == "assistant" and isinstance(content, list):
                    for block in content:
                        if not isinstance(block, dict):
                            continue
                        bt = block.get("type")
                        if bt == "text":
                            txt = (block.get("text") or "").strip()
                            if txt:
                                asst_text.append(_truncate(txt, 400))
                        elif bt == "tool_use":
                            name = block.get("name", "?")
                            inp = block.get("input") or {}
                            preview = ""
                            if isinstance(inp, dict):
                                if "command" in inp:
                                    preview = str(inp["command"])
                                elif "file_path" in inp:
                                    preview = str(inp["file_path"])
                                elif "skill" in inp:
                                    preview = "/" + str(inp["skill"])
                                elif "description" in inp:
                                    preview = str(inp["description"])
                            tool_calls.append(
                                f"{name}: {_truncate(preview, 200)}".strip().rstrip(":")
                            )
                # Look for explicit error markers in tool results (cheap scan)
                if "is_error" in rec and rec.get("is_error"):
                    errors.append(_truncate(line, 300))
    except OSError as e:
        return None

    if not (user_msgs or asst_text or tool_calls):
        return None

    lines: list[str] = [f"# Transcript {path.stem[:8]}", ""]
    if user_msgs:
        lines.append("## User messages")
        lines.extend(f"- {m}" for m in user_msgs)
        lines.append("")
    if tool_calls:
        lines.append("## Tool calls")
        lines.extend(f"- {c}" for c in tool_calls)
        lines.append("")
    if asst_text:
        lines.append("## Assistant text replies")
        lines.extend(f"- {a}" for a in asst_text)
        lines.append("")
    if errors:
        lines.append("## Tool errors")
        lines.extend(f"- {e}" for e in errors[:20])

    out.write_text("\n".join(lines), encoding="utf-8")
    return out


def transcripts_since(cutoff: _dt.datetime | None) -> list[Path]:
    if not PROJECT_TRANSCRIPT_DIR.is_dir():
        return []
    files = []
    for p in PROJECT_TRANSCRIPT_DIR.glob("*.jsonl"):
        mtime = _dt.datetime.fromtimestamp(p.stat().st_mtime, tz=_dt.timezone.utc)
        if cutoff is None or mtime > cutoff:
            files.append(p)
    files.sort(key=lambda p: p.stat().st_mtime)
    return files


def build_prompt(summaries: list[Path], last_run: _dt.datetime | None) -> str:
    now_iso = _dt.datetime.now(tz=_dt.timezone.utc).isoformat(timespec="seconds")
    last_run_str = last_run.isoformat(timespec="seconds") if last_run else "(no prior run)"
    transcript_list = "\n".join(f"- {p}" for p in summaries)
    return f"""You are running as a background analysis agent for the AwesomeStudioPedal repo.
Your job: skim recent Claude Code session summaries and append a review section
to .claude-recap.md at the repo root.

CONTEXT
- Repo root: {REPO_ROOT}
- Review file: {REVIEW_FILE} (gitignored)
- Last run: {last_run_str}
- Now: {now_iso}
- Pre-extracted session summaries to analyse (each is a small markdown file
  derived from one raw JSONL transcript — user messages, tool-call headers,
  assistant text, and any tool errors; bulky tool results are stripped):
{transcript_list}

WHAT TO LOOK FOR
1. Repeated manual command sequences that could become a new skill or script.
2. Existing skills under .claude/skills/ that were skipped when they should have fired
   (e.g. user ran ad-hoc `bluetoothctl` instead of /ble-reset).
3. Skills or scripts that errored, were corrected mid-use, or behaved unexpectedly.
4. CLAUDE.md rules being violated (e.g. `git add -A` despite the parallel-sessions rule,
   bypassed pre-commit hooks without the three-check protocol, missing /ts-task-active
   before edits).
5. Permission prompts the user keeps approving for the same pattern (candidate for the
   settings.json allow list — see the fewer-permission-prompts skill).
6. Hook ideas: automated behaviours that would have helped ("from now on whenever X").

OUTPUT FORMAT
Open .claude-recap.md. If it does not exist, create it with this header:

    # Claude Recap

    Automated post-session analysis. This file is gitignored.

    <!-- last_run: {now_iso} -->

If it already exists, update the `<!-- last_run: ... -->` marker to {now_iso}
(there must be exactly one such marker in the file).

Then APPEND a new dated section at the bottom:

    ## {now_iso}

    Analysed N transcript(s).

    ### New skill / automation candidates
    - ...

    ### Existing skills — gaps or misuse
    - ...

    ### CLAUDE.md rule violations
    - ...

    ### Permission / hook suggestions
    - ...

    ### Notes
    - ...

Each bullet should be ONE line, concrete, and reference the transcript file
(by short ID, e.g. `004fb34f`) plus a brief quote or paraphrase. Omit a
subsection entirely if you found nothing for it — do not pad with "nothing
to report". If overall there is nothing actionable, write a single line
"No actionable findings." under the dated heading.

PERMISSIONS
You are running non-interactively. If you hit a permission denial for any tool
— including Edit/Write on .claude-recap.md, Read on a transcript, or any
Bash(...) command — DO NOT retry. Instead add a section titled
`### Missing permissions` listing the exact permission rule strings that
should be added to .claude/settings.json (e.g. `Bash(rg *)`,
`Edit(.claude-recap.md)`, `Write(.claude-recap.md)`, `Read(/some/path)`),
then stop. Do not attempt to edit settings.json yourself.

If Edit/Write on .claude-recap.md is denied specifically, you cannot append
your findings to the file. Emit the full intended .claude-recap.md update as
plain output (so it lands in the recap log) AND emit the
`### Missing permissions` section naming `Edit(.claude-recap.md)` and
`Write(.claude-recap.md)` as the rules to add. Do not write a partial summary
to the log without naming the missing rules — the user needs to know which
allow-list entry to add.

Be terse. Total output to .claude-recap.md should be well under 200 lines
even on a busy week. The summaries are already small — read them directly,
no need to sample.

Do not modify any file other than .claude-recap.md.
"""


def main() -> int:
    LOG_FILE.parent.mkdir(parents=True, exist_ok=True)
    log = LOG_FILE.open("a", encoding="utf-8")
    stamp = _dt.datetime.now(tz=_dt.timezone.utc).isoformat(timespec="seconds")
    log.write(f"\n=== {stamp} claude_recap.py start ===\n")

    # Prevent concurrent runs (e.g. + pressed twice in quick succession)
    # from racing on .claude-recap.md and burning duplicate budget.
    try:
        lock_fd = os.open(str(LOCK_FILE), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
    except FileExistsError:
        try:
            other_pid = LOCK_FILE.read_text().strip()
        except OSError:
            other_pid = "?"
        log.write(f"Another run is in progress (pid={other_pid}); skipping.\n")
        return 0
    try:
        os.write(lock_fd, str(os.getpid()).encode())
    finally:
        os.close(lock_fd)
    try:
        return _run(log)
    finally:
        try:
            LOCK_FILE.unlink()
        except FileNotFoundError:
            pass


def _run(log) -> int:

    claude = find_claude_binary()
    if not claude:
        log.write("ERROR: no claude binary found under ~/.vscode/extensions/\n")
        return 1

    last_run = read_last_run()
    if last_run is not None:
        now = _dt.datetime.now(tz=_dt.timezone.utc)
        elapsed = now - last_run
        if elapsed < MIN_INTERVAL:
            remaining = MIN_INTERVAL - elapsed
            log.write(
                f"Throttled: last run was {elapsed} ago "
                f"(< {MIN_INTERVAL}); next eligible in {remaining}.\n"
            )
            return 0

    transcripts = transcripts_since(last_run)
    if not transcripts:
        log.write("No new transcripts since last run; skipping.\n")
        return 0

    skipped = 0
    if len(transcripts) > MAX_TRANSCRIPTS:
        skipped = len(transcripts) - MAX_TRANSCRIPTS
        transcripts = transcripts[-MAX_TRANSCRIPTS:]
    log.write(
        f"Analysing {len(transcripts)} transcript(s)"
        f"{f' (skipped {skipped} older)' if skipped else ''}"
        f"; last_run={last_run}\n"
    )

    summary_dir = Path(tempfile.mkdtemp(prefix="claude-recap-"))
    summaries: list[Path] = []
    for t in transcripts:
        s = summarize_transcript(t, summary_dir)
        if s is not None:
            summaries.append(s)
    if not summaries:
        log.write("All transcripts produced empty summaries; skipping.\n")
        return 0
    total_bytes = sum(s.stat().st_size for s in summaries)
    log.write(
        f"Wrote {len(summaries)} summaries to {summary_dir} "
        f"({total_bytes} bytes total).\n"
    )
    prompt = build_prompt(summaries, last_run)

    cmd = [
        claude,
        "-p",
        "--model", "sonnet",
        "--max-budget-usd", "1.00",
        "--no-session-persistence",
        "--add-dir", str(summary_dir),
    ]
    log.write(f"Invoking: {claude} -p --model sonnet ... (prompt on stdin)\n")
    log.flush()

    try:
        # claude -p reads the prompt from stdin when no positional arg is
        # given; passing it as argv breaks when stdin is /dev/null (the
        # background-launched case).
        result = subprocess.run(
            cmd,
            cwd=REPO_ROOT,
            input=prompt,
            text=True,
            stdout=log,
            stderr=subprocess.STDOUT,
            timeout=600,
            check=False,
        )
    except subprocess.TimeoutExpired:
        log.write("ERROR: claude -p timed out after 600s\n")
        return 2
    except FileNotFoundError as e:
        log.write(f"ERROR: failed to launch claude: {e}\n")
        return 3

    log.write(f"Exit code: {result.returncode}\n")

    # Clean up summary scratch dir on success or failure — it is only used
    # to feed this run, never read again.
    try:
        for s in summary_dir.iterdir():
            s.unlink()
        summary_dir.rmdir()
    except OSError as e:
        log.write(f"Warning: failed to clean {summary_dir}: {e}\n")

    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
