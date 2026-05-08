---
id: IDEA-028
title: Hardware Abstraction Layer — replace #ifdef soup with a platform class hierarchy
category: firmware
description: Introduce an abstract PedalApp base class and per-hardware specializations to eliminate platform #ifdefs from main.cpp and PedalLogic sources.
---

## Archive Reason

2026-04-29 — Realized by EPIC-020 (HAL Refactor). All 5 phases shipped: PedalApp / BlePedalApp / {Esp32,Nrf52840,Host}PedalApp class hierarchy, IFileSystem DI, host integration tests.

## Motivation

`src/main.cpp` currently uses four `#ifdef ESP32` guards to splice in
`BleConfigService`, NVS-based profile persistence, and the config-service loop.
`PedalLogic` sources scatter `#ifndef HOST_TEST_BUILD` and `#ifdef NRF52840_XXAA`
guards throughout action implementations, `littlefs_file_system.cpp`, and the
serial logger. Adding a third target (ESP32 with display) will make this worse.

The goal is to express "what the hardware does" through a class interface, not
through preprocessor branches, so that `main.cpp` and the shared logic have no
platform knowledge at all.

## Proposed approach

### 1. Abstract `PedalApp` base class

Extract the common setup/loop skeleton from `main.cpp` into a pure virtual base:

```cpp
// src/pedal_app.h
class PedalApp {
public:
    virtual ~PedalApp() = default;
    void setup();             // calls platformSetup()
    void loop();              // connection tracking, process_events(), calls platformLoop()

protected:
    virtual void platformSetup() = 0;    // BLE init, NVS open, display init …
    virtual void platformLoop() = 0;     // bleConfigService.loop(), display refresh …
    virtual void saveProfile(uint8_t index) = 0;
    virtual uint8_t loadProfile() = 0;
};
```

`pedal_app.h` must not include `<Arduino.h>` or any hardware-specific headers — only standard C++ and project interfaces. Arduino APIs (`attachInterrupt`, `millis`, etc.) belong in concrete subclasses or `BlePedalApp`, not in the base.

LED/button arrays and shared state (profile manager, event dispatcher, LED objects) are **constructor-injected** into the base, not owned by it. This is consistent with the existing DI pattern for `IBleKeyboard`, `ILEDController`, and `ILogger`, and keeps the base class testable without hardware.

### 2. Intermediate shared layers (mixin / partial base classes)

Not every implementation detail is unique to a single target. A two-level
hierarchy avoids copy-paste between related targets:

```
PedalApp  (pure abstract — no platform knowledge)
├── BlePedalApp          (shared: BLE connection tracking, interrupt
│                         attach/detach on connect/disconnect, LED states)
│   ├── Esp32PedalApp    (adds: NVS persistence, BleConfigService loop)
│   │   ├── Esp32PedalAppNoDisplay   ← current ESP32 target
│   │   └── Esp32PedalAppDisplay     ← future ESP32 + display target
│   └── Nrf52840PedalApp (adds: nRF BLE adapter, stub persistence)
└── HostPedalApp         (no BLE, no hardware — for host/test builds)
```

**`BlePedalApp`** captures everything that is true for any BLE-based pedal,
regardless of MCU:

- connection-state tracking (`connected` flag, `attachInterrupts` / `detachInterrupts` on state change)
- Bluetooth LED on/off on connect/disconnect
- the `process_events()` loop (same on all targets)
- delayed-action polling and power-LED blink pattern

This is verified against the current code: the connect/disconnect block,
`attachInterrupt`/`detachInterrupt` calls, LED state changes, and
`process_events()` invocation in `loop()` are byte-for-byte identical on
ESP32 and nRF52840. The only difference in the existing `loop()` body is
`bleConfigService.loop()`, which is already isolated behind `#ifdef ESP32`
and maps directly to `Esp32PedalApp::platformLoop()`.

**`Esp32PedalApp`** adds what is true for any ESP32 board:

- NVS `saveProfile` / `loadProfile` via `Preferences`
- `BleConfigService` init and `loop()` call

**`Esp32PedalAppDisplay`** extends `Esp32PedalApp` with display init and
refresh — the only layer that knows about the display driver.

**`Nrf52840PedalApp`** extends `BlePedalApp` with the nRF-specific BLE
adapter and a no-op persistence implementation.

### 3. Concrete specializations in `lib/hardware/<target>/`

| File | Purpose |
|---|---|
| `lib/hardware/esp32/src/esp32_pedal_app.cpp` | NVS persistence, `BleConfigService`, no display |
| `lib/hardware/esp32_display/src/esp32_display_pedal_app.cpp` | display init/refresh on top of `Esp32PedalApp` |
| `lib/hardware/nrf52840/src/nrf52840_pedal_app.cpp` | nRF BLE adapter, stub persistence |

