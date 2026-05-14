---
id: TASK-386
title: Wiring test tool — LED individual mode (selection, on/off, all-toggle)
status: closed
closed: 2026-05-14
opened: 2026-05-11
effort: Medium (2-8h)
effort_actual: Medium (2-8h)
complexity: Medium
human-in-loop: No
epic: wiring-test-tool
order: 4
prerequisites: [TASK-385]
---

## Description

Add the *individual* half of the LED check: select one LED and drive
it independently of the others. This is where the most pointed
soldering diagnostics live — especially `all-toggle`, which makes a
swapped pair visually obvious.

Mode switch:

- A keystroke (default `m`, finalised in TASK-387) toggles between
  group mode (TASK-385) and individual mode (this task). The status
  display names the active mode at all times.

Selection:

- In individual mode, exactly one LED is *selected*. The selected
  LED is highlighted in the LED list in the status display.
- Selection moves with `p` (previous) and `n` (next), wrapping at
  the ends.
- Selection can also be set directly by typing the pin id of the
  desired LED — useful when the builder wants to jump to a specific
  LED without stepping past several others. (UX detail: typing a
  pin id requires either a numeric-entry mode or a "type the digits
  fast" recogniser; pick whichever is simpler given the single-key
  console — a numeric mode entered via a leading key like `#` is a
  common pattern.)

Operations on the selected LED:

| Key (provisional) | Action | Effect on selected | Effect on rest |
|---|---|---|---|
| `o` | on | on | unchanged |
| `f` | off | off | unchanged |
| (none) | `all-on` | unchanged | on |
| (none) | `all-off` | unchanged | off |
| `t` | `all-toggle` | inverted state of all others | all the same |

`all-on` and `all-off` from the table need single-key bindings —
TASK-387 will pick the final keys; for this task use placeholders
(e.g. uppercase `O` / `F`) and document them in the in-tool legend.

`all-toggle` is the diagnostic that earns the tool its keep:

- Press `t` — the selected LED is set to the *opposite* of all
  others. If the builder expects "the second LED from the left" to
  be the selected one and the *third* one inverts instead, the
  wiring of two LEDs is swapped. One keystroke, one obvious visual
  result.

## Acceptance Criteria

- [ ] Mode switch toggles cleanly between group mode (TASK-385) and
      individual mode. Status display names the active mode.
- [ ] In individual mode, the selected LED is highlighted in the
      LED list in the status display.
- [ ] `p` / `n` cycle the selection with wrap-around at both ends.
- [ ] Direct selection by pin id works for at least the `ledPower`,
      `ledBluetooth`, and any `ledSelect[]` LED.
- [ ] `on` / `off` affect only the selected LED; the rest stay in
      their last group-mode state (i.e. switching to individual mode
      does not implicitly reset all the other LEDs).
- [ ] `all-on` / `all-off` affect every non-selected LED.
- [ ] `all-toggle` puts the selected LED in the inverted state of
      all others.
- [ ] Switching back to group mode from individual mode resumes the
      last group mode cleanly (no stuck LEDs from the individual
      operations).

## Test Plan

**Host tests** (`make test-host`):

- Extend the mode state machine from TASK-385 to cover individual
  mode. Test: selection wraps with `p`/`n`; `on`/`off` affect only
  the selected LED; `all-on`/`all-off` affect the rest; `all-toggle`
  inverts the selected vs the others; mode-switch round-trips
  preserve last group mode.
- Register new sources in `test/CMakeLists.txt`.

**On-device verification** (manual):

- Flash the wiring tool, enter individual mode, step through the
  selection with `p`/`n` and watch the highlight track in the
  status display.
- Pick one LED, press `t` (all-toggle), and confirm the right LED
  inverted. Then deliberately swap two LEDs on the test bench,
  rerun `t`, and confirm the swap is visible (this is the failure-
  mode demo the tool's value rests on).

## Prerequisites

- **TASK-385** — provides the group-mode state machine and the LED
  iteration order that individual mode inherits for `p`/`n` cycling.

## Notes

- **All-toggle is the headline diagnostic.** The other operations
  are conveniences; `all-toggle` is what makes the tool worth
  flashing. Make sure it works first, document it clearly in the
  status block, and pick a keystroke that is unambiguous (TASK-387).
- **Selection persistence across mode toggles.** When the builder
  switches to group mode and back, restore the previously selected
  LED. Re-deriving "first in the list" each time is annoying when
  the builder is iterating on one specific LED.
- **Direct-pin-id selection UX.** Typing pin ids in a single-key
  console is a small UX trap. A `#` prefix entering a brief
  numeric mode (terminated by Enter or by timeout) is a known
  pattern. Alternative: a "jump to" sub-prompt entered via `g`
  ("go to"). Pick one and document it in the legend.
- **Out of scope.** Final keystroke bindings, `?` legend, and the
  builder-facing doc are TASK-387.
