# Hardware Configuration

## Pin assignments

GPIO pin assignments and build counts (number of profiles, select LEDs, and action buttons)
are set in `builder_config.h`, one file per hardware target:

```
lib/hardware/esp32/include/builder_config.h     ← edit this for ESP32
lib/hardware/nrf52840/include/builder_config.h  ← edit this for nRF52840
```

Open the file for your board, change the `CONF_*` values to match your wiring, and rebuild.
The file contains plain-English `static_assert` checks — if your configuration is invalid,
the build fails with a descriptive message telling you exactly what to fix.

For the default wiring table, see [BUILD_GUIDE.md](BUILD_GUIDE.md).

### `ledPower` — firmware-controlled status indicator

Despite the name, `ledPower` is the pin the firmware drives for visible
status signals: the boot-time hardware-mismatch halt, the DelayedAction
countdown, and the config-load-error fallback. It is **not** the pin that
indicates "the board is powered" — that role is covered by the dev board's
hardwired red power LED (always on whenever USB or Vin is supplied).

The default config maps `ledPower` to **GPIO 2** because every common
NodeMCU-style ESP32 dev board (DOIT DevKit V1, Joy-IT SBC-NodeMCU-ESP32,
HiLetgo, etc.) carries an onboard blue LED on that pin, wired LED-to-ground.
That arrangement gives builders a firmware-controlled visual channel
without any external wiring. The strapping-pin caveats in the next section
apply.

If a future build attaches an external panel "status" LED, point `ledPower`
at the corresponding GPIO (any non-strapping pin from the safe list below).

### Pin selection notes — ESP32 strapping pins

GPIOs **0, 2, 5, 12, and 15** are *strapping pins* on the ESP32: their voltage at the moment
of reset selects boot mode, flash voltage, and JTAG behavior. After reset they work like any
other GPIO, but anything wired to them must not pull them HIGH at reset, or flashing and
normal boot will break.

Rules of thumb when assigning pins:

- **GPIO 0** — do not reuse; it is the BOOT button.
- **GPIO 2** — most NodeMCU-style dev boards (including Joy-IT SBC-NodeMCU-ESP32) carry an
  onboard blue user LED here, wired LED-to-ground. That arrangement is safe and the pin is
  usable as a firmware-controlled status LED. **Never add an external pull-up** to GPIO 2 —
  it would force the pin HIGH at reset and put the chip into the rarely used SDIO download
  mode every time you tried to flash.
- **GPIO 12, 15** — avoid unless you have measured the flash-voltage and bootloader-logging
  consequences for your specific board.

For new pin assignments, prefer non-strapping pins: 4, 13, 14, 16–19, 21–23, 25–27, 32, 33.

## Profile configuration

Button mappings are stored in `data/profiles.json`. The file is deployed to the device's
LittleFS filesystem during upload.

### File structure

```json
{
  "_doc": "Key reference: docs/builders/KEY_REFERENCE.md",
  "profiles": [
    {
      "name": "Profile Name",
      "buttons": {
        "A": { "type": "ActionType", "name": "Button label", "value": "..." },
        "B": { "type": "ActionType", "name": "Button label", "value": "..." }
      }
    }
  ]
}
```

Buttons are identified by letter (A, B, C, … Z). The number of button entries per profile
should match the number of action buttons wired (`CONF_NUM_BUTTONS` in `builder_config.h`).
The default build uses 4 buttons (A–D); the firmware supports up to 26 (A–Z).

### Action type reference

| Type | Description | Required fields |
|------|-------------|-----------------|
| `SendStringAction` | Types a text string | `value` |
| `SendCharAction` | Sends a single key | `value` (key name) |
| `SendKeyAction` | Sends a USB HID key code | `value` (key name) |
| `SendMediaKeyAction` | Sends a media key | `value` (media key name) |
| `SerialOutputAction` | Writes to serial monitor | `value` |
| `DelayedAction` | Waits, then executes another action | `delayMs`, `action` |

