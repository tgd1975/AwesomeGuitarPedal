# BLE Config Service — Implementation Notes (moved)

The contents of this file have been folded into the *Conventions* and *Gotchas* sections of [BLE_SERVICES_GUIDE.md](BLE_SERVICES_GUIDE.md) as part of [EPIC-027](tasks/active/epic-027-ble-services-developer-guide.md) / [TASK-367](tasks/closed/task-367-draft-conventions-invariants-gotchas.md).

This stub is kept (rather than `git rm`-ed) so existing inbound references continue to resolve without modifying firmware comments:

- [src/esp32/include/ble_keyboard_adapter.h:28,51](../../src/esp32/include/ble_keyboard_adapter.h)
- Archive task files: [TASK-228](tasks/archive/v0.4.0/task-228-defect-adv-override-leaks-to-production.md), [TASK-229](tasks/archive/v0.4.0/task-229-defect-ble-pairing-policy-mitm.md), [TASK-250](tasks/archive/v0.4.0/task-250-defect-android-manifest-missing-ble-permissions.md), [TASK-320](tasks/archive/v0.5.0/task-320-remove-ble-flag-precommit-check.md), [TASK-353](tasks/archive/v0.5.0/task-353-feasibility-firmware-ble-readback-surfaces.md)

Where to look now:

| You came here for | Read this section instead |
|---|---|
| Stack choice (NimBLE vs classic) | [BLE_SERVICES_GUIDE.md → Conventions → Stack choice](BLE_SERVICES_GUIDE.md#stack-choice) |
| GATT registration timing (Challenge 1) | [BLE_SERVICES_GUIDE.md → Gotchas → ESP32 NimBLE GATT registration timing](BLE_SERVICES_GUIDE.md#esp32-nimble-gatt-registration-timing) |
| Multiple-inheritance return-type clash (Challenge 2) | [BLE_SERVICES_GUIDE.md → Gotchas → BleKeyboard adapter quirks](BLE_SERVICES_GUIDE.md#blekeyboard-adapter-quirks-multiple-inheritance-return-type-clash) |
| BlueZ HID daemon disconnect (Challenge 3) | [BLE_SERVICES_GUIDE.md → Gotchas → BlueZ HID daemon](BLE_SERVICES_GUIDE.md#bluez-hid-daemon-historical-resolved) |
| `WRITE \| WRITE_NR` rule (Challenge 4) | [BLE_SERVICES_GUIDE.md → Conventions → READ vs WRITE vs NOTIFY](BLE_SERVICES_GUIDE.md#read-vs-write-vs-notify-decision-rule) |
| BlueZ GATT cache (Challenge 5) | [BLE_SERVICES_GUIDE.md → Gotchas → BlueZ GATT cache stale entries](BLE_SERVICES_GUIDE.md#bluez-gatt-cache-stale-entries) |
| Serial-line race / READY / PROFILE? / persistence (Challenges 6–9) | [BLE_SERVICES_GUIDE.md → Gotchas](BLE_SERVICES_GUIDE.md#gotchas) (test-runner subsections) |
| `pairing_pin` semantics | [BLE_SERVICES_GUIDE.md → Conventions → Security model](BLE_SERVICES_GUIDE.md#security-model) |
| Architecture summary | [BLE_SERVICES_GUIDE.md → Conventions → Architecture summary](BLE_SERVICES_GUIDE.md#architecture-summary) |
