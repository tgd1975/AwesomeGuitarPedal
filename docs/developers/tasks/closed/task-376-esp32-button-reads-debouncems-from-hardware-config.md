---
id: TASK-376
title: ESP32 — Button reads debounceMs from the loaded hardware config
status: closed
closed: 2026-05-11
opened: 2026-05-11
effort: Small (<2h)
effort_actual: Medium (2-8h)
complexity: Medium
human-in-loop: No
epic: configurable-debounce
order: 2
prerequisites: [TASK-375]
---

## Description

Plumb the `debounceMs` field from the loaded hardware config into the
ESP32 `Button` class so the debounce window is configurable per build
without recompiling firmware. Today
[`src/esp32/include/button.h:29`](../../../../src/esp32/include/button.h)
hard-codes `unsigned long debounceDelay = 100;` and there is no path
from `HardwareConfig` to that member. After this task, the boot path
that constructs `Button` instances must pass the configured value
through (or call a setter immediately after construction), with a
100 ms fallback when the field is absent.

The change is small in lines but touches the boot sequence — get
the order right:

1. Hardware config loads from LittleFS.
2. `debounceMs` is resolved (config value if present, otherwise 100).
3. `Button` instances are constructed for `buttonPins[]` and
   `buttonSelect`, with `debounceDelay` set to the resolved value
   *before* `attachInterrupt()` runs (the value is read inside the
   ISR via `isDebounced`, so it must be stable before the first
   interrupt can fire).

Prefer constructor injection (`Button(uint8_t pin, unsigned long
debounceMs)`) over a post-construct setter; it makes the dependency
explicit and removes a "did I forget to call `setDebounce()`?" foot-
gun. Keep the existing one-arg constructor for any test code that
relied on the 100 ms default, or update those call sites as part of
this task.

## Acceptance Criteria

- [ ] ESP32 firmware constructs `Button` instances using
      `HardwareConfig::debounceMs` (or its 100 ms default).
- [ ] `Button::debounceDelay` is set before `attachInterrupt()` is
      called.
- [ ] Booting with `debounceMs: 250` in `data/config.json` produces a
      visibly slower debounce (manual on-device verification — a
      stop-watch press test or scope capture is overkill; a simple
      "does double-pressing within 250 ms register as one press" is
      sufficient).
- [ ] Booting with `debounceMs` absent uses 100 ms (the existing
      behaviour).
- [ ] No new compile-time constant for debounce remains in the ESP32
      sources — the only debounce constant in the tree is the JSON
      schema/model default.

## Test Plan

**Host tests** (`make test-host`):

- Extend the Button host test (`test/unit/test_button*.cpp`) to
  construct a `Button` with a non-default `debounceMs` and verify
  the ISR-equivalent path respects the configured window using the
  Arduino shim's `millis()` mock. Cover both 50 ms and 250 ms cases.

**On-device tests** (`make test-esp32-button`):

- Extend `test/test_buttons_esp32/test_main.cpp` with one scenario
  that boots a config with a non-default `debounceMs` (e.g. 250) and
  asserts that two presses inside that window register as a single
  press. Requires ESP32 connected via USB.

## Prerequisites

- **TASK-375** — delivers the schema, example config, and Dart
  model so the field is observable end-to-end. This task consumes
  the JSON contract that task lands.

## Notes

- **nRF52840 deferral.** The nRF52840 `Button` has the same
  hard-coded constant
  (`src/nrf52840/include/button.h:23`). Per the
  `nrf5-task-routing` skill and EPIC-025, on-device nRF52840 work
  is paused. Do **not** add an nRF52840 firmware change in this
  task or scaffold a paused task here — the parity audit
  (TASK-360) will pick this up when the hardware is reachable
  again, and the schema/model contract already supports it.
- **ISR safety.** `debounceDelay` is read from inside the ISR
  (`isDebounced`). Setting it after `attachInterrupt()` would race
  the first physical press. Construct → set → attach, in that order.
- **Constructor overload, not template.** Two constructors (one with
  default 100, one with explicit) is fine. Resist the urge to add a
  config struct or builder pattern — this is one integer.
- **Bounds enforcement.** Schema-side bounds (TASK-375) make
  firmware-side bounds checking unnecessary. Trust the contract.