`BlePedalApp` lives in a shared location (e.g. `src/ble_pedal_app.h/cpp` or
`lib/PedalLogic/`) so all hardware targets can inherit it without duplicating
the connection-management logic.

### 4. Test / host specialization

`HostPedalApp` lives in `test/fakes/host_pedal_app.h/cpp` and is linked
into host test binaries by CMake. It never appears in the PlatformIO build
graph — the build system chooses the concrete class, not the preprocessor.
It provides no-op persistence and wires up the existing fakes
(`hardware_config_fake.cpp`, `NullLedController`, `NullLogger`). Because
`HostPedalApp` is selected at link time, no `#ifdef HOST_TEST_BUILD` is
needed anywhere in `src/` or `lib/PedalLogic/` — that is the whole point.

### 5. Eliminate platform guards from PedalLogic

Once hardware-specific behaviour lives only in the concrete `PedalApp`
subclass, the `#ifndef HOST_TEST_BUILD` and `#ifdef NRF52840_XXAA` guards
in `send_action.h/.cpp`, `serial_action.*`, `delayed_action.cpp`,
`pin_action.cpp`, `littlefs_file_system.cpp`, and `serial_logger.cpp` can
be removed in favour of proper dependency injection (strategy pattern,
constructor-injected interfaces already exist for `ILEDController`,
`IBleKeyboard`, `ILogger`).

### 6. Preprocessor policy

Preprocessor guards are acceptable **only** when there is no sane
alternative. The bar is high: a guard must be ported to a new target or
configuration simply by being present — it must not encode a decision that
the class hierarchy or DI can express instead.

**Acceptable — keep:**

| Guard | Reason |
|---|---|
| `#pragma once` | Include guard — not a platform branch |
| `#ifndef IRAM_ATTR` in `platform.h` | Portable compiler-attribute shim |
| `#ifndef MAX_CONFIG_BYTES` in `ble_config_reassembler.h` | Overridable compile-time constant |

**Replace with class hierarchy / DI:**

| Guard | Replacement |
|---|---|
| `#ifdef ESP32` in `main.cpp` | `Esp32PedalApp::platformSetup/platformLoop` |
| `#ifdef ESP32` for NVS persistence | `Esp32PedalApp::saveProfile/loadProfile` |
| `#ifdef NRF52840_XXAA` in `littlefs_file_system.cpp` | nRF-specific `IFileSystem` implementation |
| `#ifndef HOST_TEST_BUILD` throughout PedalLogic | `HostPedalApp` + injected fakes; no guard needed |

A guard that would otherwise belong to the "replace" row is not made
acceptable by being small, old, or convenient. The default answer is: use
the class hierarchy.

## Display support — option analysis

The hierarchy above places display knowledge in a leaf class
(`Esp32PedalAppDisplay`). That works for a single display target, but
breaks down once we assume two unrelated MCUs can both have an LCD
(e.g. ESP32 + LCD and Arduino + LCD, but no Arduino without one). Three
approaches were considered.

### Option A — deep inheritance chain

Extend the existing hierarchy with a display-aware leaf for each MCU:

```
BlePedalApp
├── Esp32PedalApp
│   ├── Esp32PedalAppNoDisplay
│   └── Esp32PedalAppDisplay      ← display logic duplicated here …
└── ArduinoPedalApp
    └── ArduinoPedalAppDisplay    ← … and here
```

**Pro:** straightforward, no new patterns.  
**Con:** display init and refresh logic must be copied into every
MCU-specific display leaf. Adding a third MCU with a display means a
third copy. The hierarchy grows as the Cartesian product of
MCU × display-presence.

### Option B — `IDisplay` injected via constructor (composition)

Introduce a thin `IDisplay` interface alongside the existing
`ILEDController`, `IBleKeyboard`, and `ILogger`:

```cpp
class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual void setup() = 0;
    virtual void update() = 0;   // called every loop tick
};
```

The app class hierarchy stays flat. `BlePedalApp` (or `PedalApp`)
holds an `IDisplay*` that is passed in at construction time. Targets
without a display pass a `NullDisplay`. Targets with an LCD pass an
`LcdDisplay` — the same implementation regardless of MCU.

```
PedalApp (holds IDisplay* — injected)
├── BlePedalApp
│   ├── Esp32PedalApp       ← wired with LcdDisplay or NullDisplay
│   ├── Nrf52840PedalApp    ← wired with NullDisplay
│   └── ArduinoPedalApp     ← wired with LcdDisplay
└── HostPedalApp            ← wired with NullDisplay
```

