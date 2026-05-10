---
id: TASK-358
title: nRF52840 BLE readback surfaces (firmware-version DIS + config readback + active-profile notify)
status: paused
opened: 2026-05-01
effort: Large (8-24h)
complexity: Senior
human-in-loop: Clarification
epic: nrf52840-blocked
order: 2
prerequisites: [TASK-353, TASK-354, TASK-355, TASK-356, TASK-359]
decision_doc: docs/developers/BLE_READBACK_IMPACT.md
---

## Description

Bring the three BLE readback surfaces shipped on ESP32 (TASK-354/355/356) to nRF52840. Bundled under one task because the dominant cost is **scaffolding a custom Bluefruit GATT service from scratch** — once that exists, the three small characteristics on top are roughly the same shape as the ESP32 ones.

This task is **gated on nRF52840 hardware being available** for on-device verification. Splitting it before then is wrong shape: the infra spike is the work, and the three surfaces are riders.

Today's nRF52840 BLE state (per [BLE_READBACK_IMPACT.md §1](../../BLE_READBACK_IMPACT.md#1-platform-asymmetry--the-elephant-in-the-room)):

- [BleKeyboardAdapter::begin()](../../../src/nrf52840/src/ble_keyboard_adapter.cpp) is HID-only via Bluefruit; no custom GATT service exists.
- [Nrf52840PedalApp::saveProfile()](../../../src/nrf52840/include/nrf52840_pedal_app.h) is a no-op — profile index does not persist across reboot, always boots to 0.
- No automated BLE test directory exists for nRF52840 ([test/test_ble_*_esp32/](../../../test/) are ESP32-only).

## What ships

Three readback characteristics under a custom Bluefruit GATT service mirroring the ESP32 service UUID family:

1. **Firmware-version surface** via standard **DIS (0x180A)** Device Information Service using Bluefruit's bundled `BLEDis` (Manufacturer, Model, Firmware Revision). Cheaper than a custom char on this stack and standard.
2. **Config readback** — option (a) per [BLE_READBACK_IMPACT.md §3.2](../../BLE_READBACK_IMPACT.md#32-config-readback--task-355): make the (yet-to-exist) config-write char readable, retain canonical JSON in RAM. Note: this presupposes config-**write** also exists on nRF52840 — that's part of this task's infra spike, since today nRF52840 has no way to *upload* a config either.
3. **Active-profile notify** — read+notify char with a single-byte payload; mirror of the ESP32 surface at UUID `…5c6`.

## Acceptance Criteria

- [ ] Custom Bluefruit GATT service registered alongside HID, advertising the same service UUID family `516515c0-…` as the ESP32 `BleConfigService`.
- [ ] Firmware-version readable via DIS (0x180A) on nRF52840 — verified with a generic BLE explorer (e.g. nRF Connect) and an automated test where feasible.
- [ ] Config write+read round-trip works on nRF52840 — write a config from the app, read it back, byte-equal at JSON level. (Config-write infra is in scope here because the readback path requires it.)
- [ ] Active-profile notify fires within one BLE connection-interval of a profile switch on nRF52840.
- [ ] App `BleService` works end-to-end against an nRF52840 pedal for the Connected-Pedal page Firmware / Configuration / Storage rows and the active-profile stream.
- [ ] nRF52840 RAM regression measured before/after; budget per [BLE_READBACK_IMPACT.md §2](../../BLE_READBACK_IMPACT.md#2-nrf52840-ramflash-headroom-measured) (≥ 200 KB free heap after option-(a) resident copy).
- [ ] Profile-index persistence on nRF52840 — decide explicitly whether to fix the no-op `saveProfile()` here or document the "always boots to 0" behaviour. Either path is acceptable as long as it's recorded in this task's Notes section.

## Prerequisites

- **TASK-353** — feasibility analysis; option (a) and DIS bundle decisions.
- **TASK-354** — ESP32 firmware-version char shipped; the app-side `BleService.readDeviceFirmwareVersion()` plumbing already exists, this task adds the nRF52840-side mechanism behind it.
- **TASK-355** — ESP32 config readback shipped via option (a); this task mirrors the resident-copy pattern on nRF52840.
- **TASK-356** — ESP32 active-profile notify shipped; this task mirrors the notify mechanics on nRF52840.

**Hard prerequisite outside the task system: nRF52840 hardware must be physically available** before this task is activated. Until then it stays open with no owner.

## Test Plan

**On-device tests** (nRF52840 — requires hardware):

- New directory `test/test_ble_config_nrf52840/` mirroring the structure of [test/test_ble_config_esp32/](../../../test/test_ble_config_esp32/). Cover: chunked-write reassembly, write→read round-trip, active-profile notify timing.
- Pairing and security model tests — extend or mirror [test/test_ble_pairing_esp32/](../../../test/test_ble_pairing_esp32/) for nRF52840.

**Host tests** (`make test-host`):

- Reassembler logic is already host-tested via `test/unit/test_ble_config_*` and is platform-agnostic; no new host tests required for the protocol layer.
- If the Bluefruit-side service registration logic is wrappable behind an `IBleService` shim (extending the existing [IBleKeyboard](../../../src/i_ble_keyboard.h) pattern), add host coverage for the registration sequence.

**App-side**:

- Existing widget tests already cover the Connected-Pedal page rows; no new widget test work expected unless the row rendering needs to differentiate beyond the existing platform-aware "—" fallback.

## Sizing rationale

Kept whole as Large/Senior (rather than split into one task per surface) because the dominant cost is the custom-Bluefruit-GATT-service spike — config-write infra + service registration timing + Bluefruit's API quirks. The three small chars on top are riders. Splitting them before that spike lands would force the spike's cost into whichever sub-task got activated first, then leave the others as artificially-cheap "add a char" tasks that don't reflect their real coupling.

If the spike runs longer than expected during activation, the task can be re-split *after* the infra lands — at that point each remaining char is genuinely independent.

## Paused

- 2026-05-01: Paused — no Adafruit Feather nRF52840 device is currently on hand. Every AC in this task requires on-device verification (BLE GATT registration, DIS round-trip, config write+read, active-profile notify timing), so execution is fully blocked on hardware acquisition. Resume with `/ts-task-active TASK-358` once a device is available **and** TASK-359 has closed (skill-removal must precede this task per the prerequisite chain).

## Notes

- Originally belongs to feature epic `app-content-pages` by scope; routed to EPIC-025 nrf52840-blocked while nRF52840 hardware is unavailable. Once [TASK-359](task-359-remove-nrf5-task-routing-skill.md) closes, this task can be re-homed to `app-content-pages`.
- Carved out of TASK-353's feasibility analysis. The original TASK-337 packaged ESP32+nRF52840 as flat per-deliverable cost; the [impact analysis](../../BLE_READBACK_IMPACT.md) found the asymmetry is dramatic, so the nRF52840 work is now its own bundle.
- Bluefruit's `BLEDis` is a known-good library; the firmware-version surface is the lowest-risk piece. Sequence the spike to land DIS first as a smoke-test before tackling config write/read.
- The protocol UUID family `516515cX-…` documented in [BLE_CONFIG_PROTOCOL.md](../../BLE_CONFIG_PROTOCOL.md#characteristics) is shared across both platforms. nRF52840 reuses ESP32's UUIDs so app-side discovery code stays platform-agnostic.
- Test-infra side-effect: this task creates the first `test/test_ble_*_nrf52840/` directory. Pattern follows the ESP32 BLE test layout.
