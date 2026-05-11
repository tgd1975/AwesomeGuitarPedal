---
id: TASK-385
title: Wiring test tool — LED group modes (on/off/blinking/chase/cycle-all)
status: open
opened: 2026-05-11
effort: Medium (2-8h)
complexity: Medium
human-in-loop: No
epic: wiring-test-tool
order: 3
prerequisites: [TASK-383]
---

## Description

Implement the *group* half of the LED checks. Six modes that operate
on the union of all configured LEDs (`ledBluetooth`, `ledPower`,
`ledSelect[]`, and anything else the hardware config lists). Each
mode answers a different electrical question; together they catch
the common solder failures (dead joint, swapped polarity, bridged
neighbours, missing pull-down).

The six modes:

| Mode | Behaviour | What it catches |
|---|---|---|
| `on` | All LEDs on, indefinitely | Dead LED, wrong polarity (won't light) |
| `off` | All LEDs off, indefinitely | LED stuck on (short to rail) |
| `blinking` | All LEDs blink in sync, ~2 Hz | Cross-checks the `on`/`off` paths in one mode |
| `chase-on` | All off, one at a time walks the set | Swapped neighbours (wrong one lights) |
| `chase-off` | All on, one at a time goes dark | Bridged neighbours (two go dark together) |
| `cycle-all` | Walks every mode above back-to-back | Single keystroke, full sweep |

`cycle-all` names the currently active sub-mode on the status line
so a builder watching the LEDs knows what they should be seeing.

Single-key bindings (final set in TASK-387; draft values from
IDEA-062):

| Key | Action |
|---|---|
| `o` | group `on` |
| `f` | group `off` |
| `b` | group `blinking` |
| `c` | group `chase-on` |
| `C` | group `chase-off` |
| `a` | group `cycle-all` |

The keystroke set above is provisional — TASK-387 owns the final
binding table. What matters here is the *behaviour* is right; the
bindings can shift one keystroke without rewriting the mode logic.

LED iteration order for chase modes: stable across runs, derived
from the hardware config's declaration order (`ledPower`,
`ledBluetooth`, then `ledSelect[0..N]`). Document the order in the
status display so a builder watching the chase knows which LED
should light next.

## Acceptance Criteria

- [ ] All six group modes work correctly against the example
      hardware config: `on`, `off`, `blinking`, `chase-on`,
      `chase-off`, `cycle-all`.
- [ ] `blinking` runs at approximately 2 Hz (within ±20% — a
      stop-watch test or scope confirms; this is a builder-facing
      tool, exact period is not load-bearing).
- [ ] Chase modes use a stable iteration order documented in the
      status display.
- [ ] `cycle-all` names the currently active sub-mode on the status
      line; the active mode advances on a known cadence (e.g. 3-5 s
      per sub-mode).
- [ ] Switching between modes does not leave LEDs in a stale state
      — e.g. switching from `chase-off` to `on` does not leave the
      "currently dark" LED dark.
- [ ] Status display reprints on every mode change and includes the
      current group-mode name.

## Test Plan

**Host tests** (`make test-host`):

- The mode-state machine is pure (mode + tick → LED states). Split
  it out and add host tests covering: each mode produces the right
  LED bitmask over time; transitions between modes clear stale
  state; `cycle-all` advances sub-modes on schedule.
- Register new sources in `test/CMakeLists.txt`.

**On-device verification** (manual):

- Flash the wiring tool against the example config and step through
  each mode visually. Confirm chase order matches the documented
  order, blink rate looks right, no LED gets stuck.
- Soldering-failure simulation: with the test bench, deliberately
  disconnect one LED's anode and run `chase-on` / `cycle-all` —
  the missing LED should be visibly missing from the sweep, which
  is the diagnostic the tool exists to deliver.

## Prerequisites

- **TASK-383** — provides the env, Makefile, embedded-config
  plumbing, status-display foundation, and keystroke I/O loop this
  task plugs into.

## Notes

- **Pure mode state machine.** The mode behaviour depends only on
  the current mode, the configured LED set, and a tick counter.
  Keeping it pure makes it host-testable and keeps the embedded
  entry point thin.
- **No PWM, no fade.** Binary on/off only. PWM brightness is not a
  wiring-correctness question; if a builder wants to verify PWM
  later, that is a different tool.
- **Cycle-all cadence.** Aim for ~3 seconds per sub-mode. Faster
  than that and a builder cannot read the active-mode name fast
  enough; much slower and the full cycle feels tedious. Tune in
  the on-device pass.
- **Iteration order matters for diagnostics.** A builder watching
  `chase-on` is checking "the LED at position N+1 lights right
  after the LED at position N." If the order is non-deterministic
  or undocumented, the diagnostic value collapses. Pin it to the
  hardware-config declaration order and *show* the order in the
  status block.
- **Out of scope.** Individual-LED mode (selection by pin /
  `prev`/`next`, `all-toggle`) is TASK-386. Final keystroke
  bindings and the `?` legend are TASK-387.
