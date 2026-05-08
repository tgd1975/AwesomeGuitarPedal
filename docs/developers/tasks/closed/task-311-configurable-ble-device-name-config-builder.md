---
id: TASK-311
title: Configurable BLE device name — web config builder support
status: closed
opened: 2026-04-29
closed: 2026-04-29
effort: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: config-driven-runtime-customisation
order: 6
prerequisites: [TASK-310]
---

## Resolution

**Won't do — closed without implementation (2026-04-29).**

Prerequisite TASK-310 was closed without implementation: the BLE adapter
on ESP32 is constructed during static initialization, before LittleFS is
mounted, so the device name cannot be read from `config.json` without a
significant restructure. Decided not worth the cost. There is nothing
left for the config builder to configure here.

See TASK-310's `## Resolution` section and IDEA-025 for the postmortem.

## Description

Add a UI field in the web config builder for editing
`ble.deviceName` in `config.json`. The field is optional; leaving it
empty must omit the key entirely (not write `""`), so the firmware
fallback rule from TASK-310 kicks in.

## Acceptance Criteria

- [ ] Builder shows a `BLE device name` input under a `BLE` section.
- [ ] Setting the field writes `ble.deviceName` to the JSON; clearing it removes the key entirely.
- [ ] Loading an existing `config.json` with or without the field populates the input correctly.
- [ ] Output validates against `config.schema.json` from TASK-310.

## Test Plan

**Builder tests** (whatever framework `docs/tools/config-builder/` already uses):

- Cover: set name, clear name, round-trip with key present, round-trip with key absent.

## Prerequisites

- **TASK-310** — defines the schema field and the empty-vs-absent semantics the builder must respect.

## Notes

- Show a hint near the field: "Leave empty to use the auto-generated name (`ASP_{numButtons}_{numProfiles}`)."
