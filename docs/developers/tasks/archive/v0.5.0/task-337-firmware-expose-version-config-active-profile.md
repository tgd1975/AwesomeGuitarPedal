---
id: TASK-337
title: Firmware — expose firmware version, readable config, active-profile notify (BLE) [SPLIT]
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Large (8-24h)
effort_actual: XS (<30m)
complexity: Senior
human-in-loop: Clarification
epic: app-content-pages
order: 8
---

## Closed — split into TASK-353/354/355/356

This task bundled three independently-scopable firmware deliverables
plus a real design decision (config-readback option a vs b). At
activation time it was split so each piece can scope itself
honestly, the design decision lands in writing first, and the
small/cheap pieces aren't held hostage to the heavier ones:

- **[TASK-353](task-353-feasibility-firmware-ble-readback-surfaces.md)**
  — feasibility & impact analysis (gates the others). Captures the
  config-readback option (a vs b) decision, the nRF52840 RAM
  headroom number, and the DIS (0x180A) bundle/skip decision.
- **[TASK-354](task-354-firmware-version-read-characteristic.md)**
  — firmware-version read characteristic (+ DIS decision). Cheap,
  read-only, finishes the Connected-Pedal page Firmware row.
- **[TASK-355](task-355-firmware-config-readback.md)** — config
  readback (option chosen in TASK-353). Largest of the three, with
  the real design + risk surface.
- **[TASK-356](task-356-firmware-active-profile-notify.md)** —
  active-profile-index notify char. Independent infrastructure;
  may defer until a UI consumer exists.

The original task body is preserved below as historical context.

---

## Original description (preserved)

Original ordering placed all three deliverables in one Large/Senior
task. That packaging assumed they shared enough GATT-table plumbing
to ship together cheaply; the activation-time review found they
share only the registration boilerplate, while their per-deliverable
costs and risks differ by an order of magnitude.

## Description

Add the BLE surfaces the Connected-Pedal and Live-keystroke pages need.
Three deliverables, sized together because they share the same BLE
GATT-table edits and characteristic-registration plumbing:

1. **Firmware-version read characteristic** — short string like
   `"1.4.0+build42 abcdef0"`. New UUID alongside `kHwIdentityUuid`.
   Cheap, read-only, no security implications.
2. **Config readback** — pick one of two paths and document the
   choice in the task body before implementing:
   - (a) Make the existing config characteristic readable and return
         the canonical JSON the firmware is currently running with.
   - (b) Add a separate "current config" read characteristic that
         streams the active config back via the same chunked
         reassembler protocol used for uploads, just inverted.
   Option (a) is simpler but has a memory cost on nRF52840 worth
   measuring; option (b) avoids the resident copy at the cost of a
   second protocol path.
3. **Active-profile-index notify characteristic** — small read+notify
   characteristic so the app reflects profile switches in real time
   without re-reading the whole config.

Plus: evaluate exposing **Device Information Service (0x180A)**
(manufacturer name, model number, firmware revision string). Standard
GATT, picked up by generic BLE explorers — likely free given (1) is
already a firmware version string. Decide and either implement here or
spin a follow-up task.

Out of scope: Battery Service (0x180F) — only relevant once IDEA-012
(battery-powered variant) lands.

## Acceptance Criteria

- [ ] New firmware-version read characteristic exists and returns the
      canonical version string. UUID documented in
      `docs/developers/BLE_PROTOCOL.md` (or wherever current UUIDs
      live).
- [ ] Config readback path implemented per the chosen option (a or b),
      with a short rationale in this task's Notes section. App can
      read back exactly what was written for at least the example
      configs in `profiles/`.
- [ ] Active-profile notify characteristic emits a notification within
      one BLE connection-interval of a profile switch.
- [ ] Memory budget on nRF52840 verified — no characteristic
      registration regresses available RAM beyond a documented
      threshold.
- [ ] Device Information Service decision recorded (in or follow-up).

## Test Plan

**On-device tests** (`make test-esp32-ble` and the matching nRF52840
target — extend, don't duplicate):

- Add coverage for: firmware-version characteristic returns the
  expected string format; config-readback round-trip equals the most
  recently written config; active-profile notify fires on switch.
- Requires: ESP32 and nRF52840 connected via USB for the matching
  runs.

**Host tests** (`make test-host`) where logic permits:

- If the chunked-reassembler-inverted path (option b) is chosen, the
  reassembly logic itself is host-testable behind the existing
  `HOST_TEST_BUILD` shim. Add a `test/unit/test_config_readback.cpp`.

## Documentation

- `docs/developers/BLE_PROTOCOL.md` — register the new UUIDs and
  describe the config-readback protocol.
- `docs/developers/ARCHITECTURE.md` — update the BLE GATT table
  diagram if one exists.

## Notes

- The Connected-Pedal page (TASK-336) returns here in a small
  follow-up to wire the firmware / config / storage rows once these
  surfaces exist. That follow-up is not a new task — it's part of
  this task's "wire app side" closing step.
- Option-(a) vs option-(b) for config readback is the main design
  decision in this task. Make it explicit in the task body before
  implementation; do not start with one and silently switch.
