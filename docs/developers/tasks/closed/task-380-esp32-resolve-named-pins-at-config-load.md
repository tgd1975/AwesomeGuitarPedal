---
id: TASK-380
title: ESP32 firmware — resolve named-pin references at config load
status: closed
closed: 2026-05-11
opened: 2026-05-11
effort: Medium (2-8h)
effort_actual: Medium (2-8h)
complexity: Senior
human-in-loop: No
epic: named-pins
order: 4
prerequisites: [TASK-378, TASK-379]
---

## Description

Make the ESP32 boot path consume the new `pinNames` mapping
(TASK-378) and the new profile pin-reference forms (TASK-379) so the
loaded profiles bind to physical pins correctly. Today the firmware
treats profile pin refs as physical pin IDs directly; after this task
a profile authored against `pedal_a` will fire when the builder
presses the pin that the hardware config maps `pedal_a` to.

Boot sequence after this task:

1. Load `data/config.json` (hardware config) from LittleFS.
2. Build a name → pin lookup table from the `pinNames` map (the
   inverse of the on-disk pin → name map; build it at load time, not
   per-event, because it is read on every press).
3. Load profiles.
4. For each pin reference in each profile:
   - **Direct form** → use the integer directly.
   - **Named form** → look up the name in the lookup table.
     - **Hit** → use the resolved pin.
     - **Miss** → log a warning naming the profile, the action, and
       the unresolved name; skip *that one action* and continue
       loading the rest. **Do not** abort profile loading or boot.
       The missing-mapping policy from IDEA-061 is "warning, not
       error" — a profile that references `button_d` on hardware
       that does not map `button_d` is acceptable when the builder
       only cares about `button_a`, `button_b`, `button_c`.

Diagnostics:

- A name → pin resolution miss is observable in the serial log with
  enough context to fix: which profile, which action slot, which
  unresolved name.
- A summary line at end of profile-load: "N profiles loaded, M
  actions, K unresolved name references."

## Acceptance Criteria

- [ ] ESP32 firmware builds a `pinNames`-derived lookup table at
      hardware-config load.
- [ ] Profile load resolves named pin refs against the lookup table;
      direct refs continue to work unchanged.
- [ ] Unresolved named refs produce a serial-log warning naming
      profile + action + name, and the surrounding profile continues
      to load.
- [ ] End-of-profile-load summary line is emitted with counts of
      profiles, actions, unresolved name refs.
- [ ] Existing profiles (all-direct, no `pinNames` in the hardware
      config) continue to function with zero behavioural change.

## Test Plan

**Host tests** (`make test-host`):

- Profile-loader unit tests covering: all-direct, all-named with
  full mapping, mixed, missing-mapping (warning emitted, profile
  still functional), one profile with a typo'd name (only the bad
  action drops, the rest still binds).
- Use the existing fake / shim infrastructure to drive the loader
  without flashing.

**On-device tests** (`make test-esp32-button`):

- Extend `test/test_buttons_esp32/test_main.cpp` with one scenario
  that loads a hardware config + profile pair containing named
  refs and asserts the right physical pin fires for the right
  named action. Requires ESP32 connected via USB.

## Prerequisites

- **TASK-378** — delivers the `pinNames` map on the hardware-config
  side that this task consumes.
- **TASK-379** — delivers the profile-side sum-type pin reference
  shape that this task resolves.

## Notes

- **nRF52840 deferral.** The nRF52840 boot path needs the same
  treatment; that is deferred per the `nrf5-task-routing` skill
  and EPIC-025 (TASK-360 parity audit will pick it up when the
  hardware is reachable again). Do **not** scaffold an nRF52840
  task here.
- **Warning, not error, on missing mapping.** This is load-bearing.
  Builders deliberately under-map their hardware config (mapping
  only the names they care about) — that case must not break
  boot or skip whole profiles, only the specific action that
  references the unmapped name.
- **Build the lookup table once.** Per-event reverse-walking the
  `pinNames` map is wasteful given how often actions fire. Build a
  small lookup at config-load and reuse it.
- **Profile-author error reporting matters.** A vague "unresolved
  pin name" is useless when ten profiles fail; an actionable
  "profile=blues_rig, action=action[3], name=pedal_d" tells the
  builder exactly where to look.
