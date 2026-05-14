# BLE Services Developer Guide

This is the first thing you should read when adding or changing a BLE service or characteristic in this project. It collects, in one place, the catalog of services that exist today, the end-to-end recipe for adding or changing one, the conventions and cross-cutting invariants that span layers, the gotchas that we keep rediscovering, and pointers to the tests and external references.

**Status of related docs.** This guide is the entry point. Two pre-existing docs are kept and cross-linked from here rather than folded in:

- [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md) — authoritative byte-level wire format for the chunked-write protocol. Cited from the *Service catalog* and *Conventions* sections below.
- [BLE_READBACK_IMPACT.md](BLE_READBACK_IMPACT.md) — frozen-in-time feasibility analysis from [TASK-353](tasks/archive/v0.5.0/task-353-feasibility-firmware-ble-readback-surfaces.md) that gates the EPIC-026 readback cluster (TASK-354 / TASK-355 / TASK-356). Cited from *Service catalog*.

A third historical doc, [`BLE_CONFIG_IMPLEMENTATION_NOTES.md`](BLE_CONFIG_IMPLEMENTATION_NOTES.md), was folded into the *Conventions* and *Gotchas* sections of this guide ([TASK-367](tasks/closed/task-367-draft-conventions-invariants-gotchas.md)). Its content now lives here; the original file is preserved as a one-line redirect so the inbound links from [src/esp32/include/ble_keyboard_adapter.h](../../src/esp32/include/ble_keyboard_adapter.h) and historical archive task files (TASK-228 / TASK-229 / TASK-250) continue to resolve. Replacing the file with a redirect rather than `git rm`-ing it keeps EPIC-027 strictly docs-only — updating firmware comments was deemed out of scope.

## Contents

