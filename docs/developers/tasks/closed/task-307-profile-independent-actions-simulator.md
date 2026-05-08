---
id: TASK-307
title: Profile-independent actions — web simulator support
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: config-driven-runtime-customisation
order: 2
prerequisites: [TASK-306]
---

## Description

Update the web simulator so it understands the optional top-level
`independentActions` block in `profiles.json` and fires both the active
profile's action and the matching independent action on every button event.
The simulator should mirror firmware semantics: no profile slot consumed,
profile switches leave independents intact, missing block is a no-op.

## Acceptance Criteria

- [x] Simulator loads a `profiles.json` containing `independentActions` without errors.
- [x] On a button event the simulator dispatches both the profile action and the independent action (independent fires first, mirroring firmware order).
- [x] Switching profiles in the simulator UI does not clear or change the independent action set.

## Test Plan

**Simulator tests** (whatever framework `docs/simulator/` already uses — likely Jest/Vitest):

- Add a test fixture `profiles.json` with `independentActions` set for one button.
- Cover: both actions fire on press, profile switch leaves independents in place, missing block is a no-op.

## Prerequisites

- **TASK-306** — defines the `independentActions` schema and reference firmware behaviour the simulator must mirror.

## Notes

- Keep simulator semantics identical to firmware — divergence here will confuse users debugging configs.
