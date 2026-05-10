---
id: EPIC-020
name: hal-refactor
title: HAL Refactor — replace #ifdef soup with platform class hierarchy
status: closed
opened: 2026-04-28
closed: 2026-04-29
assigned:
---

Seeded by IDEA-028 (Hardware Abstraction Layer — replace #ifdef soup with a platform class hierarchy).

`src/main.cpp` and the shared `PedalLogic` sources are riddled with
`#ifdef ESP32`, `#ifdef NRF52840_XXAA`, and `#ifndef HOST_TEST_BUILD`
guards. Adding a third target (ESP32 with display) makes this worse.

This epic introduces a `PedalApp` class hierarchy with constructor-injected
dependencies so that platform behaviour lives in concrete subclasses, not
in preprocessor branches. See IDEA-028 for the full design — Option B for
display (DI via `IDisplay`) and Option C for entry-point layout (per-target
subfolders under `src/`).

## Phases

The work is split into four implementation phases plus a documentation
phase. Each phase leaves the codebase in a buildable, passing state.

1. **Phase 1** — eliminate `#ifdef ESP32` from `main.cpp`. Tasks T1–T3 in
   the breakdown:
   - reorganize entry points into per-target subfolders (move-only, no
     class changes)
   - introduce `PedalApp` base + `Esp32PedalApp`, migrate `#ifdef ESP32`
     blocks
   - add `Nrf52840PedalApp`, retire shared `main.cpp`
   - **milestone:** `grep -r "#ifdef ESP32" src/` returns zero hits.
2. **Phase 2** — extract `BlePedalApp` shared layer (connect/disconnect,
   interrupts, `process_events()`, LED blink). Single task.
3. **Phase 3** — eliminate `#ifndef HOST_TEST_BUILD` from `PedalLogic`
   via a `HostPedalApp` test fake and `IFileSystem` DI. Likely 2–3 tasks
   split by guard family (logger/delayed → actions → littlefs).
4. **Phase 4 (build-system collapse)** — fold
   `lib/hardware/<target>/` into `src/<target>/` so the platform code
   lives next to its entry-point shim. Removes the deprecated
   `lib_extra_dirs` mechanism (PIO 6.0+) and reduces per-env target
   selection to a single axis (`build_src_filter`). `lib/PedalLogic/`
   stays — it is genuinely shared code and exactly what PIO's `lib/`
   is for.
   - **TASK-296 (ESP32):** done. `src/esp32/` now holds the platform
     code.
   - **TASK-297 (nRF52840):** done on the second attempt. The first
     attempt hit a PIO LDF gotcha — `bluefruit.h` was only included
     transitively from a header, and the LDF in `chain` mode does
     not follow header chains, so the framework-bundled library
     never activated. Fix: one direct `#include <bluefruit.h>` (and
     `<Adafruit_nRFCrypto.h>`) at the top of
     `src/nrf52840/ble_keyboard_adapter.cpp` gave the LDF the direct
     signal it needed. ESP32 worked first time because every
     framework lib it uses already had a direct `.cpp` include
     (`<Preferences.h>`, `<LittleFS.h>` etc.).
   - **milestone met:** `lib/hardware/` is gone; `lib_extra_dirs`
     and `lib_ignore` lines are gone from `platformio.ini`; both
     production builds and host tests pass. The detailed LDF
     diagnosis lives in the TASK-297 body for future maintainers.
5. **Phase 5 (docs)** — overhaul `ARCHITECTURE.md`, `TESTING.md`, and the
   platform-specific developer guides to match the new class hierarchy
   and the post-Phase-4 layout. Inline comment sweep for stale guard
   rationale.

## Tasks

Tasks are listed automatically in the Task Epics section of
`docs/developers/tasks/OVERVIEW.md` and in `EPICS.md` / `KANBAN.md`.