**Pro:** display logic lives in exactly one place (`LcdDisplay`).
Adding a new MCU with a display requires no change to display code at
all — just wire up `LcdDisplay` in the factory. Consistent with the
existing DI pattern for `IBleKeyboard` and `ILogger`.  
**Con:** one more constructor parameter; the no-display case requires
an explicit `NullDisplay` object (minor boilerplate).

### Option C — `DisplayPedalApp` intermediate base

Insert a display-aware intermediate between `BlePedalApp` and the
concrete targets:

```
BlePedalApp
├── DisplayPedalApp             ← display init/refresh here, once
│   ├── Esp32DisplayPedalApp
│   └── ArduinoDisplayPedalApp
├── Esp32PedalApp               ← no-display ESP32
└── Nrf52840PedalApp
```

**Pro:** display logic is not duplicated.  
**Con:** the hierarchy is wider and deeper; the concrete leaves still
exist per MCU; a target that is display-optional would need to pick a
branch at compile time, not at runtime.

### Recommendation — Option B

Inject `IDisplay` as a constructor dependency. It fits the pattern
already established by `IBleKeyboard` and `ILogger`, keeps the class
hierarchy flat, and makes the display completely reusable across MCUs
without any extra leaf classes. The `NullDisplay` no-op is three lines
of code and eliminates every display-related `#ifdef`.

The open question about `IDisplay` scope (raised earlier) is answered:
design it alongside this refactor, not separately — it is the same
architectural move.

## Entry-point file placement — option analysis

With `src/main.cpp` replaced by a concrete `PedalApp` subclass, each
target still needs a free `setup()` / `loop()` shim — Arduino / PlatformIO
requires these as global functions. All targets cannot share one
`src/main.cpp`. Three options were considered.

The slim shim for any target looks like this:

```cpp
#include <Arduino.h>
#include "esp32_pedal_app.h"

static Esp32PedalApp app;

void setup() { app.setup(); }
void loop()  { app.loop();  }
```

The `#ifdef ESP32` is gone: the correct concrete class is selected by
`lib_ignore` in `platformio.ini`, so the preprocessor has nothing left
to branch on.

### Option A — one file per target in a flat `src/`

```
src/
  main_esp32.cpp
  main_nrf52840.cpp
  main_arduino.cpp
  main_esp32_display.cpp
```

Each env in `platformio.ini` selects its file:

```ini
[env:nodemcu-32s]
build_src_filter = -<*> +<main_esp32.cpp>

[env:feather-nrf52840]
build_src_filter = -<*> +<main_nrf52840.cpp>
```

**Pro:** all entry points visible at a glance in one directory.  
**Con:** `src/` becomes a flat list of thin files with no grouping;
filename suffixes carry the only structure.

### Option B — `main.cpp` inside each hardware library

```
lib/hardware/esp32/src/main.cpp
lib/hardware/nrf52840/src/main.cpp
lib/hardware/arduino/src/main.cpp
```

PlatformIO compiles library sources automatically, but `setup()` /
`loop()` as free functions inside a library is unconventional. If more
than one hardware library is in scope (e.g. during a misconfigured build)
the linker sees duplicate symbols.

**Pro:** entry point travels with its library — nothing extra to wire up.  
**Con:** fragile; fights PlatformIO conventions; linker conflicts if
`lib_ignore` is ever misconfigured.

### Option C — one subfolder per target inside `src/`

```
src/
  esp32/main.cpp
  esp32_display/main.cpp
  nrf52840/main.cpp
  arduino/main.cpp
```

Each env selects its subfolder:

```ini
[env:nodemcu-32s]
build_src_filter = -<*> +<esp32/>

[env:feather-nrf52840]
build_src_filter = -<*> +<nrf52840/>
```

**Pro:** subfolder names mirror `lib/hardware/esp32/`,
`lib/hardware/nrf52840/` — consistent mental model across the whole
project. Each subfolder can grow to hold additional target-specific
init code without polluting the other targets. `build_src_filter` reads
as "compile only the esp32 target", which is self-documenting.  
**Con:** same `build_src_filter` boilerplate per env as Option A, but
no worse.

### Recommendation — Option C

The subfolder layout keeps `src/` and `lib/hardware/` symmetrical —
both are organised by target name. It scales cleanly: adding
`src/esp32_display/` for the display variant is one new folder with
one three-line file, and a single `build_src_filter` line in
`platformio.ini`. No linker risk, no filename-suffix noise.