For the full list of valid `value` strings per action type, see
[KEY_REFERENCE.md](KEY_REFERENCE.md).

### DelayedAction

`DelayedAction` runs another action after a delay. The `delayMs` field sets how long to wait in
milliseconds. While the countdown runs, the power LED blinks.

Example — the camera remote delayed shutter:

```json
{
  "type": "DelayedAction",
  "name": "Delayed Shutter",
  "delayMs": 3000,
  "action": { "type": "SendMediaKeyAction", "value": "KEY_VOLUME_UP" }
}
```

Press the button, set down the controller, and the action fires 3 seconds later.

## Profile-select LED encoding

The firmware picks an encoding mode at runtime based on how many profiles and select LEDs
you have wired:

**One-hot mode** (`numProfiles ≤ numSelectLeds`): one LED lights exclusively for each profile.
Profile 1 → LED 1, Profile 2 → LED 2, etc. Easiest to read at a glance.

**Binary mode** (`numProfiles > numSelectLeds`): the 1-based profile number is encoded in
binary across all select LEDs. LED 1 = bit 0 (LSB), LED 2 = bit 1, etc.

The maximum number of profiles your wiring can represent is `2^numSelectLeds − 1`:

| Select LEDs wired | Max profiles |
|-------------------|-------------|
| 1 | 1 |
| 2 | 3 |
| 3 | 7 |
| 4 | 15 |
| 5 | 31 |
| 6 | 63 |

If you set `CONF_NUM_PROFILES` higher than this ceiling, the build will fail with an error
message.

## Editing profiles

1. Edit `data/profiles.json`.
2. If you only changed the config file (no firmware changes): run a filesystem-only upload:

   ```bash
   pio run -e nodemcu-32s --target uploadfs
   ```

3. If you also changed firmware: run the full upload:

   ```bash
   make upload-esp32
   ```

## Fallback behaviour

If the JSON file is missing or invalid on startup, all LEDs blink 5 times and the factory default
configuration loads. Factory defaults are hardcoded in `lib/PedalLogic/src/pedal_config.cpp`.

## Hardware target maturity

| Target | Board | Status |
|--------|-------|--------|
| ESP32 | NodeMCU-32S | Deployed and tested |
| nRF52840 | Adafruit Feather nRF52840 | Implemented, not tested — use at own risk |

## Named pins — portable profile vocabulary

When you share a profile with another builder whose pedal is wired
differently, hard-coded GPIO numbers stop being meaningful. EPIC-029
adds an optional second way to reference pins in profiles: by a
**role name** drawn from a small, curated set.

### The two reference forms

A pin reference in a profile's `Pin*Action` can be either form, and
you can mix both forms inside one profile:

| Form | Example | When to use |
|------|---------|-------------|
| **Direct** | `"pin": 27` | One-off prototype pins, build-specific extras. Always works on your own pedal. |
| **Named** | `"pin": "button_a"` | Anything you want to share. Resolves to whatever physical pin your `pinNames` maps `button_a` to. |

A named reference becomes meaningful when your hardware config tells
the firmware what `button_a` *means on this build*. Without a
mapping, the firmware drops that action with a warning at boot — the
rest of the profile still loads, so a community profile that uses
names you have not mapped is not an error.

### Mapping a pin to a name

Add a `pinNames` block to `data/config.json` keyed by stringified GPIO
number:

```json
{
  ...
  "buttonPins": [13, 12, 27, 14],
  "pinNames": {
    "13": "button_a",
    "12": "button_b",
    "21": "button_select",
    "26": "led_bluetooth"
  }
}
```

The same UI lives in the configurator at
[`docs/tools/configuration-builder/`](../tools/configuration-builder/index.html):
the "Named pins" section renders one row per configured pin with
autocomplete from the v1 standard set.

### Authoring a profile that uses named pins

