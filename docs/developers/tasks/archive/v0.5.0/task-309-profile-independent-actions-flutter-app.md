---
id: TASK-309
title: Profile-independent actions — Flutter app support
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: config-driven-runtime-customisation
order: 4
prerequisites: [TASK-306]
---

## Description

Update the Flutter app so it can read, edit, and write the optional
top-level `independentActions` block in `profiles.json`. The block must
round-trip cleanly when sent to and received from the pedal over BLE.

## Acceptance Criteria

- [x] App reads a `profiles.json` containing `independentActions` from the pedal without losing data.
- [x] App UI exposes editing the independent action set separately from per-profile button maps.
- [x] App writes a `profiles.json` with or without the block; absent block is omitted entirely (not written as `{}`).

## Test Plan

**Flutter widget/unit tests** (whatever the app already uses):

- Add tests covering: parse + serialise round-trip with block present, with block absent, edit then save.

## Prerequisites

- **TASK-306** — defines the schema and round-trip semantics the app must respect.

## Notes

- Mirror the simulator and config builder labelling so users see consistent terminology across surfaces.
