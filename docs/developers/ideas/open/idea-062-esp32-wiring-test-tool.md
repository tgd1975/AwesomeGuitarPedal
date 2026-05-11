---
id: IDEA-062
title: Interactive ESP32 wiring/solder test tool
description: Interactive `make test-esp32-wiring CONFIG=...` sanity check for buttons and LEDs after soldering — uses hardware config only, profile-free.
category: 🛠️ tooling
---

`make test-esp32-wiring CONFIG=<hardware-config.json>` flashes a small interactive firmware to the ESP32 and drives a serial-console UI that lets the builder verify, button by button and LED by LED, that a freshly soldered board is electrically sound — every button reaches firmware, every LED lights, no shorts to neighbouring pins, no dull joints.

## Motivation

After soldering a new board (e.g. the 5-pedal layout) the first question is always "did I get the wiring right?" — long before any profile or app behaviour matters. Today we answer it indirectly by booting the real firmware and trying actions through the app, which conflates **electrical** correctness (is the pin connected? is the LED forward-biased? did I bridge two pads?) with **logical** correctness (does the profile fire the right action?). The two failure modes look different in the firmware logs but identical in the app.

This tool isolates the electrical layer. It loads only the **hardware configuration** (pin map for buttons and LEDs) — profiles, actions, BLE, HID are all out of scope. The builder gets a deterministic, fast, hands-on way to validate the board before they start configuring it.

## Scope

### Makefile entry point

- New target: `make test-esp32-wiring CONFIG=config-5pedal-board.json`.
- The hardware config (pin list) is a required argument; the target errors out cleanly if it is missing.
- The profile config is **not** consumed — even if the config file contains a profile block, the wiring tool ignores it.

### Buttons

- For every configured button: on a press event, emit a single serial log line identifying the button (logical name from the hardware config, GPIO pin, active-high / active-low polarity, current debounced state).
- No setup required — the operator just presses each button in turn and confirms a line appears.
- A short summary mode (`s` key, TBD) prints the press counter for every configured button so the operator can confirm coverage at a glance.

### LEDs — group modes (operate on all configured LEDs)

- `on` — all on
- `off` — all off
- `blinking` — all blink in sync (~2 Hz)
- `chase-on` — all off; one LED at a time walks through the set (only one on at a time)
- `chase-off` — all on; one LED at a time walks through the set (only one off at a time)
- `cycle-all` — runs every mode above back-to-back in a continuous loop, with the active mode named on the status line

### LEDs — individual mode

- Select one LED of interest, either by:
  - entering its pin id, or
  - stepping with `prev` / `next` through the configured LEDs
- Operations on the selected LED:
  - `on` / `off` — turn the selected LED on or off
- Operations on the rest:
  - `all-on` / `all-off` — turn the rest on or off
- Combined:
  - `all-toggle` — the selected LED has the **inverted state** of all others. Makes a swapped pair visually obvious: if the LED you intend to control isn't the one that inverted, the wiring is swapped.

### Keyboard bindings (final set TBD)

Single-key commands over the serial console — e.g.:

| Key | Action |
|---|---|
| `p` | prev LED (individual mode) |
| `n` | next LED (individual mode) |
| `o` | selected LED on / group `on` |
| `f` | selected LED off / group `off` |
| `b` | group `blinking` |
| `c` | group `chase-on` |
| `C` | group `chase-off` |
| `a` | group `cycle-all` |
| `t` | `all-toggle` |
| `m` | switch between group mode and individual mode |
| `?` | print help / legend |
| `q` | quit (return to bootloader / reset) |

The exact binding set will be finalised during implementation; the principle is **single-key, no enter required**, with a printed legend always one keystroke away.

### Status display

Continuously re-rendered on each state change so the operator always has context:

- Loaded hardware config (filename + summary: N buttons, M LEDs)
- Configured button list (logical name → GPIO, polarity)
- Configured LED list (logical name → GPIO, active-high / low)
- Current mode (group / individual)
- In individual mode: currently selected LED (highlighted in the LED list)
- Active group mode name (e.g. "chase-on")
- Press counters per button (lightweight, so you can confirm "I pressed each one once")
- Legend of currently meaningful keys

## Decisions

- **PlatformIO env, not a standalone sketch.** A dedicated env (e.g. `esp32-wiring-test`) under `platformio.ini`, wrapped by the Makefile target. Reuses the existing toolchain, BSP, and GPIO HAL; we accept that the env pulls in unused stack (BLE, HID, profile parser) — keeping a second build path in sync is more pain than the dead code.
- **Rebuild per board is fine.** The config is delivered as a compile-time build flag (the simplest of the three options — `-D CONFIG_JSON=...` or embed-as-string). No LittleFS / SPIFFS runtime loader needed for this tool. This is a debugging session tool used rarely; a one-off recompile per board is cheap compared to the maintenance cost of a runtime config path.
- **All hardware targets, not ESP32-only.** The tool should exist for every supported MCU — `make test-esp32-wiring` and `make test-nrf52840-wiring` (and any future targets). The nRF52840 implementation remains blocked by the device-availability situation tracked in EPIC-025, so scope the *first* delivery to ESP32 and add the nRF52840 env once hardware is available again. The Makefile / structure should be set up from the start to accommodate per-target variants.
- **Line-oriented output, no ANSI / dashboard rendering.** Status info is reprinted as a block on every state change rather than redrawn in place. This is a soldering-debug tool used rarely — speed of bringup matters more than terminal polish, and line-oriented output works on every serial monitor (including `pio device monitor`) without setup.
- **`CONFIG=` is mandatory.** No auto-detect, no default fallback. The whole point of the tool is to validate a *specific* board's wiring — driving the wrong pins because we guessed at a default config is exactly the failure mode we're trying to avoid. Missing `CONFIG=` → the target errors out cleanly with a usage message.
- **Documentation lives in the builder section + in-tool help.** Two homes:
  1. A builder-facing page under `docs/builders/` (not under the test directory or developer docs) — this is the discoverable "what is this and when do I use it?" entry point. The audience is the person who just finished soldering, not a firmware developer.
  2. A printed `?` help screen inside the tool — the cheatsheet you reach for mid-test, with the current key bindings and mode legend.
  The test directory does **not** get its own README; the builder page is the canonical doc. Run `/doc-check` after creating the builder page to confirm persona placement.

## Non-goals

- This tool **does not** test profile mappings, action firing, BLE pairing, HID output, or any app-side behaviour. Those are the job of the main firmware and the existing test suites.
- It does not replace the automated hardware testing rig discussed in [IDEA-014](idea-014-automated-hardware-testing-rig.md) — that is for CI-grade, hands-off validation. This is the human-in-the-loop, post-soldering smoke check that every builder runs once per board.
