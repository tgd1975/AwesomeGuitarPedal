---
id: TASK-312
title: Configurable BLE device name — Flutter app support
status: closed
opened: 2026-04-29
closed: 2026-04-29
effort: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: config-driven-runtime-customisation
order: 7
prerequisites: [TASK-310]
---

## Resolution

**Won't do — closed without implementation (2026-04-29).**

Prerequisite TASK-310 was closed without implementation: the BLE adapter
on ESP32 is constructed during static initialization, before LittleFS is
mounted, so the device name cannot be read from `config.json` without a
significant restructure. Decided not worth the cost. There is nothing
left for the Flutter app to configure here.

See TASK-310's `## Resolution` section and IDEA-025 for the postmortem.

## Description

Update the Flutter app so it can read, edit, and write the optional
`ble.deviceName` field in `config.json`. Empty input must omit the key
entirely so the firmware fallback rule from TASK-310 applies.

## Acceptance Criteria

- [ ] App reads a `config.json` with or without `ble.deviceName` from the pedal without losing data.
- [ ] App UI exposes a `BLE device name` input under a `BLE` section.
- [ ] App writes the field when set; clearing it omits the key entirely (not `""`).
- [ ] After save, the pedal advertises the configured name (manual on-device check).

## Test Plan

**Flutter widget/unit tests** (whatever the app already uses):

- Cover: parse + serialise round-trip with field present, with field absent, edit then save, clear then save.

## Prerequisites

- **TASK-310** — defines the schema field and the empty-vs-absent semantics the app must respect.

## Notes

- Mirror the config builder labelling and helper hint so users see consistent terminology across surfaces.
