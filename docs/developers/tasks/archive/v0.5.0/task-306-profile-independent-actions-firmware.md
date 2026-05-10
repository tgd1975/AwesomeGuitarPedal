---
id: TASK-306
title: Profile-independent actions — firmware + schema
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: Medium (2-8h)
effort_actual: Medium (2-8h)
complexity: Medium
human-in-loop: Clarification
epic: config-driven-runtime-customisation
order: 1
---

## Description

Add an optional top-level `independentActions` block to `profiles.json` whose
entries fire on every button event in parallel with the active profile's
actions. The block uses the same button-map syntax as a profile's `buttons`
block but is never selectable and is not affected by profile switching.
A missing entry for a button is a no-op; a missing block entirely leaves
behaviour identical to today.

This task covers the firmware change, the JSON schemas, and the
builder-facing key reference. Web simulator, web config builder, and
Flutter app updates are tracked as separate sibling tasks under the same
epic.

## Acceptance Criteria

- [x] `ConfigLoader` parses `independentActions` and round-trips it through `saveToFile` and `mergeConfig`.
- [x] On every button event (press, long-press, double-press, release) the matching independent action fires alongside the active profile's action. Independent action fires first; absent block is a no-op. `mergeConfig` replaces independents when the JSON contains the block, keeps existing when absent.
- [x] `executeRelease()` is forwarded to independent `PinHighWhilePressed` actions.
- [x] Switching profiles leaves the independent action set untouched and active.
- [x] `data/profiles.schema.json` and `app/assets/profiles.schema.json` describe the new optional block.
- [x] `docs/builders/KEY_REFERENCE.md` documents the block with at least one JSON example.

## Test Plan

**Host tests** (`make test-host`):

- Add `test/unit/test_independent_actions.cpp` (or extend the closest existing dispatcher/config test).
- Cover: parse with block present, parse with block absent (no behavioural change), profile switch leaves independent set intact, release event reaches `PinHighWhilePressed` independent action, both action sets fire on a single event without one swallowing the other.

## Notes

- Likely dispatch site: `EventDispatcher` or `main.cpp` — confirm during implementation.
- `independentActions` must not occupy a profile slot in `ProfileManager`.
- The block is purely additive — no migration required for existing `profiles.json` files.

## Documentation

- `docs/builders/KEY_REFERENCE.md` — add a section for the `independentActions` block with a JSON example.
