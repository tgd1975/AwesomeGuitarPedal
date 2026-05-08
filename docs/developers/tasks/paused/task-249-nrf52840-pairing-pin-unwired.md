---
id: TASK-249
title: nRF52840 pairing_pin is entirely unwired (security parity with ESP32)
opened: 2026-04-25
status: paused
effort: Medium (2-8h)
complexity: Medium
human-in-loop: Verify
epic: nrf52840-blocked
order: 4
prerequisites: [TASK-246]
---

## Description

[TASK-246](task-246-defect-pairing-pin-enforced-as-just-works.md) fixed the
ESP32 pairing path: when `pairing_pin` is set in `data/config.json`, NimBLE
now raises the MITM bit and enforces Passkey Entry against the configured PIN.

The nRF52840 adapter at
[lib/hardware/nrf52840/include/ble_keyboard_adapter.h](../../../../lib/hardware/nrf52840/include/ble_keyboard_adapter.h)
has the corresponding code path entirely unwired — `hardwareConfig.pairingPin`
is never read, and there is no equivalent of the ESP32's `onStarted` hook
where security flags would be set on the Bluefruit/SoftDevice stack. So on
nRF52840:

- `pairing_pin` in config.json is silently ignored.
- Anyone in range can bond with no PIN check.
- This is *no worse than the pre-TASK-246 ESP32 behaviour* but it is now a
  visible asymmetry between the two targets.

The nRF52840 target is currently labelled "Implemented, not tested — use at
own risk" in
[HARDWARE_CONFIG.md](../../builders/HARDWARE_CONFIG.md), so this isn't a
shipping-blocker — but if nRF52840 ever moves to "tested", this gap has to
close first.

## Acceptance Criteria

- [ ] nRF52840 adapter reads `hardwareConfig.pairingPin` and configures the
      Bluefruit/SoftDevice security surface so that pairing with `pairing_pin: N`
      requires the connecting host to enter `N` as a 6-digit passkey, and
      pairing with `pairing_pin: null` works Just-Works (matches ESP32 behaviour).
- [ ] Verified on real nRF52840 hardware via either `bluetoothctl` (same
      mechanism as `make test-esp32-ble-pairing` from TASK-247) or a
      documented manual procedure if no `make test-nrf52840-ble-pairing`
      ships.
- [ ] HARDWARE_CONFIG.md's nRF52840 maturity row updates if/when the rest of
      the nRF52840 path also moves out of "implemented, not tested".

## Test Plan

**Manual / scripted on hardware**:

- Pair with the right PIN (`012345`) → success, bond persists.
- Reconnect → no second prompt.
- Pair with wrong PIN → fails, no bond.
- Reflash with `pairing_pin: null` → no prompt, Just-Works pairing succeeds.

If `make test-esp32-ble-pairing`'s `bluetoothctl` machinery generalises
cleanly, port it to a nRF52840 variant. The pedal-side cleanup (NVS erase
equivalent) on nRF52840 will look different; check what Bluefruit uses for
its bond store.

## Notes

- Originally belongs to feature epic `feature_test` by scope; routed to EPIC-025 nrf52840-blocked while nRF52840 hardware is unavailable. Once [TASK-359](task-359-remove-nrf5-task-routing-skill.md) closes, this task can be re-homed to `feature_test`.
- ESP32 fix lives at
  [lib/hardware/esp32/include/ble_keyboard_adapter.h](../../../../lib/hardware/esp32/include/ble_keyboard_adapter.h)
  — read it for the two-posture pattern (MITM=true + DisplayOnly when
  `pairingEnabled`, MITM=false + NoInputNoOutput otherwise).
- Bluefruit's security primitives are different from NimBLE's; expect the
  function names to look unfamiliar even though the BLE-spec concepts are
  identical.

## Paused

- 2026-04-27: Paused — no nRF52840 board is currently on hand. Wiring up
  the Bluefruit/SoftDevice security surface and verifying parity with the
  ESP32 fix from TASK-246 both require the physical target. Resume with
  `/ts-task-active TASK-249` once an nRF52840 is available.
