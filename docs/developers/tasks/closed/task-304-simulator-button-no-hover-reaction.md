---
id: TASK-304
title: Simulator pedal buttons must not react to mouse hover
status: closed
closed: 2026-04-29
opened: 2026-04-28
effort: XS (<30m)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
---

## Description

In the web-based pedal simulator ([docs/simulator/](docs/simulator/)), the pedal
buttons currently change their appearance on mouse-over — see the `.pedal-btn:hover`
rule at [docs/simulator/styles.css:259-261](docs/simulator/styles.css#L259-L261).
This is misleading: a real pedal has no notion of "hovering" a footswitch, and the
visual feedback suggests interaction where none has occurred. The button should only
change appearance when actually pressed (mousedown / touchstart → `.held` /
`:active`), which is already wired up correctly in
[docs/simulator/simulator.js:190-194](docs/simulator/simulator.js#L190-L194).

The fix is to remove (or neutralize) the `:hover` rule so the button stays in its
resting style until pressed. The press/held visual at
[docs/simulator/styles.css:263-271](docs/simulator/styles.css#L263-L271) is correct
and should be left alone.

## Acceptance Criteria

- [ ] Hovering the mouse over a pedal button in the simulator produces no visual change
- [ ] Pressing a pedal button (mousedown or touchstart) still shows the existing held/active visual
- [ ] Releasing or moving the mouse off a pressed button still releases it (existing `mouseleave` → `handleButtonUp` behaviour preserved)

## Test Plan

No automated tests required — change is non-functional CSS in a static demo page.
Manual verification: open [docs/simulator/index.html](docs/simulator/index.html) in a
browser, hover over each pedal button (no visual change expected), then click and
hold (held visual expected).

## Notes

- Scope is intentionally CSS-only. Do not touch the JS event handlers — they
  already model mousedown / mouseup / mouseleave correctly.
- Touch devices are unaffected (no hover state).
