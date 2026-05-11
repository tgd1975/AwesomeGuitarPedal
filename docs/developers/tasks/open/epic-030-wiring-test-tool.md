---
id: EPIC-030
name: wiring-test-tool
title: Interactive wiring/solder test tool
status: open
opened: 2026-05-11
closed:
assigned:
branch: feature/wiring-test-tool
---

Seeded by IDEA-062 (Interactive ESP32 wiring/solder test tool).

After soldering a new board (e.g. the 5-pedal layout) the first
question is always "did I get the wiring right?" — long before any
profile or app behaviour matters. Today we answer that indirectly by
booting the full firmware and trying actions through the app, which
conflates **electrical** correctness (is the pin connected? is the
LED forward-biased? did I bridge two pads?) with **logical**
correctness (does the profile fire the right action?). The two failure
modes look different in the firmware logs but identical in the app.

This epic delivers a builder-facing tool — `make test-esp32-wiring
CONFIG=<hardware-config.json>` — that flashes a small interactive
firmware to the ESP32 and drives a serial-console UI for verifying,
button by button and LED by LED, that a freshly soldered board is
electrically sound. It uses **only** the hardware configuration. No
profiles, no actions, no BLE, no HID.

## Scope

- A dedicated PlatformIO env (e.g. `esp32-wiring-test`) wrapped by a
  Makefile target. Reuses the existing toolchain, BSP, and GPIO HAL.
- Compile-time config delivery: the hardware config JSON is embedded
  via a build flag (`-D CONFIG_JSON=...` or equivalent). One recompile
  per board — no LittleFS runtime loader.
- Required `CONFIG=` argument. Missing → the target errors out cleanly
  with a usage message. No auto-detect, no default — driving the wrong
  pins because we guessed is exactly the failure mode the tool exists
  to catch.
- Button mode: each press emits a single serial line (logical name,
  GPIO, polarity, debounced state). `s` prints a coverage summary
  (press counter per configured button).
- LED group modes: `on`, `off`, `blinking` (~2 Hz), `chase-on`,
  `chase-off`, `cycle-all` (continuous walk through all the above
  with the active mode named on the status line).
- LED individual mode: select one LED by pin ID or `prev` / `next`;
  operations on the selected LED (`on` / `off`), on the rest
  (`all-on` / `all-off`), and `all-toggle` (selected LED holds the
  inverted state of all others — makes a swapped pair visually
  obvious).
- Single-key keyboard bindings over the serial console (no enter
  required) with a `?` legend always one keystroke away.
- Status display reprinted as a line-oriented block on each state
  change (no ANSI / no dashboard rendering — works on every serial
  monitor including `pio device monitor` without setup).
- Builder-facing documentation page under `docs/builders/` (the
  discoverable "what is this and when do I use it?" entry point —
  *not* a developer doc, *not* a README in `test/`). The in-tool `?`
  screen is the cheatsheet you reach for mid-test.

## Out of scope

- **Profile mappings, action firing, BLE pairing, HID output, app-side
  behaviour.** Those belong to the main firmware and existing suites.
- **CI-grade automated hardware testing.** This is the human-in-the-loop,
  one-run-per-soldered-board smoke check. The hands-off automated rig
  is tracked separately by IDEA-014.
- **nRF52840 variant.** The structure (Makefile, env layout) is set
  up to accommodate `make test-nrf52840-wiring`, but the actual
  nRF52840 env is deferred until the hardware is reachable again
  (EPIC-025).

## Tasks

Tasks are listed automatically in the Task Epics section of
`docs/developers/tasks/OVERVIEW.md` and in `EPICS.md` / `KANBAN.md`.
