---
id: IDEA-047
title: Device-resource arbiter for parallel test sessions
category: tooling
description: Mutual-exclusion lock around scarce hardware (ESP32, nRF52840, Pixel) so two parallel agent sessions can't grab the same device simultaneously, with self-recovery from hangs and crashes.
---

## Motivation

Connected test devices — ESP32 dev board, nRF52840 dev board, Pixel 9
Android phone — are **rare resources**. Multiple Claude Code sessions
frequently run against this repo in parallel (see the existing
"Parallel sessions — commit only your own work" rule in `CLAUDE.md`).
If two sessions both invoke `/test-device esp32-button` or
`/verify-on-device` against the same physical device at the same time,
the result is at best garbage data and at worst a wedged device or a
half-flashed firmware.

Today there is no coordination — discipline is the only safety net.
We want an arbiter that guarantees **at most one session uses a given
device at a time**, and recovers automatically when something goes wrong.

## Rough approach

A small CLI arbiter that wraps OS-level file locking, called from the
device-touching skills (`/test-device`, `/verify-on-device`, and any
direct PlatformIO `make test-*` invocations).

### Components

1. **`scripts/device_lock.py`** — small Python CLI:
   - `acquire <device> --task TASK-NNN [--timeout SECS]` — blocks until
     the lock is held or timeout expires; exits non-zero on timeout
     with a clear message naming the current holder.
   - `release <device>` — explicit release (rarely needed; flock auto-
     releases on process exit).
   - `status [<device>]` — print who currently holds each device.
   - `run <device> -- <command...>` — convenience: acquire, run the
     command, release on exit (incl. SIGINT / SIGTERM / crash).

2. **Lock files** in a stable location outside the repo, e.g.
   `~/.cache/asp-device-locks/<device>.lock`. Contents (JSON):

   ```json
   {
     "holder_pid": 12345,
     "holder_session": "claude-2026-04-28T14-03",
     "task_id": "TASK-282",
     "acquired_at": "2026-04-28T14:03:11Z",
     "expires_at":  "2026-04-28T14:18:11Z",
     "command": "make test-esp32-button"
   }
   ```

3. **Skill integration** — `/test-device` and `/verify-on-device`
   wrap the underlying command with `device_lock.py run <device> --`.
   The wrapping is invisible on the happy path; on contention the
   user sees:

   > ESP32 is held by session `claude-2026-04-28T14-03` running
   > TASK-282 (`make test-esp32-button`), acquired 14:03:11 — expires
   > 14:18:11. Waiting up to 60s, then aborting.

### Devices to arbitrate (initial set)

| Device | ID | Touched by |
|---|---|---|
| ESP32 dev board | `esp32` | `make test-esp32-*`, firmware flashing |
| nRF52840 dev board | `nrf52840` | `make test-nrf52840-*`, firmware flashing |
| Pixel 9 (Android) | `pixel-9` | `/verify-on-device`, `flutter run`, `adb` flows |

Add more later if needed (logic analyser, second pedal, etc.).

## Hang and crash recovery — defence in depth

Three layers, each catching what the previous one misses:

### Layer 1 — `flock` for crash recovery (free)

`fcntl.flock` is released by the kernel when the holding process
exits, **including SIGKILL, OOM-kill, panic, power loss**. So any
form of process death — except a frozen-but-alive process — is
self-healing. No code needed; this comes from using `flock` at all.

### Layer 2 — max-hold timeout (handles wedged child)

The `device_lock.py run` wrapper kills its own child after a
configurable `--timeout` (default e.g. 15 minutes for tests, 30 for
interactive `/verify-on-device`). If the test or app hangs, the
wrapper kills it cleanly, the lock is released, and the next session
can proceed. The wrapper logs the kill loudly so the original session
isn't surprised.

### Layer 3 — TTL-based stale-lock steal (handles wedged wrapper)

Each lock file carries an `expires_at` timestamp roughly
`acquired_at + max_hold_timeout + grace`. If a future acquirer finds
a lock whose `expires_at` is in the past **and** the holder PID is
either gone or unreachable, it steals the lock with a loud warning:

