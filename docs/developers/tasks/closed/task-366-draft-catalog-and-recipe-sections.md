---
id: TASK-366
title: Draft Service catalog + Recipe sections of the BLE services guide (with TASK-354 worked example)
status: closed
closed: 2026-05-11
opened: 2026-05-02
effort: Medium (2-8h)
effort_actual: Medium (2-8h)
complexity: Senior
human-in-loop: No
epic: ble-services-developer-guide
order: 2
prerequisites: [TASK-365]
---

## Description

Write the implementer-facing core of the BLE services developer guide:

1. **Service catalog** — hand-written inventory of every BLE service and characteristic the project currently exposes. Per characteristic: UUID, properties (READ / WRITE / NOTIFY), source-of-truth file pointer, one-line purpose. Cover both ESP32 (e.g. `516515c1`, plus any new chars landed by EPIC-026) and the nRF52840 surfaces that exist on paper (with the TASK-358 deferral noted).
2. **Recipe — "add or change a service / characteristic"** — a numbered checklist covering: firmware files to edit, app-side files to edit, protocol/conventions doc updates, host-test (when shimmable per CLAUDE.md), on-device test extension, verification ladder (host → build → raw-read sanity → on-device runner → app-side `/verify-on-device`), and sequencing.
3. **Worked example** walking EPIC-026's TASK-354 (firmware-version READ) through the recipe step-by-step, *describing* every edit point. The implementer doing TASK-354 itself is responsible for the actual edits — this task only documents the path.

If TASK-365 chose the fold-in disposition for any of the three existing BLE docs, perform the `git mv` here as part of populating the catalog/recipe.

## Acceptance Criteria

- [ ] Service catalog covers every BLE characteristic that currently exists in firmware (ESP32 confirmed by reading `src/esp32/`; nRF52840 noted with TASK-358 deferral).
- [ ] Recipe section is a numbered checklist that an implementer can tick through without grepping the codebase.
- [ ] Worked-example walk-through of TASK-354 names every file an implementer would touch (firmware, app constant, app service, app screen, widget tests + mocks, protocol doc, on-device runner).
- [ ] If TASK-365 mandated a `git mv` of an existing doc, it is performed here and the absorbed content reshaped into the catalog or recipe.

## Test Plan

No automated tests required — change is non-functional.

## Prerequisites

- **TASK-365** — settles file structure (single vs folder; recipe-only vs recipe+reference split) and the disposition of the three existing BLE docs. Without those decisions, this task does not know where to write or what to absorb.

## Notes

- The worked example is descriptive, not implementational. **Do not edit firmware/app/tests as part of this task** — only describe the edits TASK-354 would make. Per the EPIC-027 docs-only constraint.
- If TASK-354 itself is still blocked under EPIC-026 when this task runs, the worked example can be a *future-tense* walk-through; the doc remains accurate when TASK-354 lands.
- This is the load-bearing task for the success criterion: the recipe + worked example must together be sufficient to add the example's characteristic without grepping or asking. Verify before closing.
- After writing the new guide doc(s), invoke `/doc-check` since the file lives under `docs/developers/` outside the tasks/ideas exclusions.

## Acceptance Criteria — verification (2026-05-11)

- [x] **Service catalog covers every BLE characteristic that currently exists in firmware.** Confirmed by reading [src/esp32/src/ble_config_service.cpp:18-22](../../../src/esp32/src/ble_config_service.cpp#L18) — four characteristics on ESP32 (`CONFIG_WRITE` / `CONFIG_WRITE_HW` / `CONFIG_STATUS` / `HW_IDENTITY`). nRF52840 noted as HID-only with TASK-358 deferral. Planned EPIC-026 chars (`FIRMWARE_VERSION` / `CONFIG_READBACK` / `ACTIVE_PROFILE`) listed in a separate "Planned" subtable.
- [x] **Recipe section is a numbered checklist** — 12 steps from UUID-pick through sequencing, plus inline file paths. Implementer ticks through; no grep required for the routine cases.
- [x] **Worked-example walk-through of TASK-354 names every file** an implementer would touch:
  - Firmware: `src/esp32/src/ble_config_service.cpp` (UUID const + `createCharacteristic` block), `include/version.h` (FIRMWARE_VERSION include).
  - App constant: `app/lib/constants/ble_constants.dart` (`kFirmwareVersionUuid`).
  - App service: `app/lib/services/ble_service.dart` (`_firmwareVersionChar` field, discovery branch, `readDeviceFirmwareVersion()` method).
  - App screen: `app/lib/screens/connected_pedal_screen.dart` (replace `_PendingRow(label: 'Firmware')` at line 57).
  - Widget tests + mocks: `app/test/widget/connected_pedal_screen_test.dart` and `app/test/widget/connected_pedal_screen_test.mocks.dart` (regenerate via `flutter pub run build_runner build`).
  - Protocol doc: `docs/developers/BLE_CONFIG_PROTOCOL.md` (row in Characteristics table).
  - On-device runner: `test/test_ble_config_esp32/test_main.cpp` and `test/test_ble_config_esp32/runner.py`.
- [x] **No `git mv` needed in this task.** TASK-365 deferred the `BLE_CONFIG_IMPLEMENTATION_NOTES.md` fold-in to TASK-367 (where the Conventions and Gotchas sections that absorb it are drafted). The other two BLE docs were Keep + cross-link, no move.

## Output

- Created [docs/developers/BLE_SERVICES_GUIDE.md](../../BLE_SERVICES_GUIDE.md) with sections 1–3 populated (Catalog / Recipe / Worked example) and placeholders for 4–8 (filled by TASK-367 and TASK-368).
- `/doc-check` verdict applied manually (the file is uncommitted, outside the `main...HEAD` diff scope): persona `developer` (path) matches `developer` (content). High confidence, correctly placed.
