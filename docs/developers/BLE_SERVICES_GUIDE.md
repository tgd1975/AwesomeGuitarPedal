# BLE Services Developer Guide

This is the first thing you should read when adding or changing a BLE service or characteristic in this project. It collects, in one place, the catalog of services that exist today, the end-to-end recipe for adding or changing one, the conventions and cross-cutting invariants that span layers, the gotchas that we keep rediscovering, and pointers to the tests and external references.

**Status of related docs.** This guide is the entry point. Two pre-existing docs are kept and cross-linked from here rather than folded in:

- [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md) — authoritative byte-level wire format for the chunked-write protocol. Cited from the *Service catalog* and *Conventions* sections below.
- [BLE_READBACK_IMPACT.md](BLE_READBACK_IMPACT.md) — frozen-in-time feasibility analysis from [TASK-353](tasks/archive/v0.5.0/task-353-feasibility-firmware-ble-readback-surfaces.md) that gates the EPIC-026 readback cluster (TASK-354 / TASK-355 / TASK-356). Cited from *Service catalog*.

A third historical doc, `BLE_CONFIG_IMPLEMENTATION_NOTES.md`, was folded into the *Conventions* and *Gotchas* sections of this guide ([TASK-367](tasks/closed/task-367-draft-conventions-invariants-gotchas.md)) and deleted.

## Contents

