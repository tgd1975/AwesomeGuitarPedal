---
id: IDEA-025
title: Configurable BLE Device Name
category: firmware
description: Allow users to configure the BLE device name via config.json
---

## Archive Reason

2026-04-29 — Converted to EPIC-020 (TASK-310, TASK-311, TASK-312). Auto-generated fallback rule revised to `ASP_{numButtons}_{numProfiles}` (no NVS counter).

## Postmortem — abandoned 2026-04-29

All three tasks (TASK-310 firmware, TASK-311 web config builder,
TASK-312 Flutter app) were closed without implementation on the same
day they were activated.

**Why it didn't work.** The BLE keyboard adapter on ESP32
(`HookableBleKeyboard`, `src/esp32/src/ble_keyboard_adapter.cpp`) is
declared as a `static` at file scope and constructed during the C++
static-initialisation phase — *before* `setup()` runs and therefore
before LittleFS is mounted by the Arduino runtime.

Three approaches were considered and ruled out:

1. **Read `config.json` directly during static construction.** Reading
   LittleFS before `setup()` is undefined behaviour on ESP32 Arduino
   — the runtime `init()` sequence has not yet run, so the underlying
   filesystem subsystem is not initialised. Confirmed in upstream
   issues (`espressif/arduino-esp32` and `maxint-rd/TM16xx#16`).
2. **Cache the desired name in a separate file (`ble_name.txt`) read at
   construction time.** Same root problem — the read itself triggers
   the unsafe LittleFS init. The cache file does not escape the
   initialisation-order constraint.
3. **Restructure `BlePedalApp` so the adapter is created in
   `setupCommon()` after `loadHardwareConfig()` runs.** This works:
   give `BlePedalApp` a virtual `createBleAdapter()` hook called from
   `setupCommon()` after FS init, and remove the eager adapter
   parameter from its constructor. Tractable but a meaningful refactor
   touching `BlePedalApp`, `Esp32PedalApp`, `Nrf52840PedalApp`, plus
   any test that constructs them.

**Decision.** The user-visible value is cosmetic — letting the user
pick what string shows up in their phone's BLE scan list. The
restructure under (3) is real engineering work to deliver that
cosmetic. Not worth it. The advertised name stays hard-coded
(`AwesomeStudioPedal` on ESP32, `Strix-Pedal` on nRF52840).

**Reopen criteria.** If a future change needs to defer BLE-adapter
construction for an unrelated reason — e.g. choosing a different
adapter type at runtime, or driver-level config that *does* need to be
runtime-tunable — the restructure under (3) becomes free, and at that
point reviving this idea costs only the JSON-parsing and UI work
originally scoped in TASK-310/311/312.

## Details

Currently, the BLE device name is hardcoded or not configurable. This idea proposes to make the BLE device name configurable through the `config.json` file. The vendor prefix should be hardcoded as "AwesomeStudioPedal" to maintain brand consistency, while the device name should be user-configurable.

### Proposed Implementation

1. **Vendor Prefix**: Hardcode the vendor prefix as "AwesomeStudioPedal" (or "ASP" for short).

2. **Device Name**: Add a new field in `config.json` to allow users to set a custom device name. For example:

   ```json
   {
     "ble": {
       "deviceName": "MyCustomPedal"
     }
   }
   ```

3. **Auto-Generation**: If the `deviceName` field is left empty or not provided, auto-generate the device name using the rule `ASP_{numberOfPedals}`. The `numberOfPedals` should be a unique identifier or counter to distinguish multiple pedals in the same environment.

4. **Full Device Name**: The full BLE device name should be constructed as follows:
   - If `deviceName` is provided: `AwesomeStudioPedal_{deviceName}`
   - If `deviceName` is not provided: `ASP_{numberOfPedals}`

### Example

- **Custom Name**: If the user sets `deviceName` to "MyCustomPedal", the BLE device name will be "AwesomeStudioPedal_MyCustomPedal".
- **Auto-Generated Name**: If the user does not provide a `deviceName`, the BLE device name will be "ASP_1", "ASP_2", etc., depending on the number of pedals.

### Benefits

- **Brand Consistency**: The hardcoded vendor prefix ensures that all pedals are easily identifiable as part of the AwesomeStudioPedal ecosystem.
- **User Customization**: Users can personalize their pedals with custom names, making it easier to identify their devices.
- **Auto-Generation**: The auto-generation feature ensures that even if users do not provide a custom name, their pedals will still have unique and identifiable names.

### Implementation Steps

1. Update the `config.schema.json` to include the new `ble.deviceName` field.
2. Modify the BLE initialization code to read the `deviceName` from `config.json`.
3. Implement the logic to construct the full BLE device name based on the presence or absence of the `deviceName` field.
4. Ensure that the auto-generated `numberOfPedals` is unique and persistent across reboots.
5. Update the configuration configurator (web and mobile apps) to include a field for setting the BLE device name.
6. Test the implementation to ensure that the BLE device name is correctly set and displayed in BLE scanners.
