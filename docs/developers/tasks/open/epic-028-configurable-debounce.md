---
id: EPIC-028
name: configurable-debounce
title: Configurable button debounce in the hardware config
status: open
opened: 2026-05-11
closed:
assigned:
branch: feature/configurable-debounce
---

Seeded by IDEA-060 (Add debounce time to the hardware config).

Move the hard-coded 100 ms button-debounce delay out of firmware and into
the project-wide hardware configuration so different physical-switch
models can be supported without recompiling. Debounce time is a property
of the switch hardware, not the firmware — yet today it lives as a
constant in `src/esp32/include/button.h` and
`src/nrf52840/include/button.h`. The hardware config already captures
pin assignments, button/LED counts, and the BLE pairing pin; debounce
is the obvious omission.

## Scope

- Add a single project-wide `debounceMs` field to the hardware-config
  schema (`data/config.schema.json`), the canonical example config
  (`data/config.json`), and the Dart model
  (`app/lib/models/hardware_config.dart`).
- Default to 100 ms when the field is absent so existing configs and
  community profiles keep working unchanged.
- Plumb the value through ESP32 firmware so `Button::debounceDelay` is
  read from the loaded hardware config at boot instead of the
  compile-time constant.
- Host unit tests covering: field present and applied, field absent
  → defaults to 100 ms, out-of-range value rejected by schema.

## Out of scope

- **Per-button and per-input-type overrides.** Keep the schema flat
  with one project-wide value. Granularity can be added later if a
  real build needs it; until then KISS.
- **nRF52840 firmware plumbing.** The nRF52840 hardware is currently
  unavailable (EPIC-025), so the device-side change is deferred. The
  nRF52840 parity audit (TASK-360) will pick this up when hardware is
  reachable again. Schema, model, and default behaviour land now and
  will be ready when the nRF52840 work resumes.
- **Community-shareable hardware configs.** That follow-up depends on
  this epic landing first and is tracked separately under
  [IDEA-063](../../ideas/open/idea-063-community-shareable-hardware-configurations.md).

## Tasks

Tasks are listed automatically in the Task Epics section of
`docs/developers/tasks/OVERVIEW.md` and in `EPICS.md` / `KANBAN.md`.