1. [Service catalog](#service-catalog)
2. [Recipe — add or change a service / characteristic](#recipe--add-or-change-a-service--characteristic)
3. [Worked example — firmware-version READ (TASK-354)](#worked-example--firmware-version-read-task-354)
4. [Conventions](#conventions) — populated by [TASK-367](tasks/open/task-367-draft-conventions-invariants-gotchas.md)
5. [Cross-cutting invariants](#cross-cutting-invariants) — populated by [TASK-367](tasks/open/task-367-draft-conventions-invariants-gotchas.md)
6. [Gotchas](#gotchas) — populated by [TASK-367](tasks/open/task-367-draft-conventions-invariants-gotchas.md)
7. [Tests](#tests) — populated by [TASK-368](tasks/open/task-368-draft-tests-references-and-verify.md)
8. [References](#references) — populated by [TASK-368](tasks/open/task-368-draft-tests-references-and-verify.md)

## Service catalog

Hand-written inventory of every BLE service and characteristic the project currently exposes. The catalog *will* drift between releases; the *Recipe* below makes "update this catalog row" the first checklist item when adding a characteristic, so drift is caught at change time.

### ESP32 (NimBLE)

**BLE Config service** — `516515c0-4b50-447b-8ca3-cbfce3f4d9f8`

Registered by [`BleConfigService::begin()`](../../src/esp32/src/ble_config_service.cpp) inside `BleKeyboardAdapter`'s `onStarted()` hook (must run before `ble_gatts_start()` locks the GATT table — see [Gotchas](#gotchas)).

| Short name | UUID (last 4) | Properties | Source of truth | One-line purpose |
|---|---|---|---|---|
| `CONFIG_WRITE` | `…5c1` | `WRITE` \| `WRITE_NR` | [ble_config_service.cpp:67](../../src/esp32/src/ble_config_service.cpp#L67) | Chunked write of profile JSON. Wire format: [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md#chunked-write-protocol). |
| `CONFIG_WRITE_HW` | `…5c2` | `WRITE` \| `WRITE_NR` | [ble_config_service.cpp:70](../../src/esp32/src/ble_config_service.cpp#L70) | Chunked write of hardware config JSON. Same chunked protocol as `CONFIG_WRITE`. |
| `CONFIG_STATUS` | `…5c3` | `NOTIFY` | [ble_config_service.cpp:74](../../src/esp32/src/ble_config_service.cpp#L74) | Transfer-status notifications (`READY` / `RESET` / `PROFILE:…` / `ERROR:…`). |
| `HW_IDENTITY` | `…5c4` | `READ` | [ble_config_service.cpp:77](../../src/esp32/src/ble_config_service.cpp#L77) | Board identity string (`"esp32"` / `"nrf52840"`) for app-side disambiguation. |

**Planned (EPIC-026, ESP32-only this iteration):**

| Short name | UUID (last 4) | Properties | Status | Task |
|---|---|---|---|---|
| `FIRMWARE_VERSION` | `…5c5` | `READ` | Not yet in firmware | [TASK-354](tasks/open/task-354-firmware-version-read-characteristic.md) |
| `CONFIG_READBACK` | `…5c1` (READ on existing char) | adds `READ` to `CONFIG_WRITE` | Not yet in firmware | [TASK-355](tasks/open/task-355-firmware-config-readback.md) |
| `ACTIVE_PROFILE` | `…5c6` | `READ` \| `NOTIFY` | Not yet in firmware | [TASK-356](tasks/open/task-356-firmware-active-profile-notify.md) |

See [BLE_READBACK_IMPACT.md](BLE_READBACK_IMPACT.md) for the feasibility analysis that produced these scopes and the platform-asymmetry rationale (ESP32 ships; nRF52840 deferred).

**Reassembly buffer.** Both `CONFIG_WRITE` and `CONFIG_WRITE_HW` feed into a single `BleConfigReassembler` instance — see [lib/PedalLogic/include/ble_config_reassembler.h](../../lib/PedalLogic/include/ble_config_reassembler.h) for the buffer ceiling (`MAX_CONFIG_BYTES`) and the *Cross-cutting invariants* section below for the file set it spans.

**HID service** — `0x1812` (SIG-standard). Provided by `BleKeyboard` (the NimBLE-built `ESP32-BLE-Keyboard` fork). Not implemented by this project; we register the Config service *alongside* it inside the same `BLEServer`.

### nRF52840 (Bluefruit)

**HID only.** [`BleKeyboardAdapter::begin()`](../../src/nrf52840/src/ble_keyboard_adapter.cpp) is HID-only: `Bluefruit.begin()` → `hid.begin()` → `Bluefruit.Advertising.start(0)`. There is **no** custom GATT service on this target.

Practical consequences for anyone adding a readback or writable surface:

- Adding the first nRF52840 readback / writable characteristic is not a 30-line job — it requires standing up a Bluefruit-side custom GATT service (`BLEService` / `BLECharacteristic` / `setReadCallback` / `setWriteCallback`), wiring it into `BleKeyboardAdapter::begin()` *before* advertising starts, and adding the per-pedal persistence equivalent (current nRF52840 `saveProfile()` is a no-op; device always boots to profile 0).
- The whole nRF52840 readback surface is bundled into [TASK-358](tasks/paused/task-358-nrf52840-ble-readback-surfaces.md) — currently paused on hardware availability.
- Until then: the per-platform impact for any new BLE surface on nRF52840 starts with "scaffold a custom Bluefruit GATT service first," not "add a char."

DIS (Device Information Service, `0x180A`) on nRF52840 is the one exception — Bluefruit ships `BLEDis` as a four-line drop-in. Decision logged in [BLE_READBACK_IMPACT.md §1](BLE_READBACK_IMPACT.md#1-platform-asymmetry--the-elephant-in-the-room): activation gated on device.

## Recipe — add or change a service / characteristic

A numbered checklist. Tick through it without grepping the codebase. The worked example in the next section walks one real characteristic (firmware-version READ, TASK-354) through every step.

If you are adding a **new service** rather than a new characteristic on an existing service, steps 1–4 still apply but with one extra layer (a fresh `NimBLEService* svc = pServer->createService(SERVICE_UUID)`). On nRF52840 today there is no existing custom service, so any new char on that target *is* a new-service task — see [TASK-358](tasks/paused/task-358-nrf52840-ble-readback-surfaces.md).

### 1. Pick a UUID and update the catalog

- Increment the last byte of the service UUID (next free is currently `…5c7` after the EPIC-026 cluster lands). Convention: stay inside the `516515cN-4b50-447b-8ca3-cbfce3f4d9f8` family; do not introduce a second service UUID without a reason.
- Add a row to the [Service catalog](#service-catalog) table above with: short name, UUID last 4, properties, source-of-truth file pointer, one-line purpose. Catch drift here.
- Register the UUID in [BLE_CONFIG_PROTOCOL.md → Characteristics](BLE_CONFIG_PROTOCOL.md#characteristics) if it affects the wire format (writes / notifies). READ-only "boring" characteristics still get a row.

### 2. Edit firmware

**ESP32** — [src/esp32/src/ble_config_service.cpp](../../src/esp32/src/ble_config_service.cpp):

1. Add a `static const char* CHAR_<NAME>_UUID = "516515cN-…";` constant in the UUID block at the top of the file.
2. Inside `setupGattService()` (~line 55), call `svc->createCharacteristic(CHAR_<NAME>_UUID, NIMBLE_PROPERTY::<PROPS>)`. For READ characteristics with a static value, follow the `HW_IDENTITY` pattern (wrap the value in `std::string(...)` before `setValue()` — see [Gotchas → NimBLE setValue templating](#gotchas)). For WRITE characteristics, attach a `NimBLECharacteristicCallbacks` subclass. For NOTIFY characteristics, hold the `NimBLECharacteristic*` in a static so `notify()` can be called from elsewhere (mirror `statusChar_`).
3. If the characteristic depends on a project-wide constant (firmware version, hardware identity), import the source-of-truth header (e.g. [include/version.h](../../include/version.h) for `FIRMWARE_VERSION`).

**nRF52840** — deferred to [TASK-358](tasks/paused/task-358-nrf52840-ble-readback-surfaces.md) until hardware is back. If your task is nominally cross-platform, scope it to ESP32-only with an explicit nRF52840-deferred note in the AC (see TASK-354 for the template).

### 3. Edit app-side constants

[app/lib/constants/ble_constants.dart](../../app/lib/constants/ble_constants.dart):

- Add `const String k<Name>Uuid = '516515cN-…';` next to the existing UUID constants. Keep the file ordered by last-nibble.
- If the characteristic carries a cross-layer constant (e.g. `MAX_CONFIG_BYTES`, MTU floor), mirror that constant *here* too — and add the file to the *Cross-cutting invariants* table below.

### 4. Edit app-side service surface

[app/lib/services/ble_service.dart](../../app/lib/services/ble_service.dart):

- Discovery: in the characteristic-discovery loop (around the `kHwIdentityUuid` branch, currently line 197), add an `if (uuid == k<Name>Uuid) _<name>Char = c;` branch. Declare the `_<name>Char` field at class scope.
- Surface: add a typed method on `BleService` — `Future<T?> read<Name>()` / `Future<void> write<Name>(...)` / `Stream<T> <name>Stream()` depending on properties. Follow `readDeviceHardware()` (line ~234) for READ, the existing `_chunkedWrite()` path for chunked WRITE, and the status-stream pattern for NOTIFY.

### 5. Edit app-side consumers (screens / widgets)

The screen that needs the data calls the new `BleService` method. For pages that show a placeholder when data is unavailable, replace the `_PendingRow` (or analogous widget) with a live row backed by the new method. Loading states and the "—" fallback are screen-level concerns, not service-level.

### 6. Update the protocol doc

[BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md) is authoritative for the wire format. Update it if:

- The characteristic is new — add a row to *Characteristics*.
- The wire format changes (new chunk type, new status string, new constant) — update the relevant section *and* the *Constants* table at the bottom.

For READ characteristics with a fixed/simple value (firmware version, hardware identity), a single row in *Characteristics* is enough.

### 7. Update the conventions doc (if applicable)

If the change introduces or alters a *convention* (UUID assignment, property choice, framing rule, error-code semantic) — update the [Conventions](#conventions) section of this guide. Day-to-day characteristic additions don't trigger this; they just follow existing conventions.

### 8. Host tests (when shimmable)

Per [CLAUDE.md → Testing policy](../../CLAUDE.md#testing-policy), prefer host tests over on-device when the dependency can be shimmed. A characteristic is shimmable iff:

- Its value is computed from project-internal state (constants, parsers, formatters) and not from a live BLE peer.
- The transport (NimBLE / GATT) can be replaced with a fake — see `test/fakes/arduino_shim.h` and the `HOST_TEST_BUILD` guard pattern.

If yes: add a test under `test/unit/test_<feature>.cpp`, register the source in [test/CMakeLists.txt](../../test/CMakeLists.txt) under `pedal_tests`, run `/test`. Examples:

- A version-string *formatter* (semver + git-hash concatenation) — host-test the formatter.
- The reassembler — already host-tested at the bytes-in / bytes-out boundary.

If no (e.g. the characteristic is "the GATT property is set correctly and a real central can read it"): skip to step 9.

### 9. On-device tests

For anything that requires a live GATT stack — i.e. "did the characteristic actually register, with the right properties, returning the right value":

- ESP32: extend [test/test_ble_config_esp32/test_main.cpp](../../test/test_ble_config_esp32/test_main.cpp) with a Unity test that exercises the new characteristic via NimBLE's own client API or via the central-side test runner.
- The central-side runner lives at [test/test_ble_config_esp32/runner.py](../../test/test_ble_config_esp32/runner.py). It reads/writes/subscribes to characteristics over BlueZ; extend it if the test needs a real central to talk to the firmware.
- Run via `/test-device esp32-ble-config` (or the matching `make test-esp32-…` target). Requires a USB-connected ESP32 and BlueZ on the host.

### 10. App-side widget / unit tests

- Widget test: extend [app/test/widget/connected_pedal_screen_test.dart](../../app/test/widget/connected_pedal_screen_test.dart) (or the equivalent screen test) to assert the new row / state renders correctly given a mocked `BleService` return.
- Mocks: if you added a new method on `BleService`, regenerate [`connected_pedal_screen_test.mocks.dart`](../../app/test/widget/connected_pedal_screen_test.mocks.dart) (or equivalent) — `flutter pub run build_runner build --delete-conflicting-outputs`.
- For pure parsing / formatting on the app side, add a unit test under `app/test/unit/`.

### 11. End-to-end verification ladder

Run these in order; do not skip rungs:

1. **Host tests** — `/test`. Must pass.
2. **Build** — `make esp32-build` (or the relevant target). Must pass with no clang-format / clang-tidy regressions.
3. **Raw-read sanity** — flash the firmware, connect with `bluetoothctl` or `gatttool` on Linux, manually read / write / subscribe to the new characteristic. Smoke test before the runner.
4. **On-device runner** — `/test-device esp32-ble-config`. The Unity tests must pass against the connected device.
5. **App-side `/verify-on-device`** — drive the relevant screen on the Pixel via [`/verify-on-device <TASK-ID> <SCENARIO-ID>`](../../.claude/skills/verify-on-device/SKILL.md) and confirm the new row / state renders as expected.

Only after all five pass does the task close.

### 12. Sequencing

The sequence within a single PR / commit chain is firmware → protocol-doc → app-constant → app-service → app-screen → tests. The protocol-doc and catalog row go in the same commit as the firmware change so the wire definition can never be ahead of or behind the firmware. App-side and tests can lag in subsequent commits on the same branch, but should land before the branch merges.

## Worked example — firmware-version READ (TASK-354)

This walks [TASK-354](tasks/open/task-354-firmware-version-read-characteristic.md) (firmware-version READ on ESP32) through the recipe step-by-step, *describing* every file an implementer would touch. The TASK-354 implementer makes the actual edits.

### Step 1 — UUID and catalog

- UUID: `516515c5-4b50-447b-8ca3-cbfce3f4d9f8` (next free in the `5cN` family).
- Catalog row: already present in *Service catalog → ESP32 → Planned (EPIC-026)* above. When TASK-354 lands, the implementer moves the row from the Planned table to the main characteristics table, with source-of-truth pointer `src/esp32/src/ble_config_service.cpp:NN` (where `NN` is the new `createCharacteristic` line).
- Protocol doc: add a row to [BLE_CONFIG_PROTOCOL.md → Characteristics](BLE_CONFIG_PROTOCOL.md#characteristics):

  | `FIRMWARE_VERSION` | `516515c5-…` | READ |

### Step 2 — ESP32 firmware

[src/esp32/src/ble_config_service.cpp](../../src/esp32/src/ble_config_service.cpp):

1. At the UUID block (~line 18–22), add:

   ```cpp
   static const char* CHAR_FIRMWARE_VERSION_UUID = "516515c5-4b50-447b-8ca3-cbfce3f4d9f8";
   ```

2. At the top of the file (~line 8), include the version header:

   ```cpp
   #include "version.h"  // FIRMWARE_VERSION
   ```

   ([include/version.h](../../include/version.h) defines `FIRMWARE_VERSION` — currently `"v0.5.0"`.)

3. Inside `setupGattService()`, after the `HW_IDENTITY` block (~line 77–83), add a parallel block:

   ```cpp
   NimBLECharacteristic* fwChar =
       svc->createCharacteristic(CHAR_FIRMWARE_VERSION_UUID, NIMBLE_PROPERTY::READ);
   fwChar->setValue(std::string(FIRMWARE_VERSION));
   ```

   The `std::string(...)` wrap is **mandatory** — see [Gotchas → NimBLE setValue templating](#gotchas) (TASK-235). Without it, `setValue(const char*)` writes the pointer bytes, not the string.

No nRF52840 edit. The TASK-354 AC explicitly defers the nRF52840 surface to [TASK-358](tasks/paused/task-358-nrf52840-ble-readback-surfaces.md).

### Step 3 — App constants

[app/lib/constants/ble_constants.dart](../../app/lib/constants/ble_constants.dart):

After the `kHwIdentityUuid` line (line 26), add:

```dart
// Read-only: firmware version string from include/version.h (e.g. "v0.5.0").
// nRF52840 deferred to TASK-358 — connected_pedal_screen falls back to "—".
const String kFirmwareVersionUuid = '516515c5-4b50-447b-8ca3-cbfce3f4d9f8';
```

### Step 4 — App service surface

[app/lib/services/ble_service.dart](../../app/lib/services/ble_service.dart):

1. Add a private field next to `_hwIdentityChar`:

   ```dart
   BluetoothCharacteristic? _firmwareVersionChar;
   ```

2. In the characteristic-discovery loop (around line 197):

   ```dart
   if (uuid == kFirmwareVersionUuid) _firmwareVersionChar = c;
   ```

3. Add a public method following the `readDeviceHardware` pattern (~line 234):

   ```dart
   /// Reads the firmware-version characteristic. Returns the version string
   /// (e.g. "v0.5.0") for ESP32-connected pedals, null for nRF52840-connected
   /// pedals or when disconnected (the char does not exist on nRF52840 yet —
   /// see TASK-358).
   Future<String?> readDeviceFirmwareVersion() async {
     final c = _firmwareVersionChar;
     if (c == null) return null;
     final bytes = await c.read();
     return utf8.decode(bytes);
   }
   ```

### Step 5 — Connected-Pedal screen

[app/lib/screens/connected_pedal_screen.dart](../../app/lib/screens/connected_pedal_screen.dart):

Replace the placeholder at line 57:

```dart
const _PendingRow(label: 'Firmware'),
```

with a row that:

- Calls `BleService.readDeviceFirmwareVersion()` on mount (FutureBuilder or equivalent).
- Renders the returned string for ESP32 pedals.
- Renders `—` when the read returns `null` (nRF52840 pedals, disconnected, or any read failure).

Match whatever pattern the Hardware row uses today — the two surfaces are structurally identical (READ characteristic → string → row), so reuse the Hardware-row widget if reasonable.

### Step 6 — Protocol doc

Already covered in Step 1 — add the `FIRMWARE_VERSION` row to [BLE_CONFIG_PROTOCOL.md → Characteristics](BLE_CONFIG_PROTOCOL.md#characteristics). No wire-format change (it is a plain READ), so no other section of that doc needs editing.

### Step 7 — Conventions

No new convention introduced. The change is a vanilla "add a READ characteristic returning a project-wide constant" — the same pattern `HW_IDENTITY` already uses.

### Step 8 — Host tests

The firmware-version string is just `#define FIRMWARE_VERSION "v0.5.0"`. No formatting logic to host-test. Skip.

(If TASK-354 grew to concatenate semver + git hash, that *formatter* would be shimmable and would land a host test under `test/unit/test_firmware_version.cpp`. The current scope does not warrant one.)

### Step 9 — On-device test

[test/test_ble_config_esp32/test_main.cpp](../../test/test_ble_config_esp32/test_main.cpp):

Follow the existing `kHwIdentityUuid` test pattern. Add a Unity test case that:

- Connects to the device under test (the runner handles this).
- Reads `kFirmwareVersionUuid`.
- Asserts the returned string matches the expected format (e.g. starts with `v` followed by digits + dots — keep the assertion regex-loose so a version bump doesn't break the test).

If the central-side runner ([test/test_ble_config_esp32/runner.py](../../test/test_ble_config_esp32/runner.py)) needs a new helper to perform the read, add it there. The TASK-354 AC explicitly calls out this folder.

### Step 10 — App-side widget test

[app/test/widget/connected_pedal_screen_test.dart](../../app/test/widget/connected_pedal_screen_test.dart):

Extend with a test that:

- Builds the Connected-Pedal screen with a mocked `BleService.readDeviceFirmwareVersion()` returning `"v0.5.0"`.
- Pumps the future and asserts the Firmware row text reads `v0.5.0`.
- Optionally a second test: mocked return `null` → row reads `—`.

Regenerate [app/test/widget/connected_pedal_screen_test.mocks.dart](../../app/test/widget/connected_pedal_screen_test.mocks.dart) because `BleService` gained a new method:

```bash
cd app && flutter pub run build_runner build --delete-conflicting-outputs
```

### Step 11 — End-to-end verification

In order, gated:

1. `/test` — host tests still pass (no new host tests were added; this is just a regression check).
2. `make esp32-build` — firmware builds clean.
3. Raw read with `bluetoothctl`:
   ```bash
   bluetoothctl connect $ASP_PEDAL_MAC
   gatttool -b $ASP_PEDAL_MAC --char-read --uuid=516515c5-4b50-447b-8ca3-cbfce3f4d9f8
   ```
   Returned bytes decode to the expected version string.
4. `/test-device esp32-ble-config` — the new Unity test passes against a flashed ESP32.
5. `/verify-on-device TASK-354 <scenario>` — drive the Pixel app, navigate to the Connected-Pedal page, confirm the Firmware row renders the live string.

### Step 12 — Sequencing

Single PR. Commits in order:

1. Firmware + protocol-doc row + catalog-row promotion (this guide).
2. App constant + service method + screen wiring + widget test (+ regenerated mocks).
3. On-device test extension.

The pre-commit hook gates host tests and formatting. The CI merge gate (see [DEVELOPMENT_SETUP.md → CI merge gate](DEVELOPMENT_SETUP.md#ci-merge-gate-branch-protection-on-main)) gates the rest.

## Conventions

Populated by [TASK-367](tasks/open/task-367-draft-conventions-invariants-gotchas.md). Folds in the content of the former `BLE_CONFIG_IMPLEMENTATION_NOTES.md`.

## Cross-cutting invariants

Populated by [TASK-367](tasks/open/task-367-draft-conventions-invariants-gotchas.md). The table lists, for each project-wide constant that spans layers (`MAX_CONFIG_BYTES`, MTU floor, endianness, version negotiation), the full set of files that must change together when it changes. This section exists specifically to stop the next [TASK-357](tasks/archive/v0.5.0/task-357-reconcile-max-config-bytes-doc-vs-code.md).

## Gotchas

Populated by [TASK-367](tasks/open/task-367-draft-conventions-invariants-gotchas.md). Folds in `BLE_CONFIG_IMPLEMENTATION_NOTES.md`'s "Challenge N" sections plus rationale mined from closed BLE tasks ([TASK-235](tasks/archive/v0.3.0/), TASK-357, the EPIC-026 cluster, IDEA-046) and commit messages.

## Tests

Populated by [TASK-368](tasks/open/task-368-draft-tests-references-and-verify.md).

## References

Populated by [TASK-368](tasks/open/task-368-draft-tests-references-and-verify.md).
