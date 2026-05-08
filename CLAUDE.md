# Project: AwesomeStudioPedal

## OS context

This project is developed on both **Windows 11** and **Ubuntu**. At the start of every session, check the platform from the system environment info (or run `uname -s`) and apply the correct shell syntax:

| Situation | Rule |
|---|---|
| Windows 11 (Git Bash / MSYS2) | Use Unix shell syntax (bash). Absolute paths use forward slashes: `C:/Users/...`. No `cmd.exe` commands (`dir`, `type`, `copy`, `del`). |
| Ubuntu / Linux | Standard bash. `/dev/null`, `apt`, etc. all work as expected. |

When in doubt, run `/os-context` to detect and announce the current OS before issuing shell commands.

## Missing executables

When a CLI tool or executable is not found (e.g. `markdownlint`, `clang-tidy`, `jq`):

1. Try once with the most obvious alternative location or invocation (e.g. `npx`, full path).
2. If it still fails, **stop and ask the user** to install the tool — do not attempt further workarounds or reimplement the tool's logic in shell/Python.

Example message: "I can't find `markdownlint`. Can you install it (e.g. `npm install -g markdownlint-cli`) and let me know when it's ready?"

Never spiral through multiple fallback strategies for a missing tool. One retry, then ask.

## Dev container software changes

When running inside a dev container (detected by `REMOTE_CONTAINERS`, `CODESPACES`, `/.dockerenv`, or a `/workspaces/` path) and any software is installed, removed, or configured, **always** invoke `/devcontainer-sync` to update `.devcontainer/devcontainer.json` so the change persists across rebuilds.

## Testing policy

Every implementation — bug fix, new feature, or refactor — **must** be accompanied by fitting tests before it is committed.

**Decision rule:**

| What you changed | Where to write tests |
|---|---|
| Pure logic (no GPIO, no BLE, no Arduino API) | Host tests — `test/unit/test_*.cpp` (GoogleTest) |
| Hardware interaction (GPIO, UART, BLE, ISR timing) | On-device tests — `test/test_*_esp32/` or `test/test_*_nrf52840/` (Unity) |
| Both | Both layers |

**Host test checklist** (run via `make test-host`):

1. Add a `test/unit/test_<feature>.cpp` (or extend the relevant existing file).
2. Register the source file in `test/CMakeLists.txt` under `pedal_tests`.
3. Run `/test` to verify all tests pass before committing.

**On-device test checklist** (run via `make test-esp32-*` / `make test-nrf52840-*`):

1. Add or extend a test in the matching `test/test_*_esp32/` or `test/test_*_nrf52840/` directory.
2. Document that the test requires hardware in the task or PR description.
3. Run `/test-device <target>` (e.g. `/test-device esp32-button`) with a connected device before marking the task done.

If a piece of code can be shimmed (see `test/fakes/arduino_shim.h` and the `HOST_TEST_BUILD` guard pattern), prefer a host test over an on-device test — host tests run faster, require no hardware, and are enforced by the pre-commit hook.

When in doubt, ask: "Can I fake the hardware dependency with a shim or mock?" If yes → host test.

## Human interaction — batch questions, don't loop

Keep the number of back-and-forth exchanges to a minimum. When you have multiple questions
before starting a task, ask them **all in one message** rather than one at a time.

**Rule:** If N questions can be asked simultaneously, ask all N at once. Only use a
sequential ask→do→ask→do loop when each answer genuinely depends on the previous one
(e.g. the answer to Q1 determines whether Q2 is even relevant).

**Examples:**

| Situation | Wrong | Right |
|---|---|---|
| Need package name + target file before editing | Ask for name → edit → ask for file | Ask for both upfront |
| Ambiguous scope with 3 unknowns | Ask one, wait, ask next | Ask all three together |
| Q2 depends on answer to Q1 | — | Sequential is correct here |

This does **not** mean reducing oversight — steering questions and approval checkpoints
are still important. The goal is to avoid unnecessary round-trips where the human sits
idle while you ask things you could have asked together.

## Auto-activate tasks when work begins

As soon as you actually start working on a task — i.e. you are about to make edits,
run experiments, or otherwise change repo state in service of `TASK-NNN` — invoke
`/ts-task-active TASK-NNN` **before the first such action**. This overrules the
older "user commits when they start real work" wording in the `ts-task-active`
skill: **you trigger the activation, not the user.**

**Rules:**

- Trigger point: the first repo-state-changing action you take for the task
  (edit, write, run-with-side-effects, scaffold). Pure reading / planning does
  not count.
- If the task is already in `active/`, do nothing — `/ts-task-active` will
  detect that and report "already active".
- If the task is in `paused/`, the same skill resumes it — still the right call.
- Do **not** auto-activate when the user is only asking a question about the
  task ("what's TASK-47 about?", "summarise it"). Activation tracks *work*,
  not *attention*.
- Do not commit the activation — `/ts-task-active` deliberately leaves the
  status change unstaged so it rides along with the first real commit for the
  task.
