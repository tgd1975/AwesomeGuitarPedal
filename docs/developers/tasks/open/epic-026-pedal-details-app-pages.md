---
id: EPIC-026
name: pedal-details-app-pages
title: Pedal-details app pages — firmware readback surfaces
status: open
opened: 2026-05-01
closed:
assigned:
branch: feature/config-extensions
---

Cover the three firmware-side BLE readback surfaces that drive the
"pedal details" rows on the Connected-Pedal page (and any future
diagnostic page that needs live firmware data):

1. **Firmware version** — read characteristic exposing the canonical
   `FIRMWARE_VERSION` string from `include/version.h` so the
   Connected-Pedal page Firmware row can render the live value
   instead of a placeholder. (TASK-354)
2. **Active configuration readback** — make the running config
   readable over BLE so the Connected-Pedal page Configuration and
   Storage rows can fill in real values instead of placeholders.
   (TASK-355)
3. **Active profile index notify** — read+notify char so the app can
   reflect SELECT-button profile switches in real time without
   re-reading the whole config. (TASK-356)

These three were carved out of [EPIC-023](epic-023-app-content-pages.md)
because they share a different cost profile from the rest of that
epic: each one touches firmware (BLE GATT layer), needs an on-device
verification cycle, and is gated on the same nRF52840 deferral
(TASK-358 picks up nRF52840 once hardware is available again).
EPIC-023 retains the pure client-side pages and the closed
foundational work; this epic owns the new firmware surfaces and
their app-side wiring.

## Scope

- ESP32 firmware: new READ / READ+NOTIFY characteristics under the
  existing Config service UUID range (`516515c5`, `516515c1` made
  readable, `516515c6`).
- App-side: extend `BleService` to read the new chars and
  Connected-Pedal page to render them.
- BLE protocol doc updates for each new characteristic.
- On-device verification via `make test-esp32-ble-config`
  (test/test_ble_config_esp32/runner.py extended with per-char
  scenarios).

## Out of scope

- nRF52840 surfaces — deferred to TASK-358 (nrf52840-blocked epic).
- Live-keystroke diagnostic page — that work landed under EPIC-023
  via the original TASK-338.

## Tasks

Tasks belonging to this epic reference it via `epic: pedal-details-app-pages` in their frontmatter.
