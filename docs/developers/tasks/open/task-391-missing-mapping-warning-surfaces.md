---
id: TASK-391
title: Missing-mapping warning surfaces — profile editor + connected-pedal page
status: open
opened: 2026-05-11
effort: Small (<2h)
complexity: Medium
human-in-loop: Support
epic: named-pins
order: 9
prerequisites: [TASK-378, TASK-379, TASK-389]
---

## Description

Split out of TASK-381 (configurator UX) for the warning-surface piece.
When a profile references a named pin that has no entry in the active
hardware config's `pinNames` map, the firmware silently drops the
action (TASK-380's load summary reports the count). The Flutter app
should surface this *before* upload so the builder can fix it, and
again on the connected-pedal page if a config is already on-device.

Two surfaces, same data:

1. **Profile editor / list**: loading a profile (file, BLE, community
   import) computes the unresolved-name set against the active
   hardware config. A non-blocking banner names each unresolved
   reference and offers a "fix the mapping" link.
2. **Connected-pedal page**: after the pedal reports its post-load
   summary (TASK-380), if `unresolvedNamedPinRefs_ > 0` the page
   shows a banner with the same wording.

The warning is **informational, never blocking**: a builder may
intentionally load a community profile that uses names they don't
need to map (e.g. `button_d` on a 3-button build).

## Acceptance Criteria

- [ ] A helper computes the set of unresolved named pin references
      given a profile JSON tree + a `HardwareConfig.pinNames` map.
- [ ] The profile editor / list displays a non-blocking banner listing
      each unresolved name when the helper returns a non-empty set.
- [ ] The banner has a dismiss control and a "fix the mapping" link
      that routes to the hardware-config editor (placeholder if that
      screen does not yet exist; the route is the AC, not the
      destination screen).
- [ ] Connected-pedal page mirrors the banner when the device's
      post-load summary reports unresolved refs.
- [ ] Unit tests cover the helper's behaviour: no references → empty
      set, all resolved → empty set, partial → the unresolved names,
      mixed direct + named — only the named are considered.

## Test Plan

**Host tests** (Dart, via `flutter test`):

- `app/test/unit/unresolved_named_pins_test.dart` — pure-function
  helper.
- Widget test alongside the profile list / editor exercising the
  banner appearance + dismiss.

## Prerequisites

- **TASK-378** — `HardwareConfig.pinNames` is the lookup source.
- **TASK-379** — `PinRef` sum type is what the helper iterates over.
- **TASK-389** — `PinNamesCatalog` is needed only for messaging /
  copywriting (the unresolved-set computation does not need the v1
  vocabulary itself, only the active mapping).

## Notes

- **Origin.** Carved out of TASK-381. The warning surface is small
  on its own and largely independent of the action-editor UX — it
  reads data that is already in memory after a profile is loaded.
- **Not a hard error.** Repeat the project's rule loud and clear in
  the banner copy: this is a warning, the profile still uploads.
- **Both surfaces, same helper.** The connected-pedal page and the
  editor share the unresolved-set computation. Place it in
  `app/lib/services/` so both screens import it.
