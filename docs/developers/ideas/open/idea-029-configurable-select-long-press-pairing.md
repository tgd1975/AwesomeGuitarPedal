---
id: IDEA-029
title: Configurable SELECT Long-Press to Activate BLE Pairing Mode
category: firmware
description: Very-long-press on the SELECT button activates BLE pairing mode; the feature is opt-in via config.json so it can be disabled for testing and CI.
---

## Motivation

Today, BLE pairing/bonding on the pedal is always available whenever the
device is powered on. That is convenient for end users, but it also makes
**testing and CI workflows harder** — any stray BLE client in range can
connect or trigger pairing prompts during bring-up, on-device test runs,
and when the device sits on the bench next to a developer laptop.

This mirrors the situation we already have with the **`pairing_pin`** field
in [`data/config.json`](../../../../data/config.json): a security/UX knob
that needs to be toggleable for development vs. production. The same
reasoning applies here — pairing activation should be an explicit,
user-gated gesture, and the gesture itself should be configurable (or
disabled entirely) per build.

## Proposal

Introduce a **very-long-press on the SELECT button** (longer than the
existing long-press event, e.g. ~3–5 s) as the gesture that puts the
device into BLE pairing/advertising mode. While inactive, the device
either advertises under restricted conditions or not at all (to be
refined during implementation — see open questions below).

The feature must be **fully configurable via `config.json`**, so that:

- End users can enable it and tune the press duration.
- Developers and CI can **disable it entirely**, keeping the device in a
  predictable state during tests.
- The Configurator (web + mobile) exposes the same knobs so users do not
  have to hand-edit JSON.

### Proposed config shape

Two activation styles should be supported, selected via `trigger`:

**A) Very-long-press on the existing SELECT button** — no extra hardware,
reuses the wiring every pedal already has.

```json
{
  "ble": {
    "pairingActivation": {
      "enabled": true,
      "trigger": "select_very_long_press",
      "longPressMs": 3000,
      "timeoutMs": 60000
    }
  }
}
```

**B) Dedicated pairing button on its own GPIO** — preferred for builds
where SELECT is already heavily overloaded or where an enclosure has
room for a recessed "pair" button. Only the press matters; duration is
optional (default: short press triggers immediately).

```json
{
  "ble": {
    "pairingActivation": {
      "enabled": true,
      "trigger": "dedicated_button",
      "pairingButtonPin": 4,
      "longPressMs": 0,
      "timeoutMs": 60000
    }
  }
}
```

Fields:

| Field | Meaning |
|---|---|
| `enabled` | Master switch. When `false`, the activation gesture is ignored and pairing mode follows whatever the non-interactive default is. |
| `trigger` | Which gesture activates pairing. One of `select_very_long_press` or `dedicated_button`. Leave room for future values (e.g. button combos). |
| `longPressMs` | How long the trigger must be held to activate. For `select_very_long_press` this must be comfortably longer than the normal long-press event (see [IDEA-009 archived](../archived/idea-009-long-press-event.md)) to avoid collisions. For `dedicated_button`, `0` means "activate on press". |
| `pairingButtonPin` | GPIO number of the dedicated pairing button. Required when `trigger` is `dedicated_button`, ignored otherwise. Must not collide with `buttonSelect`, `buttonPins`, or any LED pin. |
| `timeoutMs` | Optional: how long the device stays in pairing mode before returning to the previous state. |

Defaults should be chosen so that **existing users see no behaviour change**
unless they opt in. For CI the corresponding `test/test_ble_config_esp32/data/config.json`
should set `enabled: false`.

## Why this helps testing

- On-device BLE tests (`test/test_ble_config_esp32/`) can run with
  pairing activation disabled — the advertising/bonding state becomes
  deterministic and does not depend on bench environment.
- Host unit tests remain unaffected (no BLE), but the config parser
  gains explicit coverage for the new block.
- Matches the pattern we already have with `pairing_pin` — one more
  knob a developer can turn off to get a quiet device.

## Scope of work