The `lib_ignore` mechanism continues to handle which hardware
*library* each env sees; `build_src_filter` handles which entry-point
*file* each env compiles. The two mechanisms stay orthogonal.

## Migration plan

This is a three-phase refactor. Each phase leaves the codebase in a
buildable, passing state; no phase depends on the next to be merged.

### Phase 1 — eliminate `#ifdef ESP32` from `main.cpp`

1. Introduce `PedalApp` base class in `src/pedal_app.h`.
2. Implement `Esp32PedalApp` in `lib/hardware/esp32/src/`.
3. Add entry-point subfolders: `src/esp32/main.cpp`,
   `src/nrf52840/main.cpp`; wire `build_src_filter` in `platformio.ini`.
4. **Milestone:** `grep -r "#ifdef ESP32" src/` returns zero hits.

### Phase 2 — extract `BlePedalApp` shared layer

1. Create `src/ble_pedal_app.h/cpp` (or `lib/PedalLogic/src/`).
2. Move the connect/disconnect block, `attachInterrupts`/`detachInterrupts`,
   `process_events()`, action polling, and LED blink loop into `BlePedalApp`.
3. `Esp32PedalApp` and `Nrf52840PedalApp` both inherit from it.
4. **Milestone:** `loop()` bodies in both hardware libs are ≤ 3 lines.

### Phase 3 — eliminate `#ifndef HOST_TEST_BUILD` from PedalLogic

1. Add `HostPedalApp` to `test/fakes/`.
2. Wire it into `test/CMakeLists.txt`; drop the `target_compile_definitions(pedal_tests PRIVATE HOST_TEST_BUILD)` line.
3. Remove guards file by file: start with `serial_logger.cpp` and
   `delayed_action.cpp` (simplest), finish with `littlefs_file_system.cpp`
   (requires an `IFileSystem` DI point if one is not already present).
4. **Milestone:** `grep -r "HOST_TEST_BUILD\|NRF52840_XXAA" lib/PedalLogic/`
   returns zero hits.

### Test suite updates (all phases)

Every phase that changes the architecture must update the tests in the same
PR — not as a follow-up. Concretely:

- **Phase 1:** Any host unit test that currently instantiates production
  objects that are guarded by `#ifndef HOST_TEST_BUILD` (e.g.
  `test_pin_action.cpp`, `test_macro_action.cpp`) must be verified to still
  compile and pass after the guards are gone. No test should silently become
  a no-op because a previously guarded code path is now always compiled in.

- **Phase 2:** `BlePedalApp` logic (connect/disconnect, interrupt
  attach/detach, LED state) must be covered by new host unit tests in
  `test/unit/test_ble_pedal_app.cpp`. The `arduino_shim.h` already stubs
  `attachInterrupt` / `detachInterrupt`; use it. On-device integration tests
  (`test_buttons_esp32`, `test_buttons_nrf52840`) must still pass on
  hardware.

- **Phase 3:** After the `HOST_TEST_BUILD` define is removed from
  `test/CMakeLists.txt`, every source file that previously compiled
  differently under that flag must be audited. Run `/test` and confirm zero
  tests are skipped or guarded out. Any test that was relying on a guarded
  no-op path must be rewritten to inject the appropriate fake via the new DI
  interface.

## Scope

This is a preparatory refactor — no behaviour changes. A good milestone
would be: `grep -r "#ifdef ESP32" src/ lib/PedalLogic/` returns zero hits
after the work is done.

## Documentation impact

This refactor will likely require an extensive documentation overhaul. The
class hierarchy, DI wiring, and file layout changes touch almost every part
of the developer-facing docs:

- **`ARCHITECTURE.md`** — the core class diagram, data-flow description, and
  platform-layer section all describe the current `#ifdef` structure; these
  need to be rewritten around `PedalApp`, `BlePedalApp`, and the concrete
  subclass tree
- **`TESTING.md`** — the host-test section references `HOST_TEST_BUILD` and
  the `arduino_shim.h` pattern; once `HostPedalApp` replaces the guard, the
  instructions for writing new tests change significantly
- **`docs/developers/`** platform-specific guides (ESP32, nRF52840) — any
  guide that explains how to add platform behaviour currently points at
  `main.cpp` `#ifdef` blocks; those become "add a method to your
  `PedalApp` subclass" instructions instead
- **Inline code comments** throughout `src/` and `lib/` that explain
  guard rationale become misleading once the guards are gone — a sweep is
  needed

The documentation overhaul is significant enough that it should be tracked
as a dedicated task (or sub-tasks per phase) alongside the implementation
tasks, not treated as an afterthought.
