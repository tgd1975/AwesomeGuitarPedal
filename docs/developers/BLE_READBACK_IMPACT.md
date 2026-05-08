# BLE Readback Surfaces — Feasibility & Impact Analysis

Source task: [TASK-353](tasks/active/task-353-feasibility-firmware-ble-readback-surfaces.md). Gates [TASK-354](tasks/open/task-354-firmware-version-read-characteristic.md), [TASK-355](tasks/open/task-355-firmware-config-readback.md), [TASK-356](tasks/open/task-356-firmware-active-profile-notify.md).

## TL;DR — recommended scope

| Deliverable | ESP32 | nRF52840 | Notes |
|---|---|---|---|
| Firmware-version read | Custom char in `BleConfigService` (UUID `…5c5`) | **Deferred to TASK-358** (planned path: Bluefruit `BLEDis` / DIS 0x180A) | Hardware not currently available — write-only-and-pray rejected |
| **DIS (0x180A) decision** | **Skip** | **Bundle (via `BLEDis`) — TASK-358** | Mixed-mechanism plan stands; activation gated on device |
| **Config readback option** | **(a) make existing char readable, retain JSON in RAM** | **Deferred to TASK-358** (no BLE config service exists yet) | nRF52840 lacks even config-write today |
| Active-profile notify | Read+notify char in `BleConfigService` (UUID `…5c6`) | **Deferred to TASK-358** | Same nRF52840 infra gap |

The platform asymmetry below is the load-bearing finding. **All three deliverables are ESP32-only for this iteration** — nRF52840 hardware is not currently available for verification, and shipping untested firmware-only changes (even cheap ones like `BLEDis`) is rejected. The full nRF52840 readback story is bundled into [TASK-358](tasks/open/task-358-nrf52840-ble-readback-surfaces.md), gated on hardware availability.

## 1. Platform asymmetry — the elephant in the room

The original TASK-337 packaged "ESP32 + nRF52840" as a flat per-deliverable cost. It isn't:

- **ESP32** has [`BleConfigService`](../../src/esp32/src/ble_config_service.cpp) — a registered NimBLE custom GATT service hosting four characteristics (`CONFIG_WRITE`, `CONFIG_WRITE_HW`, `CONFIG_STATUS`, `HW_IDENTITY`). Adding a new char is mechanically a single `svc->createCharacteristic(...)` call inside [`setupGattService()`](../../src/esp32/src/ble_config_service.cpp#L55) plus, where relevant, hooking the existing reassembler / status-notify plumbing.
- **nRF52840** has **no custom GATT service.** [`BleKeyboardAdapter::begin()`](../../src/nrf52840/src/ble_keyboard_adapter.cpp#L10) is HID-only — `Bluefruit.begin()` → `hid.begin()` → `Bluefruit.Advertising.start(0)`. Config write does not exist; there is no per-pedal NVS-equivalent for the active profile (`Nrf52840PedalApp::saveProfile()` is a no-op, and the device always boots to profile 0).

This means the per-platform impact for **every** TASK-354/355/356 deliverable on nRF52840 starts with "scaffold a custom Bluefruit GATT service" — which is multi-hour infra work that the original task scopings did not account for. A 30-line ESP32 change becomes a 200+ line nRF52840 change with the bulk of the cost being plumbing the Bluefruit-side service registration, not the new characteristic itself.

The honest scope is therefore: **ship the readback surfaces on ESP32 now, defer the equivalent nRF52840 work behind a separate "should nRF52840 have a custom BLE service?" question.** Firmware-version is the single exception, because nRF52840's Bluefruit ships [`BLEDis`](https://github.com/adafruit/Adafruit_nRF52_Arduino) — the standard 0x180A Device Information Service — as a four-line drop-in.

## 2. nRF52840 RAM/flash headroom (measured)

Measured against `.pio/build/feather-nrf52840/firmware.elf` (built 2026-04-29):

```
section             size        addr
.text             183 648      0x00026000   (flash, code)
.data                 964      0x20006000   (RAM, init values)
.bss               14 700      0x20006304   (RAM, zero-init statics)
.heap             219 856      0x20009D30   (RAM, reserved heap region)
```

- **Flash use**: 183 648 + 964 = **184 KB** used out of 1 024 KB total (nRF52840 has 1 MB flash; SoftDevice S140 v7 occupies ~150 KB of that, bootloader ~28 KB → ~820 KB available for application). Plenty of headroom — adding 1–2 small characteristics + DIS strings is < 2 KB code.
- **RAM use**: SoftDevice + MBR consume RAM 0x20000000 → 0x20006000 = **24 KB**. Application static use (`.data + .bss`) = **15.6 KB**. The linker reserves 215 KB as a `.heap` section that runs to the stack at the top of RAM (~0x20040000). Practical free heap budget: **~215 KB**.
- **Headroom for option (a)**: a resident 16 KB JSON copy consumes ~7 % of available heap. Well within budget.
- **Headroom for any new BLE characteristic**: each Bluefruit `BLECharacteristic` instance is roughly 100–150 bytes RAM; SoftDevice GATT-table entries ~50 bytes each. Negligible against 215 KB.

**Threshold to verify in TASK-355**: ≥ 200 KB free heap after adding the resident-config copy. (Static use rises by ~16 KB → ~32 KB; heap region shrinks by the same amount.)

## 3. Per-deliverable analysis

### 3.1 Firmware-version read characteristic — TASK-354

Source: `FIRMWARE_VERSION "v0.4.1"` from [include/version.h:7](../../include/version.h#L7) (already project-wide; identical on both platforms; not currently exposed via BLE).

**ESP32 implementation sketch.** Add to `setupGattService()` next to `CHAR_HW_IDENTITY_UUID`:

```cpp
static const char* CHAR_FW_VERSION_UUID = "516515c5-4b50-447b-8ca3-cbfce3f4d9f8";
NimBLECharacteristic* fwChar =
    svc->createCharacteristic(CHAR_FW_VERSION_UUID, NIMBLE_PROPERTY::READ);
fwChar->setValue(std::string(FIRMWARE_VERSION));
```

3 lines. Risk: none — read-only, no security implications, no bond required. UUID `…5c5` continues the family established in [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md#characteristics).

**nRF52840 implementation sketch (deferred to TASK-358 — hardware not available).** When hardware is back, use Bluefruit's bundled `BLEDis`:

```cpp
#include <bluefruit.h>
BLEDis bledis;
// in BleKeyboardAdapter::begin(), after Bluefruit.begin():
bledis.setManufacturer("AwesomeStudioPedal");
bledis.setModel("Strix-Pedal nRF52840");
bledis.setFirmwareRev(FIRMWARE_VERSION);
bledis.begin();
// then continue with hid.begin() …
```

DIS (0x180A) is the standard Bluetooth GATT Device Information Service. Generic BLE explorers (nRF Connect, LightBlue) auto-recognise the Firmware Revision characteristic. Cost is roughly the lines above + service-table slot + ~200 B RAM for the BLEDis instance.

**DIS bundle/skip decision (recommended).**

- **Bundle on nRF52840**: yes. Bluefruit ships `BLEDis` for free; the alternative is a custom char inside a service that doesn't exist yet (worse on every axis).
- **Skip on ESP32**: yes. The ESP32 already has a custom-UUID firmware-version char in `BleConfigService`; adding a parallel DIS service to expose the same string would be duplication. The Connected-Pedal page reads from the custom char on ESP32 anyway (it does not need DIS). DIS on ESP32 would only matter if generic third-party BLE tools became a target user, which is not on the roadmap.

This **mixed-mechanism choice** is deliberate. The Connected-Pedal page must read `firmware-version` regardless of platform; the app gets the value from whichever characteristic the connected pedal exposes. Both surfaces return the same string.

**Test strategy.**

- ESP32: extend [`test/test_ble_config_esp32/`](../../test/test_ble_config_esp32/) — read `…5c5`, assert it equals `FIRMWARE_VERSION`. On-device, hardware-bound.
- nRF52840: light on-device check via a generic BLE explorer or a small additional fixture; assert DIS Firmware Rev string round-trip. The version-string formatting itself (if any post-processing is added later, e.g. semver + git hash) is host-testable behind `HOST_TEST_BUILD`, but today the value is a literal `#define`.

**Recommendation: GO on ESP32. Defer nRF52840 to TASK-358** until hardware is available — the four-line `BLEDis` integration is cheap to write but cannot be verified today, and shipping untested firmware-only changes is the kind of "we'll find out later" that bites.

### 3.2 Config readback — TASK-355

Two competing options:

- **(a)** Make `CHAR_WRITE_UUID` readable (add `READ` property) and retain the canonical JSON resident in RAM after a successful upload. Reads return that buffer.
- **(b)** Add a separate "current config" read characteristic that streams the active config back via the same chunked-reassembler protocol used for uploads, just inverted. No resident copy — rebuilt from `/profiles.json` on LittleFS on each read.

#### Option (a) — resident copy

**ESP32 implementation sketch.**

1. Promote `CHAR_WRITE_UUID` to `WRITE_NR | WRITE | READ` in `setupGattService()`.
2. After successful `BleConfigReassembler::applyTransfer()`, copy the reassembled buffer into a static `std::string lastAppliedConfig_` before discarding it.
3. Add a NimBLE read callback (or `setValue` after each successful upload) that returns `lastAppliedConfig_`.

Cost: ~30 LOC. RAM: up to 16 KB (`MAX_CONFIG_BYTES` from [ble_config_reassembler.h:30](../../lib/PedalLogic/include/ble_config_reassembler.h#L30)). The reassembler already buffers the same bytes during transfer; option (a) merely declines to free that allocation.

**MTU pressure.** A single BLE read returns the ATT MTU's worth of bytes (510 B payload). Reads of payloads > MTU require client-side iteration via `read offset` (NimBLE supports this natively). The Flutter app's `flutter_blue_plus` exposes offset reads. Acceptable.

**Race conditions.** The resident copy must be replaced atomically — either swap a pointer or guard with a mutex around the `applyTransfer` callback. NimBLE's read callback runs on the BLE host task; concurrent writes to the same buffer from the application thread would race. Recommended: build the new buffer in a temporary, then swap.

#### Option (b) — inverted reassembler, no resident copy

**ESP32 implementation sketch.** New read char `CHAR_CURRENT_CONFIG_UUID`. Read handler streams from `/profiles.json` via [`IFileSystem`](../../lib/PedalLogic/include/i_file_system.h) using the same 510-byte chunk size and `seq` framing as upload, with the receiver acknowledging via `CONFIG_STATUS` notifications.

Cost: ~150 LOC firmware + matching app-side reassembler. Round-trip equality test is more complex (sequence-aware). No resident copy.

**Risk.** Two independent protocol paths sharing UUID semantics is a maintenance burden. The chunked-write protocol's edge cases (out-of-order, mid-transfer errors) all need symmetric handling on the read side.

#### Decision (recommended)

**Pick option (a).** Rationale:

- **Resident-copy memory cost is non-binding on the actual measured budget.** 16 KB out of 215 KB free heap on nRF52840 is 7 %. On ESP32 the budget is even more generous (PSRAM available; 320 KB SRAM core).
- **Code surface is 5× smaller.** ~30 LOC vs ~150 LOC. The original task body's preference for option (b) ("avoids the resident copy") was hedging against a budget pressure that the actual measurement contradicts.
- **Test strategy is materially simpler for option (a).** Round-trip equality is "write, read, assert byte-equal at JSON level". Option (b)'s test must exercise the inverted-reassembler protocol — more code per assertion.
- **Operational invariant aligns with option (a).** The firmware already round-trips through `ConfigLoader::loadFromString` then writes to LittleFS; keeping the source bytes in memory is consistent with "the config the firmware is currently running", which is what the Connected-Pedal page needs to display. Option (b) reads from disk, which could disagree with the in-memory `ProfileManager` state during the brief window between apply and disk-write.

#### nRF52840 deferral

Both options presuppose a custom GATT service that doesn't exist on nRF52840. Implementing config-write from scratch is well outside TASK-355's "Medium" sizing.

**Recommendation: implement option (a) on ESP32 only. Defer nRF52840 config-readback** until a separate task scopes "should nRF52840 expose a custom BLE config service?" That decision has its own product weight (does the user ever configure an nRF52840 pedal over BLE today? — no) and shouldn't be made under TASK-355's Medium budget.

The Connected-Pedal page already needs to handle "Configuration row not available" for nRF52840 — show "—" with the existing pedal-platform-aware widget pattern.

**Test strategy.**

- ESP32 on-device: extend `test/test_ble_config_esp32/` with a write→read round-trip equality test against the example configs in `profiles/`.
- Host: option (a) is mostly covered by the existing reassembler tests; the only new logic is the "retain after apply" step, which is one-line behaviour.

**Risk.**

- **MTU pressure** on offset-reads of 16 KB blobs — the app must iterate; verify on Pixel 9 with `flutter_blue_plus`.
- **Bond/security** — same as upload (passkey-bonded if `pairing_pin` set, else open). No new surface.
- **Memory regression** on ESP32 — 16 KB on a 320 KB core; measure via `pio run -e nodemcu-32s --target size` before/after.

### 3.3 Active-profile-index notify characteristic — TASK-356

Single-byte payload: current index from [`ProfileManager::currentProfile`](../../lib/PedalLogic/include/profile_manager.h#L114).

**ESP32 implementation sketch.** Add `CHAR_ACTIVE_PROFILE_UUID` (`…5c6`) with `READ | NOTIFY` to `setupGattService()`. Hook [`ProfileManager::switchProfile`](../../lib/PedalLogic/include/profile_manager.h#L64) and `setCurrentProfile` to call `activeProfileChar_->setValue(idx); activeProfileChar_->notify();`. Already-existing test-serial logging path (`[BLE_TEST] PROFILE:<name>`) can stay; it serves a different audience.

Cost: ~40 LOC.

**nRF52840 deferral.** Same custom-service infra gap as option (a/b). Additionally, the profile index doesn't persist on nRF52840 (`saveProfile` is a no-op) — the notify char would always start at 0 after each reboot, which is consistent but a separate user-facing oddity to flag if/when we ship it.

**Recommendation: GO on ESP32. Defer nRF52840** along with the config-readback deferral.

**Test strategy.** Extend `test/test_ble_config_esp32/` to subscribe to the notify, trigger a profile switch (existing `test/test_profilemanager_esp32/` button-injection harness), and assert notification arrival within one connection-interval (BLE `connInterval` defaults to ~30 ms; budget 1× connInterval ≈ 50 ms with margin).

**Risk.**

- **Connection interval timing**: NimBLE notify is best-effort; under high HID-event load the notify could be queued behind keyboard reports. Acceptable — the app reflects the state on next interval. No correctness issue.
- **Subscriber lifecycle**: ensure the app unsubscribes on disconnect to avoid wasted callbacks.

## 4. Decisions captured

- **Config readback option**: (a) — resident copy in RAM. Rationale §3.2.
- **DIS (0x180A)**: planned for nRF52840 via Bluefruit `BLEDis` (TASK-358); skip on ESP32. Rationale §3.1.
- **nRF52840 scope**: deferred entirely from this iteration — no hardware available for verification. All three deliverables (firmware-version DIS + config readback + active-profile notify) bundled into TASK-358, gated on hardware availability. TASK-354/355/356 ship ESP32-only.

## 5. UUID assignments

| Characteristic | UUID | Properties | Platform |
|---|---|---|---|
| `CONFIG_FW_VERSION` (new) | `516515c5-4b50-447b-8ca3-cbfce3f4d9f8` | READ | ESP32 |
| `CONFIG_ACTIVE_PROFILE` (new) | `516515c6-4b50-447b-8ca3-cbfce3f4d9f8` | READ, NOTIFY | ESP32 |
| `CONFIG_WRITE` (existing, extend) | `516515c1-…` | WRITE_NR, WRITE, **+ READ** | ESP32 |
| DIS (0x180A) Firmware Rev | standard | READ | nRF52840 |

UUID family `516515cX-…` follows the convention in [BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md#characteristics): increment the last byte of the service UUID. TASK-354 / TASK-355 / TASK-356 should update that doc as they land.

## 6. Open follow-ups (not in scope here)

- [TASK-358](tasks/open/task-358-nrf52840-ble-readback-surfaces.md) — nRF52840 BLE readback (firmware-version via DIS + config readback + active-profile notify). Bundles all three nRF52840 surfaces under one custom-Bluefruit-GATT-service infra spike, gated on hardware availability.
- Connected-Pedal page UI — already gated on platform per existing widget patterns; "—" placeholder for nRF52840-only-deferred rows is fine.
- [TASK-357](tasks/open/task-357-reconcile-max-config-bytes-doc-vs-code.md) — `MAX_CONFIG_BYTES` doc/code mismatch ([BLE_CONFIG_PROTOCOL.md](BLE_CONFIG_PROTOCOL.md#constants) says 32768; [ble_config_reassembler.h:30](../../lib/PedalLogic/include/ble_config_reassembler.h#L30) defines 16384). TASK-355 sizes against the code value (16 KB); land TASK-357 first so the doc agrees.