- When the task ID is ambiguous (no `TASK-NNN` mentioned), ask once which
  task this work belongs to before proceeding.

## Commits go through /commit — always

Every commit on this repo must flow through the `/commit` skill, which uses git's
**pathspec form** (`git commit -m "..." -- <files>`) to commit atomically without
touching the real index. Never run `git add` followed by `git commit` directly,
and never invoke `git commit` without an explicit pathspec list.

**Why this is mandatory** — see [docs/developers/COMMIT_POLICY.md](docs/developers/COMMIT_POLICY.md)
for the full rationale (parallel-session race, pathspec atomicity, hook
load-bearing role). Short version: multiple Claude Code sessions run against
this repo in parallel, and `git add` + `git commit` mutates a shared index. A
hook failure in one session leaves another session's foreign staged files
mixed in with yours. Pathspec form scopes blast radius to a single commit
attempt.

**Rules:**

- Always invoke `/commit "<message>" <file> [<file> …]`. Never `git add` +
  `git commit`. Never `git commit` without a pathspec list.
- Stage and commit **only the files you changed in the current conversation**.
  If `git status` shows staged or modified files you did not touch, leave them
  alone — they almost certainly belong to another in-flight session. Do not
  mention or "clean up" the foreign files.
- **Exception:** if the user explicitly says "commit all staged files", "commit
  everything", or similar, then include them.

**Bypass** — `ASP_COMMIT_BYPASS=<reason>` in the environment of the `git commit`
invocation skips the `/commit`-only check. The reason string is mandatory
(non-empty), and every bypass is logged to `.git/asp-commit-bypass.log` for
periodic review. Reserved for: interactive rebases (git invokes `git commit`
internally during conflict resolution), recovery from a broken `/commit` skill
itself, and rare manual repo surgery. Empty / unset = no bypass. See
COMMIT_POLICY.md for the full bypass policy.

## Pre-commit hook failures on unrelated changes

When a commit fails because the pre-commit hook (e.g. unit tests, clang-format) runs against
**unstaged or pre-existing changes that are unrelated to the files being committed**, bypassing
the hook (`--no-verify`) may be acceptable — but it is always a case-by-case decision.

**Before suggesting `--no-verify`, check all of the following:**

| Check | Question |
|---|---|
| Pathspec files only | Are all files in this commit's pathspec list unrelated to the hook failure? (e.g. only `.md` files in the commit, but C++ tests fail.) Compare against the pathspec list, not against `git status` — pathspec form does not stage anything, so the working tree may show unrelated mods. |
| Pre-existing breakage | Is the failing check broken on `main` / in the working tree already, not caused by this commit? |
| No silent regression | Would bypassing hide a real regression introduced by the pathspec files? |

**If all three are true**, present this to the user:

> "The pre-commit hook failed, but the failure is in `[file/check]` which is unrelated to
> the files in this commit (`[list pathspec files]`). This appears to be a pre-existing issue.
> It may be OK to bypass the hook for this commit with `--no-verify`.
> Do you want me to proceed with `--no-verify`, or fix the hook failure first?"

**Never bypass silently.** Always name the failing check, explain why it appears unrelated,
and get explicit user approval before using `--no-verify`.

Use `/commit` to apply this protocol consistently — it commits via pathspec form, and on hook failure runs the three checks above before asking about `--no-verify`.

## Project env vars — use `$ASP_*`, never retype device serials