1. **Firmware**
   - Extend `PedalConfig` / `HardwareConfig` parsing to read the new
     `ble.pairingActivation` block. Missing block → safe defaults
     (decision: default `enabled: true` for end users, `false` in the
     test config).
   - Implement two trigger paths behind a common interface:
     - `select_very_long_press`: very-long-press detector on the
       existing SELECT button that fires only above `longPressMs` and
       does **not** consume the existing long-press event used by
       [IDEA-024 profile-independent actions](idea-024-profile-independent-actions.md)
       or other handlers.
     - `dedicated_button`: configure `pairingButtonPin` as an input
       with pull-up, debounce it, and fire on press (or after
       `longPressMs` if > 0).
   - Reject configs where `pairingButtonPin` collides with
     `buttonSelect`, any entry in `buttonPins`, or any LED pin —
     surface this via the existing config-validation path.
   - Wire the chosen trigger to the BLE config service so it toggles
     advertising/bonding state.
   - When `enabled` is `false`, skip detector setup entirely (no GPIO
     allocation for the dedicated pin).

2. **Configurator (web + mobile)**
   - Add a form group "BLE pairing activation" with: enabled toggle,
     trigger selector (`Very long press on SELECT` /
     `Dedicated button`), long-press duration, timeout.
   - When `Dedicated button` is selected, reveal a `pairingButtonPin`
     field with the same pin-picker UX as `buttonPins`, and validate
     against collisions with other assigned pins.
   - When `Very long press on SELECT` is selected, validate
     `longPressMs` against the existing long-press threshold (must be
     strictly greater).
   - Mirror in the mobile app's `hardware_config.dart`.

3. **Documentation**
   - Update [docs/developers/BLE_CONFIG_IMPLEMENTATION_NOTES.md](../../BLE_CONFIG_IMPLEMENTATION_NOTES.md)
     with the new state machine (Idle → LongPressArmed → PairingMode → Idle).
   - User-facing note in the Builder / Musician docs: "Hold SELECT for
     3 seconds to put the pedal into pairing mode."
   - Update [docs/developers/TESTING.md](../../TESTING.md) to explain
     how to disable pairing activation for CI.

4. **Tests**
   - **Host** (`test/unit/`): new `test_ble_pairing_activation.cpp`
     covering config parsing for both triggers (enabled/disabled,
     missing block, invalid values, pin collisions for
     `dedicated_button`) and the detector timing logic for both paths
     (via shim + simulated `millis()`).
   - **On-device** (`test/test_ble_config_esp32/`):
     - With the feature disabled in config, a very-long SELECT press
       does **not** change BLE state.
     - With `select_very_long_press` enabled, a long enough hold does.
     - With `dedicated_button` enabled (on a free test GPIO), pressing
       the configured pin activates pairing while SELECT alone does
       not.
   - Extend `test/fakes/hardware_config_fake.cpp` with the new fields,
     including `pairingButtonPin`.

## Open questions

- **What is the "resting" BLE state when `enabled: true` but the user
  has not activated pairing?** Options: (a) advertise continuously as
  today, (b) advertise but refuse new pairings, (c) do not advertise
  at all until activated. Needs a security review alongside TASK-236 /
  TASK-237.
- Should we expose a **visual indicator** (e.g. blink the Bluetooth LED
  pattern) while in active pairing mode? Likely yes — worth defining
  before firmware work starts.
- Interaction with profile switching: SELECT is already overloaded.
  Confirm the very-long-press threshold does not make the device feel
  unresponsive during normal profile changes — this is also one of the
  motivations for offering `dedicated_button` as an alternative trigger.
- For `dedicated_button`: should the pin default to "unset" (feature
  effectively unusable without an explicit assignment) or to a
  conventional default pin per hardware target? Leaning toward "unset"
  to avoid surprising users who don't wire a pair button.

## Related

- [IDEA-025 Configurable BLE Device Name](idea-025-configurable-ble-device-name.md)
  — same spirit: move hardcoded BLE behaviour into `config.json`.
- Existing `pairing_pin` field in `data/config.json`.
- Recent BLE security tasks `TASK-236` / `TASK-237` (see the task
  overview) — any change here should land after, or be coordinated
  with, those cleanups.