> ⚠ Stealing stale lock on `esp32` (held by PID 12345 since 14:03,
> expired 14:23). Previous holder may have crashed.

This catches the rare case where even the wrapper is wedged at the
OS level (NFS hang, USB driver deadlock, signal-uncatchable state).

### Layer 4 — verify-holder-on-acquire (handles "ghost lock")

The dangerous case is **A finished cleanly but the arbiter still
thinks it's holding the device** — release was skipped (cleanup-path
bypassed, filesystem ate the unlink, wrapper crashed *after* useful
work but *before* unlinking, lock file created but `flock()` never
actually grabbed). The lock file says "held" forever, B is locked
out, and no TTL has elapsed yet.

The fix is: **never trust the lock file's contents alone — always
verify the holder is actually alive and actually holding the
OS-level lock.** When `acquire` sees an existing lock file, it does
*both* of:

1. `os.kill(holder_pid, 0)` — does the PID still exist on this host?
2. Try `fcntl.flock(LOCK_EX | LOCK_NB)` on the lock file — can we
   grab the OS-level lock right now?

If **either** check says "no holder", the lock is a ghost. Reclaim
it immediately with a quiet log line (no scary warning — this is
expected hygiene, not corruption):

> Reclaiming abandoned lock on `esp32` (PID 12345 not running) —
> proceeding.

This makes the "release didn't reach the arbiter" failure mode
**self-healing on the next acquire**, with no waiting on TTL,
because PID-liveness and flock-state are both ground truth — the
lock file's content is just metadata for diagnostics.

Pair this with a `device_lock.py status --gc` subcommand that
proactively reaps ghost locks. Wire it into a pre-commit hook or
the start of `/test-device` so stale state from a prior session is
cleared before contention even surfaces.

## Open questions

- **Granularity** — is "one ESP32" enough, or do we need to
  distinguish `esp32-button` vs `esp32-led` test suites? Probably
  one lock per physical device is right; tests serialise naturally.
- **Wait vs fail** — when the lock is contended, should the second
  session block (wait for release) or fail fast and let the agent
  pick another task? Lean: short blocking wait (e.g. 60s) then fail
  with a clear message, so the agent can decide to retry, switch
  tasks, or ask the user.
- **Detection of "is this device actually plugged in?"** — the lock
  protects against concurrent use, but doesn't help if the device is
  simply not connected. Out of scope here; existing
  `make test-*` / `flutter devices` output already reports that.
- **Cross-machine sessions** — locks are per-host. If the user runs
  Claude Code on two machines that share a USB device via remote
  forwarding, this won't help. Out of scope; not a real workflow today.
- **Skill auto-retry** — when an acquire fails, should the skill
  re-prompt the agent with "ESP32 is busy — work on a non-device
  task or wait?" or just exit and let the agent decide? Probably the
  latter — the agent has more context.
- **Lock-file location on Windows** — `~/.cache/` doesn't exist by
  default on Windows. Use `platformdirs.user_cache_dir("asp")` to
  pick the right path on each OS.

## Out of scope (for the first version)

- Queueing fairness ("first to wait gets it next") — first version
  can be racy; optimise later if it matters.
- Reservation / scheduling ahead of time.
- A web dashboard showing who holds what.
- Integration with non-CLI flows (IDE Run buttons, etc.) — agents
  call the wrapped CLI; humans clicking Run in VS Code can still
  collide, but that's a smaller failure mode.

## Pointers to related work

- `CLAUDE.md` → "Parallel sessions — commit only your own work" —
  same root cause (parallel agent sessions), same need for explicit
  coordination.
- Existing skills that touch hardware:
  [.claude/skills/test-device/SKILL.md](../../../.claude/skills/test-device/SKILL.md),
  [.claude/skills/verify-on-device/SKILL.md](../../../.claude/skills/verify-on-device/SKILL.md).
- IDEA-014 (automated hardware testing rig) — different goal
  (relay-driven inputs), but shares the "hardware is a scarce
  scheduled resource" framing.
