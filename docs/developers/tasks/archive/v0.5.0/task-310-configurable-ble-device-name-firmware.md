---
id: TASK-310
title: Configurable BLE device name — firmware + schema
status: closed
opened: 2026-04-29
closed: 2026-04-29
effort: Medium (2-8h)
complexity: Medium
human-in-loop: Clarification
epic: config-driven-runtime-customisation
order: 5
---

## Resolution

**Won't do — closed without implementation (2026-04-29).**

The BLE keyboard adapter on ESP32 (`HookableBleKeyboard`) is constructed
during static initialization, before `setup()` runs and therefore before
LittleFS is mounted. To use a value from `config.json` at construction we
would need to either:

1. Read the filesystem during static construction — undefined behaviour
   on ESP32 Arduino (`init()` has not yet run, the LittleFS subsystem is
   not initialised).
2. Cache the desired name in a separate file read at construction time —
   same problem; the read itself triggers the unsafe `LittleFS.begin()`.
3. Restructure `BlePedalApp` so the adapter is created in `setupCommon()`
   after `loadHardwareConfig()` runs. This works but is a meaningful
   refactor for a cosmetic feature (the user-visible win is renaming the
   pedal in their phone's BLE scan list).

Decision: keep the advertised name hard-coded in the platform-specific
adapters (`AwesomeStudioPedal` on ESP32, `Strix-Pedal` on nRF52840). The
restructure under (3) is not worth the cost for the value delivered.

Sibling tasks TASK-311 (web config builder) and TASK-312 (Flutter app)
are also closed without implementation — there is nothing for them to
configure.

The seeding idea IDEA-025 is updated with the postmortem so future
readers see why this was tried and abandoned before reopening.

## Description

Make the advertised BLE device name configurable via `config.json`. Add an
optional `ble.deviceName` field. When present, the advertised name is
`AwesomeStudioPedal_{deviceName}`. When absent or empty, the firmware
auto-generates `ASP_{numButtons}_{numProfiles}` from the existing config
fields (e.g. `ASP_4_7` for the default config). No persistent counter,
no NVS plumbing — the fallback is purely derived from the loaded config.

This task covers the firmware change and both schemas. Web config builder
and Flutter app updates are tracked as separate sibling tasks under the
same epic.

## Acceptance Criteria

- [ ] `ConfigLoader` parses optional `ble.deviceName` and round-trips it through `saveToFile` and `mergeConfig`.
- [ ] BLE init advertises `AwesomeStudioPedal_{deviceName}` when the field is set and non-empty.
- [ ] BLE init advertises `ASP_{numButtons}_{numProfiles}` when the field is absent or empty.
- [ ] `data/config.schema.json` and `app/assets/config.schema.json` describe the new optional field.

## Test Plan

**Host tests** (`make test-host`):

- Add `test/unit/test_ble_device_name.cpp` (or extend the existing config test).
- Cover: explicit name produces `AwesomeStudioPedal_{name}`; missing key falls back to `ASP_{numButtons}_{numProfiles}`; empty string also falls back; sane behaviour for `numButtons`/`numProfiles` at edge values (0, large).

**On-device verification** (manual, not automated):

- Flash to ESP32 with default config, scan from a phone, confirm `ASP_4_7` shows up.
- Flash with `ble.deviceName: "MyPedal"`, confirm `AwesomeStudioPedal_MyPedal` shows up.

## Notes

- Length budget: BLE advertising name is limited (typically 29 bytes for the local-name AD field). Decide what to do if `AwesomeStudioPedal_{deviceName}` exceeds the limit — truncate, reject at config load, or warn? Resolve during implementation.
- The fallback uses `numButtons` and `numProfiles` from the same `config.json` — both already validated by the schema, so no extra error paths needed.

## Documentation

- `docs/builders/HARDWARE_CONFIG.md` — document the new `ble.deviceName` field and the auto-generated fallback rule.
