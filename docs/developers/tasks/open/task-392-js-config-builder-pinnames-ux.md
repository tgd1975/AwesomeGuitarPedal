---
id: TASK-392
title: JS config-builder — pinNames mapping editor with autocomplete and non-standard validation
status: open
opened: 2026-05-11
effort: Small (<2h)
complexity: Medium
human-in-loop: Support
epic: named-pins
order: 10
prerequisites: [TASK-377, TASK-378]
---

## Description

Split out of TASK-381 (configurator UX) for the JS-side configurator
at `docs/tools/config-builder/`. The Flutter app is one surface for
hardware-config editing; the browser-based config-builder is the
other. Both have to teach builders the v1 standard pin-name set the
same way for shareable profiles to converge.

UX changes to the config-builder:

- A new pinNames section (per-pin row) lets the builder pick a v1
  standard name from a `<datalist>`-backed input. The schema
  (`data/pin-names.schema.json`) is loaded over HTTP from the same
  hosting path the builder already uses for `schema.json`.
- Inline validation: typing a non-standard name flags the field with
  the same wording the Flutter app uses (TASK-390) and links to the
  TASK-382 doc anchor.
- Save / export: the generated `config.json` includes the `pinNames`
  object when at least one mapping is set; absent when the builder
  maps nothing (preserves backwards compat with pre-EPIC-029 configs).
- Existing AJV-based schema validation continues to work — adding a
  `pinNames` field with v1 values must pass schema check.

## Acceptance Criteria

- [ ] The config-builder UI exposes a pinNames editor with one row per
      currently-configured pin.
- [ ] The input is backed by a `<datalist>` populated from the v1
      standard set.
- [ ] Non-standard names trigger an inline warning (no schema error
      yet — the schema rejects them, but the JS UX warns earlier).
- [ ] Empty pinNames is omitted from the exported config.json.
- [ ] AJV schema validation continues to pass for valid configs and
      fails for unknown pin names (parity with the Flutter / Python
      paths).

## Test Plan

No automated tests required — the JS config-builder ships without a
test harness today. Manual verification:

1. Open `docs/tools/config-builder/index.html` in a browser.
2. Load `data/config.json` (the canonical example with pinNames).
3. Verify the pinNames rows render with the existing 4 mappings.
4. Add a row, autocomplete a name (e.g. `led_select_1`), export, and
   confirm the JSON matches the schema.
5. Type a non-standard name (`my_custom_role`), verify the warning
   appears and the GitHub-addition link is present.

## Prerequisites

- **TASK-377** — `data/pin-names.schema.json` is the autocomplete /
  validation source.
- **TASK-378** — the hardware-config schema's `pinNames` shape is
  what the JS UI edits.

## Notes

- **Origin.** Carved out of TASK-381 — the JS configurator lives in
  a different repo subtree from the Flutter app and is best handled
  as its own commit.
- **No vendored copy of the schema.** Load
  `data/pin-names.schema.json` at runtime — the browser fetches it
  the same way it fetches `schema.json`. Avoid copying the enum into
  builder.js where it can drift.
- **Wording parity.** Inline validation message text should match
  the Flutter app's wording (TASK-390) for a consistent builder
  experience across surfaces.
