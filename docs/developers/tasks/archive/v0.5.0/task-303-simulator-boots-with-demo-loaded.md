---
id: TASK-303
title: Simulator boots with demo profiles loaded; community gallery still reachable
status: closed
closed: 2026-04-30
opened: 2026-04-28
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
---

## Description

Currently the simulator (`docs/simulator/index.html`) opens with "No config
loaded" and the user must click *Load Example* or *Browse community profiles*
before anything renders. This makes the very first impression a chooser/splash
screen rather than a working pedal.

Make the demo (`EXAMPLE_DATA` in `docs/simulator/simulator.js`) load
automatically on `DOMContentLoaded`, so the pedal renders with profile 1
selected and buttons A–D labelled on first paint. The *Browse community
profiles* button must continue to work after auto-load — clicking it replaces
the demo config in place. Same for the *Load profiles.json* and
*Load config.json* file inputs.

## Acceptance Criteria

- [ ] Opening `docs/simulator/index.html` renders the demo profiles
  immediately; profile 1 is selected, button A–D labels populated, no manual
  click required.
- [ ] *Browse community profiles* still opens the gallery and successfully
  replaces the demo with the chosen community profile (`config-name` updates
  accordingly).
- [ ] *Load profiles.json* and *Load config.json* file inputs continue to
  override the auto-loaded demo.

## Test Plan

No automated tests required — change is non-functional (browser-only static
page). Manual check:

1. Open `docs/simulator/index.html` in a browser; confirm demo profiles load
   on first paint without clicking anything.
2. Click *Browse community profiles*, pick one, confirm the demo is replaced
   and `config-name` updates.
3. Use *Load profiles.json* with a custom file; confirm it overrides the
   auto-loaded demo.

## Notes

Implementation hint — call `loadConfig(EXAMPLE_DATA)` at the end of the
`DOMContentLoaded` handler in [docs/simulator/simulator.js](../../../simulator/simulator.js)
(around line 557), and set `config-name` to the same
`"example (default) — load your own profiles.json to replace"` string the
*Load Example* button already uses (line 618). No new code paths needed.