Inside a profile, a `Pin*Action`'s `"pin"` field accepts either a
number or one of the standard names:

```json
{
  "type": "PinHighAction",
  "pin": "button_a"
}
```

The Flutter app's action editor exposes the same Direct/Named toggle
with autocomplete; the editor renders the active hardware config's
mapping inline (e.g. "→ GPIO 13 on this build") so you do not have to
guess.

### What the warning means

If a profile references a name your hardware config does not map, the
firmware logs:

```
PinAction: unresolved named pin 'button_d' — action dropped
Config load summary: 3 profiles, 11 actions, 1 unresolved named pin refs
```

The app shows the same condition as a non-blocking banner before you
upload, listing each unresolved name. It is informational, not an
error — you may legitimately load a community profile that uses names
you have not mapped because your build does not have that role wired.

### v1 standard pin-name set

<!-- Hand-copied from data/pin-names.schema.json (TASK-377). When the
     enum changes, refresh this table. The schema file is the
     authoritative source. -->

| Category | Names | Role |
|----------|-------|------|
| Action buttons | `button_a` … `button_h` | Action button slots 1–8. The default 4-button build uses `button_a`–`button_d`; extended builds add `button_e`–`button_h`. |
| Profile control | `button_select` | Cycles the active profile. Maps to the firmware's `buttonSelect` pin. |
| Status LEDs | `led_power` | Firmware-controlled status LED (boot mismatch, DelayedAction countdown, config-load fallback). |
|  | `led_bluetooth` | BLE connection indicator. |
| Profile-select LEDs | `led_select_1` … `led_select_6` | Profile-select LED array (one-hot or binary encoding). Builds use a contiguous prefix; the max-6 ceiling matches the 2⁶−1 = 63 profile cap. |

Names are lowercase `snake_case`. Numeric suffixes use `_N`
(`led_select_1`, not `led_select1`) for one-based indexing.

### Proposing a new name

If your build needs a role that is not in the v1 set, open a GitHub
issue using the
[Propose a standard pin name](https://github.com/tgd1975/AwesomeStudioPedal/issues/new?template=named-pin-addition.md)
template. Required:

- Proposed name (lowercase `snake_case`)
- One-line role description
- Why an existing name does not fit
- A builder context where the role exists

The triage outcome is **Accept** (added to the v1 enum in a follow-up
PR) or **Decline** (closed with a one-line reason — usually an
overlap with an existing role). The intent is lightweight, fast
turnaround; this is the builder-scope friction the project
deliberately accepted in exchange for keeping the community
vocabulary curated.

## BLE pairing (optional)

By default the pedal advertises **no passkey** (BLE "Just Works" pairing,
matching consumer BT keyboards like the Apple Magic Keyboard or Logitech
MX Keys). Any BLE host in range can bond and write configuration.

This is the right default for a desk pedal: there is no display on the
hardware to show a passkey, and forcing passkey-entry without a display
locks Android/iOS hosts out of the bonding flow (the OS dialog has
nowhere to read the PIN from).

To opt **into** passkey-entry pairing, add a `pairing_pin` field to
`data/config.json` and re-flash the data partition:

```json
{
  …
  "buttonPins": [13, 12, 27, 14],
  "pairing_pin": 12345
}
```

- Value: integer in `0`–`999999` (a 6-digit BLE passkey, zero-padded
  as needed — `12345` is presented to the host as `012345`).
- Absent or `null` → Just Works (the new default).
- The host reads the PIN from this file during setup; the hardware
  itself never displays it.

Re-flash the data partition after editing:

```bash
pio run -e nodemcu-32s --target uploadfs
```

Implementation details (security posture, IO-cap matrix, why MITM is
disabled by default) are in
[../developers/BLE_CONFIG_IMPLEMENTATION_NOTES.md](../developers/BLE_CONFIG_IMPLEMENTATION_NOTES.md).
