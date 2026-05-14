---
id: TASK-384
title: Wiring test tool — button press logging, counters, and status display
status: active
opened: 2026-05-11
effort: Small (<2h)
complexity: Medium
human-in-loop: No
epic: wiring-test-tool
order: 2
prerequisites: [TASK-383]
---

## Description

Make the wiring tool useful for the *button* half of the electrical
check: every configured button (action and select) is verified by a
visible serial line on press, and a coverage summary tells the
operator "yes, you pressed all four."

Three pieces:

1. **Press logging.** For each configured button (logical name +
   GPIO from the embedded hardware config), on every accepted press
   emit a single serial line containing: logical name, GPIO pin,
   active polarity (high/low), current debounced state. One line
   per press — no multi-line block, no ANSI redraw. The wiring tool
   is line-oriented (per IDEA-062's decision against terminal
   rendering).
2. **Coverage summary.** A single keystroke (default `s`, may be
   revised in TASK-387) prints a one-block summary showing the
   press counter for every configured button. The operator presses
   each button once, hits `s`, and either sees a 1 against every
   row (board is electrically sound for buttons) or sees a 0 (which
   button silently failed?).
3. **Status display foundation.** Reprint the banner block from
   TASK-383 on every state change, with the press counters merged
   into the button list section. This is the foundation other LED
   tasks (TASK-385, TASK-386) will extend — get the re-render shape
   right here so the LED tasks add to it cleanly. Re-rendering means
   *reprinting the block*, not in-place ANSI updates (IDEA-062 decision).

## Acceptance Criteria

- [ ] Pressing any configured button (action button or select
      button) emits exactly one serial line naming the logical
      name, GPIO, polarity, and current state.
- [ ] The press log respects debouncing — chatter on a wobbly switch
      does not produce a flood of lines.
- [ ] `s` prints a summary showing press counters per configured
      button.
- [ ] Status block reprints on every state change, with current
      press counters visible.
- [ ] Pressing a *non-configured* pin (i.e. a pin not in
      `buttonPins[]` / `buttonSelect`) produces no output — the
      tool is checking the wiring of *configured* pins, nothing
      else.

## Test Plan

**Host tests** (`make test-host`):

- If the press-handling and summary-formatting logic can be split
  out of the embedded entry point (likely — the formatting is pure),
  add host tests covering: per-press line format, summary block
  format, counter increments under debounced and chattery inputs
  (use the Arduino shim's GPIO and `millis()` mocks).

**On-device verification** (manual, `/test-device esp32-wiring` if
the env tags allow):

- Flash with a known-good config, press each button once, run `s`,
  read the counters. Counters should be 1.
- Press one button rapidly 20 times, run `s`, counter should be
  20 (or thereabouts — depends on debounce window from EPIC-028 if
  that has landed).
- Press a known-disconnected pin (i.e. solder pad you intentionally
  did not wire) — no line should appear. This is the case the tool
  exists to detect on a real soldering failure.

## Prerequisites

- **TASK-383** — provides the env, Makefile, embedded-config
  plumbing, and banner this task builds on.

## Notes

- **Single line per press, deliberately.** Multi-line blocks turn
  into noise the moment more than one button is pressed. Each
  press is one log entry; the status block is the readable
  summary.
- **Polarity reporting matters.** A press registering with the
  "wrong" polarity (active-low when configured active-high) is a
  common solder failure. Surfacing it in the log makes the failure
  one read away rather than buried in firmware logs.
- **Coverage summary, not history.** Resetting counters is a
  follow-up — a builder running the tool reflashes for a new run
  if they want a clean slate. Do not over-engineer reset semantics
  here.
- **Debounce sensitivity.** If EPIC-028 has landed, the wiring
  tool naturally consumes the configured debounce window. If not,
  the existing 100 ms default applies. Either way, do not
  re-introduce a separate debounce constant inside the wiring tool.
- **Out of scope.** Anything LED-related — that is TASK-385
  (group modes) and TASK-386 (individual mode). Final key bindings
  and the `?` legend are TASK-387.
