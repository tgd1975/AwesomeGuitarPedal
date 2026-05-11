---
id: TASK-375
title: Add debounceMs to hardware-config schema, example JSON, and Dart model
status: closed
closed: 2026-05-11
opened: 2026-05-11
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: No
epic: configurable-debounce
order: 1
---

## Description

Introduce a single project-wide `debounceMs` field across the three
hardware-config surfaces that define what a hardware config *is*:

1. **JSON schema** (`data/config.schema.json`) — declare `debounceMs`
   as an optional integer with sane bounds and a description that
   explains it is the debounce window applied to all configured
   buttons (not just `buttonPins[]`; also covers `buttonSelect`).
   Suggested bounds: `minimum: 1`, `maximum: 1000`. Values outside
   that range are almost certainly wrong (1 ms is below the noise
   floor of any mechanical switch; 1 s makes the pedal feel broken).
2. **Canonical example** (`data/config.json`) — include the field
   with its default value (100) and a short `_doc`-style comment so
   community-shared configs know it exists. Add to the `_doc`
   preamble that omitting the field falls back to 100 ms.
3. **Dart model** (`app/lib/models/hardware_config.dart`) — add the
   field, default to 100 when absent during deserialisation, and
   round-trip it through serialisation so existing app flows keep
   working.

This task is **host-only** — no firmware, no on-device test, no
plumbing into `Button`. That is TASK-376. The point of splitting is
that the schema/model/default behaviour should be reviewable and
testable independently of any embedded change.

## Acceptance Criteria

- [ ] `data/config.schema.json` declares `debounceMs` as an optional
      integer, with `minimum`/`maximum` bounds and a `description`
      that names this as the project-wide button-debounce window.
- [ ] `data/config.json` includes `debounceMs: 100` with a short
      explanatory note added to the `_doc` field.
- [ ] `HardwareConfig` in `app/lib/models/hardware_config.dart`
      exposes `debounceMs`, defaults to 100 when absent during
      `fromJson`, and emits the field via `toJson`.
- [ ] Host unit tests cover: field present (value applied), field
      absent (default 100), out-of-range value (rejected by schema
      validator).
- [ ] Existing configs (community-shared and in-tree) continue to
      parse without modification.

## Test Plan

**Host tests** (`make test-host`):

- Extend `test/unit/test_hardware_config_*.cpp` (or add a new file
  if the existing one is the wrong shape) covering:
  - `debounceMs: 50` → parsed value is 50.
  - Missing `debounceMs` → default 100.
  - `debounceMs: 0` and `debounceMs: 2000` → schema rejects.
- Register any new source file in `test/CMakeLists.txt` under
  `pedal_tests`.

**App-side**: a Dart `HardwareConfig` unit test covering present /
absent / out-of-range round-trip if the app already has Dart-side
config tests. If not, do not create the test scaffolding here — it
is enough that the schema validates rejection.

## Notes

- **Why a flat single field, not per-button.** IDEA-060 deliberately
  scopes this to one value. Per-button or per-input-type overrides
  can come later if a real build needs them; until then the schema
  stays flat (KISS). Do **not** introduce a nested
  `buttons[].debounceMs` shape now — it forecloses simpler future
  designs.
- **Scope of "all buttons".** The field applies uniformly to both
  `buttonPins[]` (action buttons) and `buttonSelect` (the profile-
  switching button). Document that in the schema description so a
  future reader does not assume it is action-buttons-only.
- **Default-on-absent rule.** Existing community configs do not
  include this field. The schema is `additionalProperties: false`
  today; this field is added as an optional property, not pushed
  into `required`. Apps and firmware must default to 100 when the
  key is absent, never error.
- **Cross-task hand-off.** TASK-376 picks up the actual firmware
  plumbing into `Button`. This task lands the *contract*; that one
  lands the *behaviour*.