1. [Service catalog](#service-catalog)
2. [Recipe — add or change a service / characteristic](#recipe--add-or-change-a-service--characteristic)
3. [Worked example — firmware-version READ (TASK-354)](#worked-example--firmware-version-read-task-354)
4. [Conventions](#conventions)
5. [Cross-cutting invariants](#cross-cutting-invariants)
6. [Gotchas](#gotchas)
7. [Tests](#tests)
8. [References](#references)

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

The durable rules for adding or changing BLE services in this project. Each item is a one-line rule plus a pointer to a worked example or precedent in the codebase.

### UUID assignment

- **Service UUID** is `516515c0-4b50-447b-8ca3-cbfce3f4d9f8` — a randomly generated 128-bit UUID unique to this project. Defined in [src/esp32/src/ble_config_service.cpp:18](../../src/esp32/src/ble_config_service.cpp#L18), mirrored in [app/lib/constants/ble_constants.dart:21](../../app/lib/constants/ble_constants.dart#L21) and [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md#service-uuid). Do not introduce a second service UUID without a reason.
- **Characteristic UUIDs** increment the last byte of the service UUID: `516515cN-…` where `N` ranges over the characteristic index. Currently `c1`–`c4` are in firmware; `c5`/`c6` are planned in EPIC-026. Next free slot is `c7`. Stay inside this family — do not allocate from a separate UUID space.
- **Standard services (SIG)** are an exception. HID (`0x1812`) and DIS (`0x180A`) use SIG-assigned 16-bit UUIDs; we do not allocate inside our `516515cN` family for them.

### READ vs WRITE vs NOTIFY decision rule

Pick by *who reads, who writes, and how often*:

| Pattern | Use | Examples |
|---|---|---|
| Central reads a static-ish value | `READ` | `HW_IDENTITY` (board name), planned `FIRMWARE_VERSION` |
| Central pushes data to peripheral, one-shot | `WRITE` \| `WRITE_NR` | `CONFIG_WRITE`, `CONFIG_WRITE_HW` (chunked profile / hardware-config upload) |
| Peripheral pushes data to central as events occur | `NOTIFY` | `CONFIG_STATUS` (transfer progress / errors), planned `ACTIVE_PROFILE` |
| Central reads the latest value *and* subscribes for updates | `READ` \| `NOTIFY` | Planned `ACTIVE_PROFILE` (TASK-356) |

**Write characteristics: declare both `WRITE` and `WRITE_NR`.** BlueZ's D-Bus GATT backend uses `AcquireWrite` for write-without-response and `WriteValue` for write-with-response. `AcquireWrite` requires an established L2CAP channel and races MTU negotiation during connection setup; the resulting `org.bluez.Error.Failed: Failed to initiate write` is not recoverable with a post-connect delay. Declaring both properties lets BlueZ use the simpler `WriteValue` path (`response=True` in bleak), which is robust during setup. The extra ACK adds one round-trip per chunk — negligible at our chunk size. Pattern: [ble_config_service.cpp:67](../../src/esp32/src/ble_config_service.cpp#L67). The protocol spec ([BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md)) retains `WRITE_NO_RESPONSE` as the *canonical* property; `WRITE` is an implementation addition for BlueZ compatibility.

### Payload framing

- **Single-shot READ.** Value fits in one ATT response (≤ MTU − 1). String values are wrapped in `std::string(...)` before `setValue()` — see *Gotchas → NimBLE setValue templating* (TASK-235). Examples: `HW_IDENTITY`, planned `FIRMWARE_VERSION`.
- **NOTIFY.** Single packet per notification (≤ MTU − 3). Status strings are short — `READY` / `RESET` / `PROFILE:<name>` / `ERROR:<reason>` — and fit comfortably. Pattern: [ble_config_service.cpp:74](../../src/esp32/src/ble_config_service.cpp#L74).
- **Chunked WRITE.** Both `CONFIG_WRITE` and `CONFIG_WRITE_HW` use a single chunked-transfer protocol (`SEQ` / `LEN` / `PAYLOAD` framing) routed through one `BleConfigReassembler` instance. Wire format is authoritative in [BLE_CONFIG_PROTOCOL.md → Chunked Write Protocol](BLE_CONFIG_PROTOCOL.md#chunked-write-protocol). Implementation in [lib/PedalLogic/include/ble_config_reassembler.h](../../lib/PedalLogic/include/ble_config_reassembler.h) and [lib/PedalLogic/src/ble_config_reassembler.cpp](../../lib/PedalLogic/src/ble_config_reassembler.cpp).
- **Read Long.** Not used today. Any planned READ characteristic whose value exceeds MTU − 1 must either be split across characteristics, framed as chunked NOTIFY, or rely on BlueZ/NimBLE's automatic Read Long segmentation. Add a *Constants* row for the new max size if so.

### Max-size constants

- **`MAX_CONFIG_BYTES` = 16 384** (16 KB). Maximum reassembled JSON payload for `CONFIG_WRITE` and `CONFIG_WRITE_HW`. Defined as `constexpr std::size_t kMaxConfigBytes` in [lib/PedalLogic/include/ble_config_reassembler.h:30](../../lib/PedalLogic/include/ble_config_reassembler.h#L30) — **the header is authoritative**. Any disagreement with the protocol doc or app-side mirror is a bug; see [TASK-357](tasks/archive/v0.5.0/task-357-reconcile-max-config-bytes-doc-vs-code.md) and the *Cross-cutting invariants* table below.
- **`BLE_MTU` = 512**. ATT MTU; max chunk payload = MTU − 2 = 510 bytes. Documented in [BLE_CONFIG_PROTOCOL.md → Constants](BLE_CONFIG_PROTOCOL.md#constants). Both ESP32 and the app negotiate to this MTU; the iOS / Android client APIs will silently negotiate lower if their stack constrains it — the reassembler accepts any chunk size up to MTU − 2 and is not sensitive to the exact value.
- **`kJsonDocCapacity` = 49 152** (48 KB). Internal `ArduinoJson` document capacity used by the reassembler to parse `MAX_CONFIG_BYTES` of JSON with headroom. Internal to the parser; does not cross layers. Defined alongside `kMaxConfigBytes`.

### Version handling

There is no per-characteristic version negotiation today. The single project-wide version string is `FIRMWARE_VERSION` in [include/version.h](../../include/version.h) — exposed read-only over BLE on ESP32 when [TASK-354](tasks/open/task-354-firmware-version-read-characteristic.md) lands. Clients that need to feature-detect a new characteristic should attempt the read / discovery and fall back gracefully if the characteristic is absent.

If a wire-format breaking change is ever needed (it has not been), the convention is: allocate a *new* characteristic UUID at the next free `516515cN`, leave the old one in place for one release for backward compatibility, then drop the old in the following release. Do not version a characteristic in-place by reinterpreting its bytes.

### Error semantics

Wire-side errors are reported via the `CONFIG_STATUS` NOTIFY characteristic as `ERROR:<reason>` strings. Defined cases (authoritative in [BLE_CONFIG_PROTOCOL.md → Errors](BLE_CONFIG_PROTOCOL.md)):

| Reason | Trigger |
|---|---|
| `ERROR:too_large` | Reassembled payload exceeds `MAX_CONFIG_BYTES` |
| `ERROR:bad_seq` | Chunk sequence number out of order |
| `ERROR:parse_failed` | Reassembled bytes are not valid JSON for the expected schema |

When introducing a new error: add a row to the protocol doc, emit the same `ERROR:<reason>` string from the reassembler / handler, and handle the case app-side in `BleService`. Do not invent error codes outside the `ERROR:<reason>` convention.

### Stack choice

ESP32 uses **NimBLE** (via `ESP32-BLE-Keyboard` built with `-DUSE_NIMBLE`). Smaller RAM footprint than the classic Arduino ESP32 BLE stack and more actively maintained. nRF52840 uses **Bluefruit** (Adafruit's nRF52 Arduino BLE library), which is the only first-class option for the Feather nRF52840 hardware.

**Critical NimBLE timing rule.** When `USE_NIMBLE` is defined, `ble_gatts_start()` is called exactly once inside `BleKeyboard::begin()` — it atomically locks in every registered GATT service. Any service added *after* this point is silently ignored. The Config GATT service must be registered *before* HID advertising starts. The current architecture uses `HookableBleKeyboard::onStarted(BLEServer*)`, fired inside `BleKeyboard::begin()` after HID services start but before `adv->start()`. `BleConfigService::begin()` installs its GATT setup as that callback. **Always call `bleConfigService.begin()` before `bleKeyboardAdapter->begin()`** — see [main.cpp](../../src/esp32/src/main.cpp) and *Gotchas → ESP32 NimBLE GATT registration timing*.

### Security model

BLE pairing security is controlled by the `pairing_pin` field in the hardware config (`/config.json` on LittleFS):

- **`pairing_pin` absent or `null`** — no passkey, no encryption required on config characteristics. Any BLE client can connect and write without pairing. Used for development hardware, test fixtures, and the shipping default (rationale below).
- **`pairing_pin: <number>`** — `NimBLEDevice::setSecurityPasskey(pin)` is called in `HookableBleKeyboard::onStarted()` and passkey-entry auth is enabled. A client must complete the pairing ceremony before writing config data.

The shipping default is **no `pairing_pin`** ([data/config.json](../../data/config.json)). This was changed in the TASK-250 follow-up: the previous default of `12345` enabled DisplayOnly + MITM, but the pedal has no display, so Android/iOS hosts had no usable way to acquire the passkey and the in-app scan flow was unreachable. Builders who want passkey-entry auth opt in by adding `pairing_pin` — see [docs/builders/HARDWARE_CONFIG.md](../builders/HARDWARE_CONFIG.md#ble-pairing-optional).

The integration test fixture [test/test_ble_config_esp32/data/config.json](../../test/test_ble_config_esp32/data/config.json) also sets `"pairing_pin": null` so the automated runner can connect without pairing. The test firmware asserts `pairingEnabled == false` at startup and halts with a clear error if the wrong config was flashed.

### Architecture summary

```
Production path (src/esp32/src/main.cpp)
──────────────────────────────────────────
HookableBleKeyboard ──subclasses──▶ BleKeyboard (HID)
BleKeyboardAdapter  ──wraps──────▶ HookableBleKeyboard
BleConfigService::begin(IBleKeyboard*, …)
  └─ casts to BleKeyboardAdapter
  └─ installs onStarted callback
  └─ inside BleKeyboard::begin(): setupGattService(pServer)
  └─ pairingEnabled → setSecurityPasskey(pin) + passkey auth
  └─ pairingEnabled == false → open access (test configs)
```

`BleConfigService` does not inherit from anything BLE-related. It uses composition: it accepts an `IBleKeyboard*` and casts to `BleKeyboardAdapter` to install the `onStarted` hook. Composition was forced by a C++ return-type clash (see *Gotchas → BleKeyboard adapter quirks*).

## Cross-cutting invariants

The table lists, for each project-wide constant or convention that spans layers, the full set of files that must change together when it changes. This section exists specifically to stop the next [TASK-357](tasks/archive/v0.5.0/task-357-reconcile-max-config-bytes-doc-vs-code.md)-class miss (doc/code divergence on `MAX_CONFIG_BYTES`).

**Rule when changing any entry below: edit *every* file in the row, in the same commit.** Reviewers should reject a PR that touches one of these constants in fewer files than the row lists.

| Invariant | Authoritative source | Files that must change together |
|---|---|---|
| `MAX_CONFIG_BYTES` (reassembly buffer ceiling) | `kMaxConfigBytes` in [lib/PedalLogic/include/ble_config_reassembler.h:30](../../lib/PedalLogic/include/ble_config_reassembler.h#L30) | [lib/PedalLogic/include/ble_config_reassembler.h](../../lib/PedalLogic/include/ble_config_reassembler.h) (`kMaxConfigBytes`, `kJsonDocCapacity`) · [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md#constants) (Constants table + `ERROR:too_large` description) · [app/lib/constants/ble_constants.dart](../../app/lib/constants/ble_constants.dart) (`kMaxConfigBytes`) · existing boundary tests under `test/unit/test_ble_config_*` (the `too_large` assertion mirrors this constant) |
| `BLE_MTU` / chunk-size ceiling (ATT MTU) | [BLE_CONFIG_PROTOCOL.md → Constants](BLE_CONFIG_PROTOCOL.md#constants) | [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md) (Constants + Chunked Write framing) · [app/lib/services/ble_service.dart](../../app/lib/services/ble_service.dart) (chunk size = MTU − 2 = 510) · [lib/PedalLogic/src/ble_config_reassembler.cpp](../../lib/PedalLogic/src/ble_config_reassembler.cpp) (per-chunk size validation if any) · `test/test_ble_config_esp32/test_main.cpp` (chunked-write tests) |
| Service UUID (`516515c0-…`) | [src/esp32/src/ble_config_service.cpp:18](../../src/esp32/src/ble_config_service.cpp#L18) | [src/esp32/src/ble_config_service.cpp](../../src/esp32/src/ble_config_service.cpp) (`SERVICE_UUID`) · [app/lib/constants/ble_constants.dart](../../app/lib/constants/ble_constants.dart) (`kServiceUuid`) · [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md#service-uuid) · this guide's *Service catalog* |
| `FIRMWARE_VERSION` (project-wide version string) | [include/version.h](../../include/version.h) | [include/version.h](../../include/version.h) · root `package.json` (mirrored, bumped by `/release`) · `app/pubspec.yaml` · `awesome-task-system/pyproject.toml` (mirrored by `/release`) · [CHANGELOG.md](../../CHANGELOG.md) (release entry) · *(planned, post-TASK-354)* [src/esp32/src/ble_config_service.cpp](../../src/esp32/src/ble_config_service.cpp) (firmware-version READ char), [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md) (UUID row), [app/lib/constants/ble_constants.dart](../../app/lib/constants/ble_constants.dart) (`kFirmwareVersionUuid`) |
| `kPedalNamePrefix` (`AwesomeStudio`) — BLE advertised-name match prefix | [app/lib/constants/ble_constants.dart:19](../../app/lib/constants/ble_constants.dart#L19) | [app/lib/constants/ble_constants.dart](../../app/lib/constants/ble_constants.dart) · `src/esp32/src/ble_keyboard_adapter.cpp` (`HookableBleKeyboard("AwesomeStudioPedal", …)`) · [test/test_ble_config_esp32/runner.py](../../test/test_ble_config_esp32/runner.py) (name-prefix discovery — see *Gotchas → BlueZ HID daemon*) |
| `pairing_pin` semantics (open vs passkey-entry) | [src/esp32/src/ble_config_service.cpp](../../src/esp32/src/ble_config_service.cpp) (`pairingEnabled` derivation) | `src/esp32/src/ble_config_service.cpp` (pairing setup) · `src/esp32/src/ble_keyboard_adapter.cpp` (`HookableBleKeyboard::onStarted` security passkey) · [data/config.json](../../data/config.json) (shipping default — currently `null`) · [test/test_ble_config_esp32/data/config.json](../../test/test_ble_config_esp32/data/config.json) (test fixture — `null`) · [docs/builders/HARDWARE_CONFIG.md](../builders/HARDWARE_CONFIG.md#ble-pairing-optional) (builder-facing docs) |

When adding a new cross-cutting invariant, add a row here as part of the same change. If the constant has fewer than three sites that must agree, it does not need to be in this table.

## Gotchas

Platform quirks and hard-earned rationale. Each item starts with the *symptom*, then *cause*, then *resolution* — so a future debugger can grep for the symptom they are seeing.

### ESP32 NimBLE GATT registration timing

**Symptom**: device advertises, but BLE scans find no matching service UUID; characteristics are absent in `bluetoothctl gatt list-attributes`.

**Cause**: `BleConfigService::begin()` was called after `bleKeyboardAdapter->begin()`. NimBLE calls `ble_gatts_start()` once inside `BleKeyboard::begin()` to atomically lock in the GATT table; any service added after this point is silently ignored.

**Resolution**: `HookableBleKeyboard` exposes a virtual `onStarted(BLEServer*)` hook that fires *inside* `begin()`, after HID services start but before `adv->start()`. `BleConfigService::begin()` installs its GATT setup code as that callback. **Always call `bleConfigService.begin()` before `bleKeyboardAdapter->begin()`.** Pattern in [src/esp32/src/main.cpp](../../src/esp32/src/main.cpp).

### BleKeyboard adapter quirks (multiple-inheritance return-type clash)

**Symptom**: compiler rejects `class BleKeyboardAdapter : public BleKeyboard, public IBleKeyboard` with "conflicting return types".

**Cause**: `BleKeyboard` inherits `Print`, whose `write(uint8_t)` returns `size_t`. `IBleKeyboard::write(uint8_t)` returns `void`. C++ disallows changing return type when overriding.

**Resolution**: composition, not inheritance. `HookableBleKeyboard` subclasses only `BleKeyboard` and adds the `onStarted` hook. `BleKeyboardAdapter` implements only `IBleKeyboard` and holds a `HookableBleKeyboard` reference, delegating calls. Pattern in [src/esp32/include/ble_keyboard_adapter.h](../../src/esp32/include/ble_keyboard_adapter.h).

### NimBLE setValue templating (string vs pointer)

**Symptom**: a READ characteristic with a string value returns 4 or 8 bytes that look like garbage; central reads the pointer bytes, not the string contents.

**Cause**: NimBLE's `setValue<T>(const T&)` template, when called with `T = const char*`, stores `sizeof(pointer)` bytes (the address itself) instead of the string contents. The const-char-pointer specialisation does *not* `strlen()` the C string.

**Resolution**: wrap the value in `std::string(...)` before `setValue()` — `std::string` has `c_str()` and `length()`, so it takes the string-specific overload and stores the actual bytes. Established in TASK-235; pattern in [ble_config_service.cpp:83](../../src/esp32/src/ble_config_service.cpp#L83) (HW_IDENTITY). Apply the same wrap to any new READ characteristic returning a string.

### BlueZ HID daemon (historical, resolved)

**Symptom (historical)**: integration test runner connected to the test firmware, BlueZ disconnected immediately with `BLE_ERR_REM_USER_CONN_TERM` (reason code `0x24`) before any GATT op completed.

**Cause**: BlueZ's HID plugin (`bluetoothd` with HID) auto-connects to any device advertising HID UUID (`0x1812`) and tries to read encrypted HID characteristics without pairing. NimBLE returned `Insufficient Authentication`; BlueZ terminated. The issue only surfaced in the test runner, because the test runner used UUID-only discovery filtering on the Config service UUID — production firmware does not advertise the Config UUID (only HID + device name). A separate HID-less test firmware existed to work around this and created the disconnect path.

**Resolution (TASK-236)**: the test runner switched to name-prefix discovery (matching `kPedalNamePrefix`), the same mechanism the app and CLI use. With name-prefix discovery the runner connects to production firmware directly, and BlueZ's HID daemon does not interfere in practice. The HID-less test firmware and all guards were removed.

**Practical rule today**: any new BLE-client code (tests, CLI, in-app) discovers the pedal by name prefix, not by UUID. The Config service UUID is *not* advertised — it is discoverable post-connect via GATT service discovery, but you cannot filter the pre-connect scan on it.

### BlueZ GATT cache stale entries

**Symptom**: after changing the firmware's GATT layout (adding / removing a characteristic), writes / reads to the new layout fail with handle-mapping errors. The cache from the previous firmware version is wrong.

**Resolution**: `bluetoothctl remove <addr>` clears BlueZ's cached device entry. Run this once when the firmware GATT layout changes significantly. The test runner does not need to do this automatically — after the cache is cleared, subsequent runs work without intervention.

### Test-runner serial-line race (PROFILE vs RESET)

**Symptom**: integration test runner expects `[BLE_TEST] RESET` after sending the RESET command; an unrelated `[BLE_TEST] PROFILE:<name>` line arrives first; the assertion fails.

**Cause**: the test firmware prints `[BLE_TEST] PROFILE:<name>` whenever the active-profile index changes. Spontaneous profile changes during the test interleave with the runner's expected response line.

**Resolution**: `read_serial_line` in [test/test_ble_config_esp32/runner.py](../../test/test_ble_config_esp32/runner.py) accepts an optional `keyword` parameter (default `"[BLE_TEST]"`). Callers that expect a specific tag pass the exact string — e.g. `keyword="[BLE_TEST] RESET"` for the reset-acknowledged check, `keyword="[BLE_TEST] PROFILE:"` for the PROFILE? response. Spontaneous lines from other tags are simply skipped.

### Test-runner missing READY line (serial-port-after-boot race)

**Symptom**: the runner times out waiting 30 s for `[BLE_TEST] READY`, but the firmware booted and printed READY long before the runner opened the port.

**Cause**: the Makefile runs `pio upload` then immediately launches `runner.py`. The ESP32 boots in ~2 s and prints READY; the runner sometimes opens the serial port after that line has already been sent.

**Resolution**: `runner.py` pulses DTR low → high after opening the serial port. This triggers an ESP32 hardware reset via the USB-to-UART chip's DTR line, producing a fresh boot with READY visible from the start of the serial stream.

### Test-runner active-profile reporting (PROFILE? on demand)

**Symptom**: after upload, the test runner cannot determine the active profile — the spontaneous `[BLE_TEST] PROFILE:<name>` line fires on the *first* `loop()` tick, long before the upload completes.

**Resolution**: after a successful upload, the runner sends a `PROFILE?` command on the serial line. The firmware handles `PROFILE?` in `loop()` by printing the current profile on demand, making the check deterministic regardless of when the spontaneous line fired.

### Persistence across soft-reset (profiles not loaded on boot)

**Symptom**: after a soft-reset in a persistence test, `getProfile(0)` returns `nullptr` — the profile manager is empty.

**Cause**: the firmware reboots but does not load profiles from LittleFS at boot. If `configureProfiles()` is not called from `setup()`, no profiles are populated.

**Resolution**: `configureProfiles(*pm, nullptr)` is called in `setup()`. This reads `profiles.json` from LittleFS (written by `BleConfigReassembler` during the previous upload). Profile-index persistence across reboot uses ESP32 `Preferences` (NVS).

### Build-system smell — duplicate source paths

**Symptom (open question)**: a session repeatedly copies `ble_config_service.cpp` between locations, alternating with on-device test runs. Suggests the test build is not picking up edits to the canonical source automatically.

**Status**: filed as [IDEA-046](ideas/open/idea-046-ble-config-cpp-copy-loop-investigation.md), root cause not yet diagnosed. Candidates: wrong source root in `platformio.ini`, a stray copy under `test/`, a missing symlink. Do not scaffold a fix until the root cause is identified.

**Practical rule for now**: if you find yourself manually copying a `.cpp` between locations to make the test build pick up your edits, stop and read [IDEA-046](ideas/open/idea-046-ble-config-cpp-copy-loop-investigation.md) — your situation is the one the idea is asking to investigate.

### Parallel-session test interference

**Symptom**: an on-device BLE test passes in isolation but fails when another Claude Code session is also driving BLE in the same workspace. Two sessions can race for the same pedal MAC.

**Resolution**: the pedal MAC is single-resource. Coordinate manually — there is no automated locking. The `$ASP_PEDAL_MAC` env var is the canonical handle; if you suspect interference, run [/ble-reset](../../.claude/skills/ble-reset/SKILL.md) to recover from a flaky pairing state and confirm no other session is actively talking to the device.

## Tests

Pointers to BLE-related tests on each layer, with the decision rule for picking the right layer when adding a new test.

### Decision rule — host or on-device?

Per [CLAUDE.md → Testing policy](../../CLAUDE.md#testing-policy): prefer the host layer when the hardware dependency can be shimmed. A BLE-side change is shimmable iff its value or behaviour comes from project-internal state (constants, parsers, formatters, the reassembler) rather than from a live GATT peer.

Worked examples drawn from existing tests:

| Change | Shimmable? | Test layer |
|---|---|---|
| Reassembler `MAX_CONFIG_BYTES` boundary (`too_large` error) | Yes — bytes-in / bytes-out, no BLE stack | Host — [test/unit/test_ble_config_service.cpp](../../test/unit/test_ble_config_service.cpp) |
| Chunk-size chunking on the app side | Yes — purely splits a payload into chunks | App unit — [app/test/unit/ble_service_chunk_size_test.dart](../../app/test/unit/ble_service_chunk_size_test.dart) |
| Pre-connect scan name-prefix filter | Yes — string match on a mocked scan result | App unit — [app/test/unit/ble_service_scan_filter_test.dart](../../app/test/unit/ble_service_scan_filter_test.dart) |
| End-to-end upload (chunks → reassembler → profile installed) | No — needs the firmware's BLE stack | On-device — [test/test_ble_config_esp32/](../../test/test_ble_config_esp32/) |
| Pairing PIN ↔ open-access switching | No — needs NimBLE security manager | On-device — [test/test_ble_pairing_esp32/](../../test/test_ble_pairing_esp32/) |
| Connected-Pedal page rendering with mocked `BleService` | Yes — Flutter widget tests mock the service | App widget — [app/test/widget/connected_pedal_screen_test.dart](../../app/test/widget/connected_pedal_screen_test.dart) |

### Host layer (no hardware)

- **Location**: [test/unit/](../../test/unit/) — GoogleTest sources guarded by `HOST_TEST_BUILD` and the shims in [test/fakes/arduino_shim.h](../../test/fakes/arduino_shim.h).
- **BLE-relevant file**: [test/unit/test_ble_config_service.cpp](../../test/unit/test_ble_config_service.cpp) — covers the reassembler bytes-in / bytes-out boundary including `MAX_CONFIG_BYTES`.
- **Run**: `make test-host` (or `/test`). No hardware required.
- **CMake registration**: add new sources to the `pedal_tests` target in [test/CMakeLists.txt](../../test/CMakeLists.txt).

### ESP32 on-device layer

- **Location**: [test/test_*_esp32/](../../test/) — PlatformIO + Unity, each folder is an independent test environment with a `test_main.cpp` and (where relevant) a Python `runner.py` that drives a Linux/BlueZ central.
- **BLE-relevant folders**:
  - [test/test_ble_config_esp32/](../../test/test_ble_config_esp32/) — end-to-end config-upload tests. `test_main.cpp` is the firmware-side fixture; [runner.py](../../test/test_ble_config_esp32/runner.py) drives reads/writes from the host via BlueZ; [data/config.json](../../test/test_ble_config_esp32/data/config.json) is the LittleFS test fixture (must have `pairing_pin: null`).
  - [test/test_ble_pairing_esp32/](../../test/test_ble_pairing_esp32/) — pairing-PIN behaviour smoke test.
- **Run**: `make test-esp32-ble-config` and `make test-esp32-ble-pairing` (or the parameterised `/test-device esp32-ble-config`). Requires a USB-connected ESP32 (`$ASP_ESP32_PORT`) and BlueZ on the host. Set `PORT=` to override the device port.
- **Recovery tools**: [/ble-reset](../../.claude/skills/ble-reset/SKILL.md) for flaky pairing state, [`bluetoothctl remove <addr>`](#bluez-gatt-cache-stale-entries) for stale GATT cache after firmware-layout changes.

### nRF52840 on-device layer

No BLE-specific test environment today. The nRF52840 BLE surface is HID-only (see *Service catalog → nRF52840*); custom GATT comes with [TASK-358](tasks/paused/task-358-nrf52840-ble-readback-surfaces.md) and a matching test environment will land alongside.

### App-side layer (Flutter)

- **Location**: [app/test/unit/](../../app/test/unit/) (pure unit tests) and [app/test/widget/](../../app/test/widget/) (widget + integration tests with mocked services).
- **BLE-relevant unit tests**:
  - [ble_service_upload_test.dart](../../app/test/unit/ble_service_upload_test.dart) — chunked upload path.
  - [ble_service_upload_catch_test.dart](../../app/test/unit/ble_service_upload_catch_test.dart) — error-handling path.
  - [ble_service_chunk_size_test.dart](../../app/test/unit/ble_service_chunk_size_test.dart) — chunk-size invariant (MTU − 2 = 510).
  - [ble_service_scan_filter_test.dart](../../app/test/unit/ble_service_scan_filter_test.dart) — name-prefix scan filter (`kPedalNamePrefix`).
- **BLE-relevant widget tests**:
  - [connected_pedal_screen_test.dart](../../app/test/widget/connected_pedal_screen_test.dart) (+ [`.mocks.dart`](../../app/test/widget/connected_pedal_screen_test.mocks.dart)) — Connected-Pedal page rendering with mocked `BleService` reads.
  - [connection_details_sheet_test.dart](../../app/test/widget/connection_details_sheet_test.dart), [connection_status_strip_test.dart](../../app/test/widget/connection_status_strip_test.dart) — peripheral connection state UI.
- **Run**: `make test-flutter` (or `cd app && flutter test`). Mocks under `*.mocks.dart` are regenerated with `flutter pub run build_runner build --delete-conflicting-outputs` when you change a service surface.

### Integration / end-to-end (on a real device)

For features that span firmware + app, use [/verify-on-device `<TASK-ID>` `<SCENARIO-ID>`](../../.claude/skills/verify-on-device/SKILL.md) to drive the Pixel-connected app against a real pedal. Scenarios live in the skill's catalog; one example flow exercising a BLE READ surface end-to-end (Connected-Pedal page firmware row) is the planned TASK-354 verification scenario.

## References

External standards and library docs (cite, do not copy):

- **Bluetooth SIG specifications** — <https://www.bluetooth.com/specifications/> for ATT, GATT, GAP, MTU, pairing. Specifically:
  - HID over GATT (`0x1812`): <https://www.bluetooth.com/specifications/specs/hids-1-0/>
  - Device Information Service (`0x180A`, planned for nRF52840 via Bluefruit `BLEDis`): <https://www.bluetooth.com/specifications/specs/device-information-service-1-1/>
- **NimBLE-Arduino** (ESP32 stack) — <https://github.com/h2zero/NimBLE-Arduino>. Reference for `NIMBLE_PROPERTY::*` flags, `NimBLECharacteristic::setValue` overloads, `setSecurityPasskey`, and the `ble_gatts_start()` atomic-locking behaviour cited in *Gotchas*.
- **ESP32-BLE-Keyboard** (fork used here) — provides `BleKeyboard` and the `HookableBleKeyboard` `onStarted(BLEServer*)` extension this project relies on for late GATT registration.
- **Adafruit Bluefruit nRF52 Arduino** (nRF52840 stack) — <https://github.com/adafruit/Adafruit_nRF52_Arduino>. Reference for `Bluefruit.begin()`, `BLEService` / `BLECharacteristic`, `BLEDis`. Relevant when [TASK-358](tasks/paused/task-358-nrf52840-ble-readback-surfaces.md) unblocks.
- **bleak** (Python BLE client used by `runner.py`) — <https://github.com/hbldh/bleak>. Reference for `write_gatt_char(..., response=True/False)` semantics (the `WRITE` vs `WRITE_NR` rule in *Conventions* maps onto bleak's `response` parameter).
- **BlueZ** — Linux Bluetooth stack used by both `bluetoothctl` and bleak. Reference for the D-Bus GATT interface (`AcquireWrite` vs `WriteValue`) and the HID daemon behaviour cited in *Gotchas*.

Internal project history (the rationale behind specific conventions and gotchas):

- [TASK-235](tasks/archive/v0.3.0/) — NimBLE `setValue<T>` template behaviour with `const char*`; the `std::string(...)` wrap rule.
- [TASK-236](tasks/archive/v0.4.0/) — BlueZ HID-daemon disconnect; resolution by switching the test runner to name-prefix discovery.
- [TASK-240](tasks/archive/v0.4.0/task-240-defect-firmware-json-parser-undersized.md) — firmware JSON parser undersized vs `MAX_CONFIG_BYTES`; underwrote the `kJsonDocCapacity` headroom rule.
- [TASK-250](tasks/archive/v0.4.0/task-250-defect-android-manifest-missing-ble-permissions.md) — `pairing_pin` default change (DisplayOnly+MITM → open access).
- [TASK-353](tasks/archive/v0.5.0/task-353-feasibility-firmware-ble-readback-surfaces.md) and the EPIC-026 cluster ([TASK-354](tasks/open/task-354-firmware-version-read-characteristic.md), [TASK-355](tasks/open/task-355-firmware-config-readback.md), [TASK-356](tasks/open/task-356-firmware-active-profile-notify.md)) — readback-surfaces feasibility and the ESP32-ships / nRF52840-deferred split.
- [TASK-357](tasks/archive/v0.5.0/task-357-reconcile-max-config-bytes-doc-vs-code.md) — `MAX_CONFIG_BYTES` doc/code reconciliation; motivated the *Cross-cutting invariants* table.
- [TASK-358](tasks/paused/task-358-nrf52840-ble-readback-surfaces.md) — bundled nRF52840 readback work, paused on hardware availability.
- [IDEA-046](ideas/open/idea-046-ble-config-cpp-copy-loop-investigation.md) — open question about the duplicate-source-paths smell on the test build.
- [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md) — authoritative wire format for the chunked-write protocol.
- [BLE_READBACK_IMPACT.md](BLE_READBACK_IMPACT.md) — frozen TASK-353 feasibility analysis with the platform-asymmetry argument.
