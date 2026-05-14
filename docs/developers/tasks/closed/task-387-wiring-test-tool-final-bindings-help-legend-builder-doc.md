---
id: TASK-387
title: Wiring test tool — final key bindings, ? help legend, builder doc, close epic
status: closed
closed: 2026-05-14
opened: 2026-05-11
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: wiring-test-tool
order: 5
prerequisites: [TASK-384, TASK-385, TASK-386]
---

## Description

Close out EPIC-030 by finalising the user-visible surface — the
keystroke table, the in-tool help screen, and the builder-facing
documentation — and verifying the whole tool against a real
soldered board.

Three pieces:

1. **Final keystroke bindings.** Bindings used in TASK-384/385/386
   were provisional. This task fixes them. Decision points:
   - Group-mode keys (`o`/`f`/`b`/`c`/`C`/`a`) — IDEA-062 draft is
     a reasonable starting point; review for collisions with
     individual-mode keys.
   - Individual-mode keys (`p`/`n`/`o`/`f`/`t`, plus chosen keys
     for `all-on`/`all-off`).
   - Mode-switch (`m`).
   - `?` for help, `q` for quit/reset.
   - Direct-pin selection (`g` for "go to", or `#`-prefixed
     numeric entry — pick one based on the TASK-386 implementation).
   The final table must have no two actions mapped to the same key
   in the same mode. Cross-mode key reuse (e.g. `o` means "selected
   on" in individual, "all on" in group) is acceptable and clearly
   documented.
2. **`?` help legend.** A single keystroke (`?`) prints a
   ready-to-read cheatsheet of all bindings for the *current* mode.
   The legend reprints in full on every `?` keypress — no paging,
   no toggling. It is the canonical reference a builder reaches
   for mid-test.
3. **Builder doc** under `docs/builders/`. Persona check via
   `/doc-check` before commit. Covers:
   - **What the tool is and when to use it.** "After soldering a
     new board, run this once before configuring profiles."
   - **How to invoke it.** `make test-esp32-wiring CONFIG=<path>`,
     with at least one worked example using the in-tree example
     config.
   - **The interactive flow.** Buttons first (press each one, run
     `s`, confirm the counts). Then group LED modes (visually
     confirm all LEDs respond). Then individual mode plus
     `all-toggle` for any pair that looks suspect.
   - **Reading the diagnostics.** Common failure patterns
     (dead LED, swapped pair, bridged neighbours, dull joint) and
     what they look like through this tool.
   - **Key reference.** Reproduced from the `?` legend — preferably
     generated rather than hand-copied, but a hand-copy with a
     "matches the in-tool `?` screen" note is acceptable for a
     small table.

End-of-task verification: a real soldered ESP32 board run through
the full flow, with at least one deliberate failure (e.g. one LED
unwired) caught by the tool. This closes the epic.

## Acceptance Criteria

- [ ] Final keystroke table is implemented, with no in-mode
      collisions, and is documented in the source (one place — the
      `?` legend or a single-source header).
- [ ] `?` prints the legend for the *current* mode (group or
      individual). The legend includes every binding the mode
      exposes and the mode-switch / quit keys.
- [ ] `q` cleanly exits or resets the device (whichever is more
      useful in practice — reset is usually simplest on ESP32).
- [ ] Builder doc exists under `docs/builders/` covering: when to
      use it, how to invoke it, the recommended flow, common
      failure patterns, and the key reference.
- [ ] `/doc-check` returns a Match verdict for the doc's persona
      placement.
- [ ] Real-board verification: a deliberately-broken solder joint
      (or unwired LED) is caught by the tool in one pass, and the
      pass is summarised in this task's close comment as evidence
      the epic's success criterion is met.

## Test Plan

No new automated tests required — the key-table and help-legend
logic is small and surface-level. Existing host tests from
TASK-385/386 cover the underlying state machine.

Manual verification (mandatory before closing the epic):

- Run the tool against the example config on a known-good board.
  Walk through buttons, group modes, individual mode, `?` legend,
  `q`. Read the legend; confirm every action listed actually does
  what it says.
- Deliberately disconnect one LED and rerun. Confirm:
  - `chase-on` skips the dark LED visibly.
  - `all-toggle` against that LED stays dark (or stays in whatever
    failure state the disconnection creates).
  - The status block continues to render correctly.

## Prerequisites

- **TASK-384** — buttons mode landed; key bindings finalised here
  may revise its `s` keystroke for the summary.
- **TASK-385** — group modes landed; key bindings finalised here
  may revise the provisional `o`/`f`/`b`/`c`/`C`/`a` keystrokes.
- **TASK-386** — individual mode landed; key bindings finalised
  here may revise the provisional `p`/`n`/`o`/`f`/`t`/`m`
  keystrokes and the direct-pin-selection UX.

## Notes

- **Cross-mode reuse is fine when it tracks intuition.** `o` for
  "on" works in both modes — in group mode it means "all on", in
  individual mode it means "selected on." Document the reuse in
  the `?` legend so a builder skimming the table understands what
  each key does in their current mode.
- **Don't reinvent help.** A static, ready-to-read legend is more
  useful than a paged help system. Builders running this tool are
  glancing at it once per board — a one-screen legend that prints
  every time `?` is pressed is the right shape.
- **The builder doc is the discoverability path.** The in-tool
  `?` legend is the cheatsheet mid-test; the builder doc is what a
  newcomer reads *before* they have flashed the tool. Make sure
  the builder doc is linkable from the soldering / first-board
  builder pages — a tool nobody finds is a tool nobody runs.
- **nRF52840 follow-up.** EPIC-030 closes with ESP32 only. The
  nRF52840 variant is deferred (EPIC-025). When that hardware is
  reachable again, a follow-up task can clone the env and the
  per-target HAL bits — most of the mode logic should be portable
  as-is.
- **Closing the epic.** After this task closes, run `/housekeep` so
  EPIC-030 moves to `closed/` automatically.
