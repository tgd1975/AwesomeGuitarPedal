---
id: TASK-360
title: nRF52840 — verify functional parity with ESP32 across the codebase
status: paused
opened: 2026-05-01
effort: Medium (2-8h)
complexity: Medium
human-in-loop: Clarification
epic: nrf52840-blocked
order: 3
prerequisites: [TASK-358, TASK-359]
---

## Description

Sanity-check that the nRF52840 build does what the ESP32 build does — feature-by-feature, not just compile-clean. The asymmetry surfaced by [BLE_READBACK_IMPACT.md](../../BLE_READBACK_IMPACT.md) (no custom GATT service, no profile-index persistence, no automated BLE test directory) is the kind of thing that accumulates silently while one platform is the daily driver and the other isn't testable. Without a structured audit, "it builds" gets mistaken for "it works".

This task activates after TASK-358 lands the nRF52840 BLE readback bundle. By then the gap surface should be smaller, but exactly *which* gaps still exist is the audit's job to find — not to assume.

## What "parity" means here

The audit walks the ESP32 feature surface and confirms each item works equivalently on nRF52840. Items expected in the audit (extend the list as the codebase evolves):

- **BLE config write** — chunked-reassembly upload of `profiles.json` and `config.json` from the Flutter app; status notifications return.
- **BLE config readback** — option (a) round-trip equality (lands via TASK-358).
- **BLE firmware-version surface** — DIS (0x180A) Firmware Revision string equals `FIRMWARE_VERSION` (lands via TASK-358).
- **BLE active-profile notify** — single-byte payload, fires within one connection-interval of a profile switch (lands via TASK-358).
- **HID keyboard** — already shipping; sanity-check still works after the new GATT service was registered.
- **Profile manager** — switchProfile, setCurrentProfile, LED indication, SELECT-button cycling.
- **Profile-index persistence** — currently a no-op (`Nrf52840PedalApp::saveProfile`); decide whether TASK-358 fixed this and re-verify, or document the still-no-op behaviour.
- **Button input** — debouncing, ISR timing, long-press detection (if applicable on this platform — check the existing on-device tests).
- **LED output** — PWM, brightness, profile-indicator behaviour.
- **Filesystem** — InternalFS read/write of `profiles.json` and `config.json`.

This is not exhaustive. The audit's first pass is *deriving* the parity checklist from the ESP32 feature surface (`make help-esp32`, the `test/test_*_esp32/` directory listing, the Connected-Pedal page rows). The deliverable is the audit doc; the result is a punch list.

## Acceptance Criteria

- [ ] Audit document committed at `docs/developers/NRF52840_PARITY_AUDIT.md` (or appended to BLE_READBACK_IMPACT.md if convention prefers — decide at activation).
- [ ] Each ESP32 feature in the parity checklist has one of three verdicts on nRF52840:
      **OK** (verified working on-device), **GAP** (missing or broken; new follow-up task scaffolded with a `gap-from: TASK-360` reference), or **N/A** (intentionally not on this platform; one-line rationale).
- [ ] Profile-index persistence decision recorded (fix vs document the gap).
- [ ] Connected-Pedal page UI behaviour verified end-to-end against an nRF52840 pedal — Firmware row populated, Configuration row populated, Storage row populated or "—" with rationale, active-profile reflects in real time.
- [ ] Any GAP verdict spawns a new follow-up task (re-homed to its feature epic — by the time TASK-360 activates, TASK-359 should have removed the routing skill, so feature epics are the right destination).

## Test Plan

- **On-device tests** (nRF52840 — requires hardware): run every `make test-nrf52840-*` target that exists at activation time, plus any new `test/test_ble_*_nrf52840/` directories landed by TASK-358. All green is the floor; any red is a GAP.
- **Manual on-device** (nRF52840 — requires hardware): exercise each parity-checklist item via the Flutter app and a generic BLE explorer where useful. Capture the verdict per item in the audit doc.
- **Host tests**: not directly relevant — host coverage is platform-agnostic; this task is about platform-specific behaviour.

## Paused

- 2026-05-01: Paused — no Adafruit Feather nRF52840 device on hand. Resume with `/ts-task-active TASK-360` once TASK-358 has closed (the implementation must precede the audit) and a device is available.

## Notes

- Originally cross-cutting (no single feature epic owns "platform parity"); routed to EPIC-025 nrf52840-blocked because the work itself targets nRF52840 hardware. After TASK-359 removes the routing skill, this task closes within EPIC-025 and the epic itself can close once it's empty.
- The audit's value scales with the size of the parity gap. Today the gap is large (no custom GATT service, no profile persistence, no BLE test directory). After TASK-358 it should be small. If the audit finds nothing, that's a successful outcome — record it explicitly as "no GAPs found" and close.
- Test-infra side-effect: this audit is the first thing to systematically *run* the nRF52840 test directory landed by TASK-358. Treat broken tests as GAPs.
