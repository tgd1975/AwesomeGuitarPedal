---
id: TASK-390
title: Action editor (Flutter) — named-pin picker with autocomplete and inline mapping hint
status: closed
closed: 2026-05-11
opened: 2026-05-11
effort: Medium (2-8h)
effort_actual: Medium (2-8h)
complexity: Medium
human-in-loop: Support
epic: named-pins
order: 8
prerequisites: [TASK-378, TASK-379, TASK-389]
---

## Description

Split out of TASK-381 (configurator UX) for the profile-editor surface
specifically. Today the action editor's pin field is a numeric text
input (`PinField`). After this task, builders authoring a Pin*Action
can pick either:

- **Direct** (e.g. `27`) — the existing behaviour, unchanged.
- **Named** (e.g. `button_a`) — autocomplete against the v1 standard
  set from `PinNamesCatalog` (TASK-389); the active hardware config's
  mapping is rendered inline as a hint (e.g. "→ GPIO 13") so the
  builder is not guessing what physical pin a name means on their
  build.

When a name is not in the v1 set, the field shows an inline
validation message linking to the GitHub addition process (TASK-382
provides the doc anchor). The build still saves — the schema (TASK-379)
validates on upload — but the message guides convergence toward the
curated set.

The widget round-trips the authored form: a named ref saved as
`button_a` must read back as `button_a`, never silently rewritten to
the resolved GPIO (TASK-379 PinRef contract).

## Acceptance Criteria

- [ ] Action editor offers a toggle / dropdown between Direct and
      Named pin reference.
- [ ] Named mode autocompletes against
      `PinNamesCatalog.filterByPrefix`, grouped by category.
- [ ] Picking a named ref displays the active hardware config's
      current mapping inline ("→ GPIO 13"). If unmapped, the inline
      hint says "no mapping in current hardware config".
- [ ] Typing a non-standard name surfaces an inline validation
      message + link to TASK-382's builder doc.
- [ ] Save round-trips the authored form: direct stays direct, named
      stays named (no silent resolution). Existing widget tests
      continue to pass.

## Test Plan

**Host tests** (Dart, via `flutter test`):

- Widget tests in `app/test/widget/action_editor_*_test.dart` (extend
  existing) covering: toggle behaviour, autocomplete dropdown, inline
  mapping hint, validation message for non-standard names,
  round-trip preservation.

## Prerequisites

- **TASK-378** — the active hardware config's `pinNames` is the
  source of the inline mapping hint.
- **TASK-379** — `PinRef` sum type + back-compat `pin: <int>` ctor
  shortcut is what this UI edits.
- **TASK-389** — `PinNamesCatalog` powers the autocomplete and
  validation.

## Notes

- **Origin.** Carved out of TASK-381 — the profile-editor surface
  is independently shippable and the most user-visible piece of the
  named-pin UX.
- **No silent fix-up.** Saving a named ref does NOT collapse it to the
  resolved direct ID. The portable form is the authored form
  (TASK-379 contract).
- **Active hardware config.** The widget needs access to the current
  `HardwareConfig` to render the inline hint. Pass it down from the
  screen / state layer rather than reading a global.
