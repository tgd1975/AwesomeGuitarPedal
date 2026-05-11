---
id: TASK-381
title: Configurator / app UX — autocomplete, validation, and missing-mapping warning for named pins
status: closed
closed: 2026-05-11
opened: 2026-05-11
effort: Medium (2-8h)
effort_actual: XS (<30m)
complexity: Medium
human-in-loop: Support
epic: named-pins
order: 5
prerequisites: [TASK-378, TASK-379]
---

## Description

Surface named pins in the builder-facing tools so the standard set is
discoverable, typos are caught early, and missing mappings produce a
helpful warning rather than a silent dead reference. Three UX
surfaces are in scope:

1. **Hardware-config editor** (the app's hardware-config page and
   any external `docs/tools/config-builder/`-style helper):
   - When a builder is mapping a physical pin to a name, surface the
     v1 standard set as autocomplete suggestions grouped by category
     (action buttons, expression, bank/preset, LEDs). Free entry is
     allowed — the schema validates anyway — but the curated
     suggestions guide convergence.
   - Inline validation: typing a non-standard name highlights the
     field with a "not in the v1 set; propose an addition via
     GitHub" link (the addition process lands in TASK-382).
2. **Profile editor** (anywhere the app lets a builder pick a pin
   reference for an action):
   - Allow the builder to pick either direct (`D23`) or named
     (`pedal_a`) per pin reference. Named refs autocomplete against
     the v1 set.
   - Render the *active hardware config's* current mapping inline as
     a hint ("`pedal_a` → D23 on this build") so the builder is not
     guessing.
3. **Missing-mapping warning**:
   - When a profile is loaded that references a name with no
     mapping in the active hardware config, surface a non-blocking
     warning in the editor and on the connected-pedal page, naming
     the unresolved name(s). Offer a one-click "map it now" prompt
     that opens the hardware-config editor scoped to that name.
   - **This is a warning, not an error.** A builder may legitimately
     load a community profile that uses names they do not need
     mapped. The warning is informational.

## Acceptance Criteria

- [ ] Hardware-config editor offers autocomplete from the v1
      standard set when mapping a pin to a name; suggestions are
      grouped by category.
- [ ] Typing a non-standard name surfaces an inline validation
      message and a GitHub-addition link.
- [ ] Profile editor allows per-reference choice of direct vs named;
      named entries autocomplete against the v1 set.
- [ ] Profile editor renders the active hardware config's current
      mapping next to each named reference (e.g. "→ D23").
- [ ] Loading a profile with an unmapped named reference produces a
      non-blocking warning naming each unresolved name; the builder
      can dismiss it or open the hardware-config editor pre-scoped
      to map it.
- [ ] No path silently swallows an unmapped reference; equally, no
      path treats it as a fatal error.

## Test Plan

**App-side tests** (where the app already has widget/integration
tests for the relevant pages):

- Hardware-config editor: typing into a name field shows
  suggestions; choosing one persists; typing a non-standard name
  shows the validation message; saving still works.
- Profile editor: toggling a pin reference between direct and named
  forms; autocomplete behaviour; round-trip preserves the authored
  form (mirrors TASK-379's host-side assertion at the UI layer).
- Missing-mapping warning: profile with an unmapped name → warning
  banner appears; "map it now" navigates to the right editor scope.

**Manual verification via `/verify-on-device`** for the missing-
mapping warning end-to-end, against a real Pixel + ESP32, since the
warning's value is "does it actually help the builder fix it?" — a
unit test cannot answer that question.

## Prerequisites

- **TASK-378** — provides the hardware-config `pinNames` shape this
  UI edits.
- **TASK-379** — provides the profile-side sum-type pin reference
  this UI edits.

## Notes

- **Why the active mapping is shown inline.** A profile editor that
  shows `pedal_a` without revealing *what physical pin that means on
  this specific build* shifts a mental load onto the builder for
  no reason. The mapping is one lookup away — show it.
- **Warning surface placement.** Two places, two phrasings: the
  profile editor itself (when you load a profile with unresolved
  refs into the editor) and the connected-pedal page (when the
  pedal boots a config with unresolved refs and reports the
  end-of-load summary from TASK-380). Same data, different
  contexts.
- **No silent fix-up.** The configurator must *never* rewrite a
  named ref to a direct ID under the builder's nose. The portable
  form is the authored form.
- **Out of scope.** Suggested mappings ("looks like a 4-button
  layout, want me to fill these in?") are deferred — the GitHub-
  idea process is the right channel if a recurring hardware
  preset emerges.

## Close note

Split into four follow-ups so each surface lands as its own
reviewable commit:

- **[TASK-389](../open/task-389-pin-names-catalog-service.md)** —
  shared `PinNamesCatalog` Flutter service that reads
  `pin-names.schema.json` and exposes autocomplete / validation /
  grouping for every downstream UI.
- **[TASK-390](../open/task-390-action-editor-named-pin-ux.md)** —
  action editor (profile editor) named-pin picker, autocomplete,
  and inline mapping hint.
- **[TASK-391](../open/task-391-missing-mapping-warning-surfaces.md)**
  — non-blocking missing-mapping warning surfaces in the profile
  editor / list and on the connected-pedal page.
- **[TASK-392](../open/task-392-js-config-builder-pinnames-ux.md)**
  — JS configurator (`docs/tools/config-builder/`) pinNames editor
  with `<datalist>` autocomplete and non-standard-name warning.

This task's scope is fully covered by those four; closing here so
the OVERVIEW / KANBAN reflect the real shape of the remaining work.
Effort-actual is XS because no implementation landed under this ID
— the scaffolding above is the deliverable.
