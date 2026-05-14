---
title: Wiring test tool
audience: builder
---

## What this is and when to use it

After soldering a new board (or rewiring an existing one), the very
first question you want answered is **"did I get the wiring right?"** —
long before the question of whether your profile fires the right
action.

The wiring test tool is a one-purpose firmware that flashes onto the
ESP32 and gives you an interactive serial UI for verifying, button by
button and LED by LED, that the board is *electrically* sound. It uses
**only** the hardware configuration. There is no profile loading, no
BLE, no HID, no app — just GPIO.

Run it once after soldering, before you configure anything else. If
the wiring tool is happy, you can move on with confidence; if it is
not, you know exactly which pin is the problem before you spend an
hour chasing it through the production firmware.

## How to invoke it

```bash
make test-esp32-wiring CONFIG=data/config.json
```

`CONFIG=` is **required**. Point it at the hardware configuration
JSON for the board you just soldered (the same shape as the
production `data/config.json`). The Make target reads the file at
build time, embeds its contents into the firmware, flashes the ESP32,
and prints a tip pointing at the serial monitor. Re-run with
`MONITOR=1` to tail the monitor automatically:

```bash
make test-esp32-wiring CONFIG=data/config.json MONITOR=1
```

Or open the monitor manually:

```bash
pio device monitor -e nodemcu-32s-wiring-test
```

The monitor is configured for single-key console mode (no echo,
output unfiltered) — keystrokes are sent as you type them, no Enter
required.

The Make target will fail cleanly *before* flashing if `CONFIG=` is
missing or the file does not exist. That is on purpose — driving the
wrong pins because the tool guessed at a default config is exactly
the failure mode the tool exists to catch.

## Recommended flow

You only run this tool when you are sitting in front of the board with
a multimeter or your eyes on the LEDs. The recommended flow is:

### 1. Boot and read the banner

When the firmware boots, the serial monitor prints the configured
buttons and LEDs. Read it once and confirm:

- Every button you wired is in the **Buttons** list, with the right
  GPIO and polarity.
- Every LED you wired is in the **LEDs** list, with the right GPIO and
  polarity.
- The chase iteration order matches the physical layout you expect to
  see walked across the board.

If a row is missing or the GPIO is wrong, your `CONFIG=` JSON does
not match the board — fix that first; the tool can only verify what
the JSON tells it about.

### 2. Verify every button (`s` for the count)

Press each configured button (action buttons A/B/C/… and the select
button) **once** each. Each accepted press emits a single line on the
serial monitor naming the button, GPIO, polarity, and current state.

When you have pressed every button, hit **`s`** for the summary
block. Every button's `count=` value should be **1**. Anything still
at 0 has a wiring problem — dead joint, wrong pin in the JSON, or
a bridged neighbour stealing the input.

### 3. Verify every LED (group modes)

Cycle through the group modes by typing the keystrokes. Each
keystroke also reprints the status block so you can see which mode
is active.

| Key | Mode | What you should see | Common failure it catches |
|---|---|---|---|
| `o` | on | every LED lit | dead LED, wrong polarity |
| `f` | off | every LED dark | LED stuck on (short to rail) |
| `b` | blinking | every LED blinks together at ~2 Hz | inconsistent on/off paths |
| `c` | chase-on | one LED at a time walks the row | swapped neighbours |
| `C` | chase-off | one LED at a time goes dark | bridged neighbours |
| `a` | cycle-all | walks every mode above on a 3 s cadence | one keystroke, full sweep |

The chase walks the LEDs in the **declaration order** shown at the
top of the status block (`ledPower`, `ledBluetooth`, then each
`ledSelect[i]`). If the LED that lights does not match the LED you
expected, the wiring of two LEDs is swapped — which the next step
makes visually obvious.

### 4. Investigate suspect LEDs (individual mode)

If group mode left you uncertain about a specific LED, switch to
**individual mode** with `m`. The status block now highlights the
selected LED with a `*` and shows each LED's individual state.

Move the selection with `n` (next) or `p` (previous), or jump
directly with `g` followed by a digit (the LED's index). Then drive
just that LED:

| Key | Effect |
|---|---|
| `o` | selected LED on |
| `f` | selected LED off |
| `O` | every *other* LED on (selected unchanged) |
| `F` | every *other* LED off (selected unchanged) |
| `t` | **all-toggle** — selected LED to opposite state of all others |

`t` is the headline diagnostic for swapped pairs. Press it: every
non-selected LED goes one state, the selected LED goes the opposite.
If you expected the second LED from the left to be the dark one, but
the third LED is dark instead, the wiring of those two LEDs is
swapped. Press `t` again and both flip — useful for confirming the
diagnosis.

When you are done in individual mode, press `m` again to return to
group mode. The previously configured group mode resumes cleanly
(no stuck LEDs from the individual operations).

## Reading the diagnostics

| What you see | What it usually means | Where to look |
|---|---|---|
| Button count stays 0 after pressing it | Switch is open / dead joint, wrong GPIO in JSON, or pull-up not pulling up | Reflow the joint; verify pin in `CONFIG=` JSON |
| Button polarity reads inverted (`PRESSED`/`RELEASED` flipped) | Solder bridged the switch to power instead of ground | Inspect the switch terminals against the schematic |
| LED stays dark in `o` (group on) | Anode/cathode swapped, dead LED, missing resistor | Rotate the LED 180° or check the resistor in series |
| LED stays lit in `f` (group off) | Short between LED pin and 3V3 | Look for solder splatter near the LED pad |
| `chase-on` skips an LED | That LED has the same failure as above (dead) | Use individual mode + `t` to confirm which one |
| Two LEDs walk in the wrong order during `chase-on` | Pin assignments crossed in the build | Use individual mode + `t` to identify the swapped pair |
| Two LEDs go dark together in `chase-off` | Solder bridge between adjacent LED pins | Re-flow both joints, retest |

## Always-available keys

| Key | Action |
|---|---|
| `s` | Reprint the status block (banner + button counts + LEDs + active mode) |
| `?` | Reprint the keystroke legend for the current mode |
| `q` | Reset the device (firmware reboots) |

## Key reference

The in-tool `?` legend is the canonical mid-test reference and
mirrors the table below.

### Group mode (default at boot)

| Key | Action |
|---|---|
| `o` | all LEDs on |
| `f` | all LEDs off |
| `b` | blink (~2 Hz) |
| `c` | chase-on |
| `C` | chase-off |
| `a` | cycle-all |
| `m` | switch to individual |

### Individual mode

| Key | Action |
|---|---|
| `m` | switch to group |
| `n` / `p` | next / previous LED |
| `g`*digit* | go to LED by index (0..numLeds−1) |
| `o` / `f` | selected LED on / off |
| `O` / `F` | every *non-selected* LED on / off |
| `t` | all-toggle (selected = NOT others; alternates each press) |

### Always available

| Key | Action |
|---|---|
| `s` | reprint status block |
| `?` | reprint keystroke legend |
| `q` | reset device |

## Out of scope

The wiring tool is intentionally narrow. It does **not** load profiles,
fire actions, pair over BLE, or speak HID — those are the production
firmware's job and have their own tests. The wiring tool answers one
question only: *"is the board electrically sound?"*

The nRF52840 variant is not yet shipped — when that hardware is
reachable again, an `feather-nrf52840-wiring-test` env will land
against the same firmware sources, and a `make test-nrf52840-wiring`
target will mirror the ESP32 invocation.
