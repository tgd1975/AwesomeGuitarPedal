---
id: TASK-389
title: PinNamesCatalog service — Flutter-side reader of pin-names.schema.json
status: closed
closed: 2026-05-11
opened: 2026-05-11
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: No
epic: named-pins
order: 7
prerequisites: [TASK-377]
---

## Description

Split out of TASK-381 (configurator UX) so the Flutter UI layers have
a single, tested entry point for "what's in the v1 standard pin-name
set, grouped how?" Without this, every UI surface that wants
autocomplete or validation duplicates the schema-parsing logic.

The catalog loads `assets/pin-names.schema.json` (mirrored from
`data/pin-names.schema.json` per the TASK-378 sync convention), reads
the `enum` and the `x-categories` metadata block, and exposes:

- `allNames` — flat list of v1 names in category order.
- `groupedByCategory()` — map of category-key → entries, preserving
  insertion order so the picker UI renders categories in a stable order.
- `contains(name)` — membership check for validation.
- `lookup(name)` — entry (name + category + role description) for
  tooltips and inline help.
- `filterByPrefix(query)` — case-insensitive substring filter for
  autocomplete dropdowns.

The catalog caches the parsed schema after first load so repeated UI
opens don't re-parse the asset.

## Acceptance Criteria

- [ ] `PinNamesCatalog.load()` resolves all v1 names from
      `assets/pin-names.schema.json`.
- [ ] `groupedByCategory()` returns entries grouped by the
      `x-categories` keys (action_buttons, profile_controls,
      status_leds, profile_select_leds).
- [ ] `contains()`, `lookup()`, and `filterByPrefix()` behave per the
      service's docstring — covered by Dart unit tests.
- [ ] The schema file is registered as a Flutter asset (already done
      in TASK-378) and the catalog reads from it without a network
      fetch.

## Test Plan

**Host tests** (Dart, via `flutter test`):

- `app/test/unit/pin_names_catalog_test.dart` loads the bundled
  `assets/pin-names.schema.json` directly from disk (not via
  rootBundle, to avoid the Flutter binding) and asserts the public
  API contract.

## Prerequisites

- **TASK-377** — provides `data/pin-names.schema.json`, the canonical
  v1 file this service reads.

## Notes

- **Origin.** Carved out of TASK-381 (configurator named-pin UX) when
  it became clear the action editor + warning surface + JS configurator
  all want the same parsing logic — a shared service is cheaper than
  three open-coded readers.
- **Source of truth.** The `enum` is what the JSON Schema validates
  against; `x-categories` is presentation metadata. Both come from
  the same file so they never drift.
