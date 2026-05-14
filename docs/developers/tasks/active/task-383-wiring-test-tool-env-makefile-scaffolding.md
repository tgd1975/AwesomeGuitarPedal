---
id: TASK-383
title: Wiring test tool — PlatformIO env, Makefile target, and compile-time CONFIG flag (ESP32)
status: active
opened: 2026-05-11
effort: Medium (2-8h)
complexity: Medium
human-in-loop: No
epic: wiring-test-tool
order: 1
---

## Description

Lay the structural foundation the rest of EPIC-030 builds on. This
task delivers nothing functional that a builder can press a button
against yet — instead it gets the build path right so subsequent
tasks can focus on the interactive UI without re-litigating
infrastructure decisions.

Three pieces:

1. **PlatformIO env** — a new env in `platformio.ini`, e.g.
   `esp32-wiring-test`, that reuses the existing toolchain, BSP, and
   GPIO HAL. Per IDEA-062, we accept that the env pulls in some
   unused stack (BLE, HID, profile parser) rather than maintain a
   second build path — the unused code is dead weight, not a bug.
2. **Makefile target** — `make test-esp32-wiring CONFIG=<path>`
   that:
   - **Requires** `CONFIG=`. Missing → fail cleanly with a usage
     message naming an example invocation.
   - Resolves the path (absolute, or relative to repo root).
   - Reads the file at build time so it can be embedded as a
     compile-time flag (`-D CONFIG_JSON=<contents>` or, more
     practically, generate a small header and include it). Pick
     whichever is least painful for the existing build wiring;
     header generation tends to scale better when the JSON gets
     non-trivial.
   - Builds the new env and flashes it to `$ASP_ESP32_PORT`.
   - Optionally tails the serial monitor on completion (a flag like
     `MONITOR=1`) — convenient but not load-bearing.
3. **Skeleton firmware entry point** — a `src/` file or sub-directory
   under the new env that:
   - Parses the embedded config JSON at boot.
   - Prints a one-screen banner with: tool name, version (whatever
     `version.h` exposes), loaded config filename, button list
     (logical name → GPIO, polarity), LED list (logical name →
     GPIO, active polarity), and a placeholder "press `?` for help"
     hint.
   - Wires the serial console for single-key (no-enter) keystroke
     reads.
   - Does *nothing else yet* — no button press handling, no LED
     modes. Those are TASK-384 / TASK-385 / TASK-386.

The point of this task is "you can flash the wiring tool and see the
banner; the next tasks fill it in."

## Acceptance Criteria

- [ ] `make test-esp32-wiring CONFIG=data/config.json` builds, flashes,
      and boots; the banner appears on the serial monitor naming the
      configured buttons and LEDs.
- [ ] `make test-esp32-wiring` (no `CONFIG=`) fails cleanly with a
      usage message naming the missing argument and an example.
- [ ] `make test-esp32-wiring CONFIG=/nonexistent.json` fails before
      flashing with a clear "config not readable" message.
- [ ] The new PlatformIO env is named explicitly (e.g.
      `esp32-wiring-test`) and structured to leave room for a
      `nrf52840-wiring-test` sibling later (EPIC-025 deferral).
- [ ] The serial console is in single-key mode (no echo, no
      line-buffering) so subsequent tasks can wire keystrokes
      without revisiting the I/O setup.

## Test Plan

No automated tests required — change is build infrastructure plus a
banner that a human reads. Verification is:

- Run `make test-esp32-wiring CONFIG=data/config.json` and read the
  banner. Requires ESP32 connected via USB.
- Run the three error cases (missing `CONFIG`, bad path, empty
  file) and confirm clean failures.

If the JSON-embed mechanism is non-trivial (e.g. a Python helper
script under `scripts/`), add a small host-side test for that
helper. The point is to catch silent embed failures (truncation,
quote escaping) before the firmware boots with a broken config.

## Notes

- **Why a separate env, not a runtime mode.** IDEA-062 explicitly
  rejected a runtime LittleFS loader — this is a debugging session
  tool, recompile-per-board is cheap, and a runtime path means
  carrying mode-switching logic in production firmware forever. One
  PlatformIO env, one purpose.
- **Why `CONFIG=` is mandatory.** Driving the wrong pins because we
  guessed at a default config is exactly the failure the tool is
  built to catch. Better a usage error than a misleading test.
- **Sibling structure for nRF52840.** The nRF52840 env is deferred
  (EPIC-025) but the Makefile/env naming should be set up so
  `make test-nrf52840-wiring CONFIG=...` is a drop-in addition
  rather than a refactor. Pick names that already imply the second
  target exists (e.g. `wiring-test-esp32` not `wiring-test-only`).
- **Single-key serial console.** PlatformIO's monitor tends to be
  line-buffered by default; the wiring tool needs raw character
  input. Document the monitor invocation (or whatever the
  preferred client is) so a builder following the docs is not
  fighting their terminal in TASK-384.
- **Out of scope here.** All the interactive behaviour — button
  press logging, LED modes, status-display re-rendering, keyboard
  legend — belongs to TASK-384, TASK-385, TASK-386, TASK-387. This
  task is the foundation only.
