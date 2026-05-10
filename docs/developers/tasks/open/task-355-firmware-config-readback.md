---
id: TASK-355
title: Firmware — config readback (option chosen in TASK-353)
status: open
opened: 2026-05-01
effort: Medium (2-8h)
complexity: Senior
human-in-loop: Clarification
epic: pedal-details-app-pages
order: 2
prerequisites: [TASK-353]
decision_doc: docs/developers/BLE_READBACK_IMPACT.md
---

## Description

Make the pedal's currently-running configuration readable over BLE
so the Connected-Pedal page (TASK-336) can fill its
currently-placeholdered Configuration and Storage rows.

**Implementation path: option (a)** — make the existing
`CHAR_WRITE_UUID` (`516515c1-…`) characteristic readable, retain
the canonical JSON resident in RAM after each successful upload,
and return that buffer on read. Chosen over option (b) because
the resident-copy memory cost (≤ 16 KB / `MAX_CONFIG_BYTES`) is
non-binding on the measured nRF52840 budget (~215 KB free heap)
and the implementation is ~5× smaller than an inverted chunked
reassembler. Full rationale in
[BLE_READBACK_IMPACT.md §3.2](../../BLE_READBACK_IMPACT.md#32-config-readback--task-355).

**Platform scope: ESP32 only.** nRF52840 has no custom GATT
service today (HID-only via Bluefruit), so config readback there
is gated on a separate "should nRF52840 expose a custom BLE
service?" question that's out of scope here. The Connected-Pedal
page Configuration row stays "—" for nRF52840-connected pedals.

App-side:

- Wire `BleService.readActiveConfig()` (or equivalent) and surface
  the result on the Connected-Pedal page's Configuration row,
  replacing the `_PendingRow` widget. Storage row uses the same
  data (size of the resident config / total flash budget for
  config) — same call, different presentation.

## Acceptance Criteria

- [ ] Config readback works on **ESP32** via option (a) — existing
      `CHAR_WRITE_UUID` exposes READ, returns the resident JSON.
- [ ] App reads back exactly what was written for at least the
      example configs in `profiles/`.
- [ ] ESP32 RAM regression is ≤ 16 KB (`MAX_CONFIG_BYTES`); document
      the measured before/after via `pio run -e nodemcu-32s --target size`.
- [ ] Connected-Pedal page Configuration row is no longer a
      placeholder for ESP32-connected pedals; renders "—" for
      nRF52840-connected pedals (deferred). Storage row renders
      something useful (size of resident config / total budget),
      or stays "—" with a one-line rationale committed in this
      task's Notes.
- [ ] On-device test verifies a round-trip (write → read → equal)
      on ESP32.
- [ ] nRF52840 deferral is documented (one-line note in
      Connected-Pedal page widget + a follow-up task scoped:
      "expose custom BLE service on nRF52840?").

## Prerequisites

- **TASK-353** — feasibility analysis pinpoints option (a vs b)
  and the nRF52840 RAM headroom number this task must respect.

## Test Plan

**On-device tests** — `make test-esp32-ble`. Extend with a
"round-trip equality" test: write a known config, read it back,
assert byte-equal at the JSON level.

**Host tests** — if option (b) is chosen, the chunked
reassembler logic is host-testable behind `HOST_TEST_BUILD`. Add
`test/unit/test_config_readback.cpp`. If option (a) is chosen,
host coverage is limited to "resident copy stays correct after
write" which is mostly covered by existing tests; document the
gap.

**App-side widget test** — extend
`test/widget/connected_pedal_screen_test.dart` to assert the
Configuration row reflects mocked `readActiveConfig()` data.

## Notes

- This was carved out of the original TASK-337 — the largest of
  the three deliverables and the one with the real design
  decision (option a vs b) and the real risk surface (nRF52840
  RAM budget).
- Hardware: ESP32 connected via USB. nRF52840 round-trip
  deferred to TASK-358 (no device available; see
  [BLE_READBACK_IMPACT.md](../../BLE_READBACK_IMPACT.md)).
