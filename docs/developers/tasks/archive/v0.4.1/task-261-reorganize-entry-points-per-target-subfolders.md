---
id: TASK-261
title: Reorganize entry points into per-target subfolders
status: closed
opened: 2026-04-28
closed: 2026-04-28
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Medium
human-in-loop: Support
epic: hal-refactor
order: 1
---

## Description

Move `src/main.cpp` into per-target subfolders so that each hardware
environment compiles its own entry-point file. Initially the two files are
identical copies of the current `main.cpp` — the existing `#ifdef ESP32`
guards stay in place. No class hierarchy is introduced yet; that comes in
T2 (TASK-262 "Introduce PedalApp + Esp32PedalApp").

The point of doing this as its own task is to isolate the PlatformIO
`build_src_filter` change from the C++ refactor. Reviewers (and bisect)
can verify the build-system reorg in isolation before any logic moves.

After this task:

- `src/esp32/main.cpp` — current `main.cpp` content, ESP32-targeted
- `src/nrf52840/main.cpp` — current `main.cpp` content, nRF52840-targeted
- `platformio.ini` selects the subfolder via `build_src_filter`

The subfolder names mirror `lib/hardware/<target>/` (IDEA-028 Option C):
both `src/` and `lib/hardware/` are organised by target name. `lib_ignore`
continues to handle hardware-library selection — orthogonal to
`build_src_filter`.

## Acceptance Criteria

- [ ] `src/esp32/main.cpp` and `src/nrf52840/main.cpp` exist with the
      current `main.cpp` content; `src/main.cpp` is gone.
- [ ] `platformio.ini` has a `build_src_filter = -<*> +<<target>/>` line
      for each hardware env (`nodemcu-32s`, `feather-nrf52840`, plus any
      other ESP32/nRF envs).
- [ ] Both targets build cleanly: `pio run -e nodemcu-32s` and
      `pio run -e feather-nrf52840` succeed with no new warnings.
- [ ] `make test-host` still passes — host build is unaffected.
- [ ] On-device smoke test on each target: device boots, advertises BLE,
      buttons fire actions. No behaviour change vs. before this task.

## Test Plan

No new tests — this is a build-system reorganization with no logic change.

**Host tests** (`make test-host`):

- Verify the existing suite still passes. Host build does not use
  `build_src_filter`, so this is mostly a sanity check.

**On-device verification** (manual, both targets):

- `pio run -e nodemcu-32s` and `pio run -e feather-nrf52840` build
  successfully.
- Flash each target and confirm baseline behaviour: BLE advertises,
  pairing works, button presses send the expected actions.

## Notes

- Watch out for any other env in `platformio.ini` that currently relies
  on the implicit "compile everything in `src/`" behaviour — every env
  needs an explicit `build_src_filter` once one of them sets it.
- This task does not touch `lib/hardware/`. Future tasks will add concrete
  `PedalApp` subclasses there; the entry-point shim files will then be
  reduced to ~3 lines each.
- Reference: IDEA-028 §"Entry-point file placement — option analysis"
  (Option C — subfolder per target inside `src/`).