Device handles (Pixel serial, ESP32/nRF USB port, pedal MAC) live in `.envrc` and are exposed as `$ASP_PIXEL_DEVICE`, `$ASP_ESP32_PORT`, `$ASP_NRF52840_PORT`, `$ASP_PEDAL_MAC` (plus `$ANDROID_SERIAL` mirrored from `$ASP_PIXEL_DEVICE`). Reference these in commands and skills — **never retype the literal hex inline**. Setup steps and the canonical variable list are in [DEVELOPMENT_SETUP.md](docs/developers/DEVELOPMENT_SETUP.md#project-env-vars-direnv); the template is [.envrc.example](.envrc.example).

## BLE pairing recovery — use /ble-reset

When BLE pairing with the pedal flakes (you ran `bluetoothctl disconnect <MAC>` once and it's still misbehaving), use `/ble-reset` instead of retrying `bluetoothctl` commands by hand. It runs the canonical disconnect → remove → scan → pair → connect → verify sequence with bounded timeouts and exits non-zero naming the failed step on first failure. Reads the pedal MAC from `$ASP_PEDAL_MAC`.

## Driving the Android app — use /ui-dump and /verify-on-device

Any time you would type `adb shell uiautomator dump` (or any of the surrounding `adb pull /sdcard/ui.xml`, `python3 ui_find.py`, `adb shell input tap` chain) to interact with the Pixel app, invoke `/ui-dump` instead. It owns the dump-pull-find-tap loop, resolves the device serial from `ANDROID_SERIAL` (no hard-coded hex), and is the entry point for both exploratory drives ("tap through it once") and one-shot checks. For full feature-test or defect verification with named scenarios, use `/verify-on-device <TASK-ID> <SCENARIO-ID>` — that has the scenario catalog and pass/fail handling. Both skills delegate raw `adb` to themselves; do not hand-roll `adb shell uiautomator dump …` outside them.

## Documentation persona placement — use /doc-check

After creating or moving any `.md` file under `docs/` (excluding `docs/developers/tasks/` and `docs/developers/ideas/`, which are internal scaffolding), invoke `/doc-check` to validate persona placement (builder / musician / developer). On a Mismatch verdict the skill proposes a `git mv` and waits for confirmation — it never moves silently.

## Task-system regen — use /housekeep

After any task-system file change (status edits, idea moves, epic edits, ad-hoc OVERVIEW regen), invoke `/housekeep` rather than running `python scripts/housekeep.py --apply` directly. The skill stages the regenerated index files (OVERVIEW.md / EPICS.md / KANBAN.md / ideas OVERVIEW) so they ride along with the change that triggered them. The `/ts-task-active` / `/ts-task-done` / `/ts-task-pause` / `/ts-task-reopen` skills already invoke housekeep internally — `/housekeep` is for manual edits outside that path.

The pre-commit hook also runs `housekeep --apply` and stages OVERVIEW/EPICS/KANBAN as a backstop, so a stale index never lands. It does **not** stage moved task files — those still belong to whichever session edited the status. Treat the hook as a safety net, not a substitute: regen at edit time keeps `git status` clean and lets you review what's about to be committed.

**OVERVIEW/EPICS/KANBAN are an exception to "commit only your own work".** These four files (`docs/developers/tasks/OVERVIEW.md`, `EPICS.md`, `KANBAN.md`, and `docs/developers/ideas/OVERVIEW.md`) are entirely generated from on-disk state by `housekeep.py`. They have no per-author authorship — whoever commits the regen first wins, and a later commit will simply re-derive the latest counts. So:

- **When you make any commit, if these files are sitting modified, include them in your pathspec.** No need to check who triggered the regen. They are cheap to land and incorrect to leave behind.
- **Conversely, don't be disappointed if you triggered a regen and a parallel session committed it before you.** Their commit landed the same generated content yours would have. Re-run housekeep if you need to refresh, and move on.

This applies to the four index files above only. Moved *task* files (e.g. `task-NNN-foo.md` rename from `active/` to `closed/`) still belong to whichever session edited the status, per the parallel-session rule.

## Git reconnaissance — use /status

Use `/status` for routine git reconnaissance (branch, last 3 commits, staged short, working short) instead of separate `git status` / `git log` / `git rev-parse` calls. One bundled invocation collapses several permission prompts into one.

## Searching the codebase

For any code search likely to take **more than 3 grep / find queries** — e.g. "find every place that calls X across naming conventions", "trace how state flows through the app" — delegate to the `Explore` subagent (`Agent(subagent_type="Explore", …)`) instead of hand-rolling the searches. One Explore call covers many lookups, returns a synthesised result, and avoids per-call permission prompts and context-window bloat. Targeted single lookups (one file, one symbol) stay as direct `grep` / `find` — Explore is for multi-step exploration.

## Skill registration

When adding a new skill (creating `.claude/skills/<name>/SKILL.md`), always also add `<name>` to `enabled_skills` in [.vibe/config.toml](.vibe/config.toml). Failing to do so means the skill exists on disk but is not registered and may not be loaded.

## Task-system source-of-truth

`awesome-task-system/` is the **canonical source** for the task-system scripts, skills, config, and tests. The live copies under `scripts/`, `.claude/skills/`, and `docs/developers/task-system.yaml` are generated artifacts.

**Workflow when changing any task-system file:**

1. Edit the package copy under `awesome-task-system/` — never edit the live copy.
2. Run `python scripts/sync_task_system.py --apply` to copy package → live.
3. Stage and commit both sides together.

The pre-commit hook calls `sync_task_system.py --check` and rejects any commit where the two sides diverge. The full mirror set lives in the `MIRRORS` list at the top of [scripts/sync_task_system.py](scripts/sync_task_system.py); add new mirrored files there when introduced. See [awesome-task-system/LAYOUT.md](awesome-task-system/LAYOUT.md) for the full workflow.

## Auto-memory

Claude's auto-memory lives at `~/.claude/projects/<project-hash>/memory/` — **outside this repo, not versioned by git**. Edits made during a session persist for future conversations on the same machine, but are not captured in commits, not portable across machines, and not reviewable in PRs.

Practical consequences:

- A request like "commit that memory update" cannot be satisfied today — the memory directory is not a git working tree.
- Memory is per-machine; the Windows 11 and Ubuntu checkouts of this project see different memory.
- Accidentally deleted memory is gone — there is no git history to recover from.

Closing this gap is tracked in [IDEA-050](docs/developers/ideas/open/idea-050-version-auto-memory-via-awesome-task-system.md) — apply the awesome-task-system mirror pattern to memory so edits become real, reviewable commits.
