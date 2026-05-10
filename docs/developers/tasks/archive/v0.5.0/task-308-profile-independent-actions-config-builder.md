---
id: TASK-308
title: Profile-independent actions — web config builder support
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: config-driven-runtime-customisation
order: 3
prerequisites: [TASK-306]
---

## Description

Add UI in the web config builder for editing the optional top-level
`independentActions` block in `profiles.json`. The builder must be able to
add, edit, and remove the block, and must validate against the updated
schema from TASK-306.

## Acceptance Criteria

- [x] Builder shows a section for editing `independentActions` separate from the per-profile button maps.
- [x] Adding an entry produces JSON that validates against `profiles.schema.json` from TASK-306.
- [x] The block can be removed entirely, producing a `profiles.json` without the key (not an empty object).
- [x] Loading an existing `profiles.json` that already contains `independentActions` populates the editor correctly.

## Test Plan

**Builder tests** (whatever framework `docs/tools/config-builder/` already uses):

- Add fixture covering: add block, edit entry, remove block entirely, round-trip an existing file with block present.

## Prerequisites

- **TASK-306** — defines the schema the builder validates against and the semantics the user is configuring.

## Notes

- Section labelling: the user-facing copy should make clear this is *not* a profile (e.g. "Independent actions — fire on every button event regardless of profile").
