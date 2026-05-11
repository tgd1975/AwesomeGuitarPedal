---
id: TASK-377
title: Define the v1 standard pin-name set and pick its canonical home
status: open
opened: 2026-05-11
effort: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: named-pins
order: 1
---

## Description

The whole value of named pins depends on convergence on a curated
vocabulary. Without one, named pins fragment immediately and shareable
profiles regress to the same coupling problem direct pin IDs have.
This task delivers the v1 set and the file that holds it.

Two sub-decisions:

1. **What names are in the v1 set?** Inventory candidate roles by
   reading the existing project artifacts where pedal/button/LED
   purposes already appear:
   - Builder docs under `docs/builders/` (wiring guides, profile
     authoring notes).
   - Example/community profiles in `data/` and any profile-related
     samples committed to the repo.
   - Existing test fixtures that name buttons/LEDs by role.
   - Tutorial videos / images referenced from the docs (if their
     captions name roles).
   Cluster the observed roles into a small, opinionated set. Resist
   the urge to enumerate every conceivable knob — a *strongly
   encouraged* standard works only when it is small enough that
   builders can scan it. Aim for ≤30 names for v1.
2. **Where does the set live?** IDEA-061's preferred answer was "one
   file — likely a schema." Decide whether the v1 set is:
   - A JSON Schema `enum` inlined into `data/config.schema.json`
     and the profile schema (referenced via `$ref`), or
   - A separate small JSON / YAML file under `data/` re-used by
     both schemas via `$ref`, or
   - A code-side constant generated from one of the above.

   The criterion is *one source of truth, mechanically syncable to
   firmware / app / docs / configurator*. Whatever you pick, name the
   single sync point and explain how each downstream consumer reads
   from it.

Output: the canonical file with the v1 enum, plus a short rationale
note (in this task's close comment or as a `## Decisions` section in
the file itself) documenting the inventory sweep and the trade-offs
behind the chosen home.

## Acceptance Criteria

- [ ] A single canonical file holds the v1 standard pin-name set
      (≤30 names, lowercase snake_case, role-based — e.g.
      `pedal_a`, `expression_1`, `bank_up`).
- [ ] The set is grouped by category (action buttons, expression
      pedals, bank / preset controls, status LEDs, …) with one-line
      role descriptions next to each name.
- [ ] The file is consumable by both the hardware-config schema
      (TASK-378) and the profile schema (TASK-379) via `$ref` or an
      equivalent single-source mechanism. Pick one; do not duplicate
      the list.
- [ ] A short `## Decisions` block or close-comment records: the
      sources you inventoried, the trade-off behind the chosen home,
      and the contributor-facing process for proposing additions
      (refers forward to TASK-382 for the full doc).

## Test Plan

No automated tests required at this stage — the deliverable is a
data file plus rationale. TASK-378/379 will exercise the file
through schema validation as soon as they consume it.

If the chosen home is a runnable artifact (e.g. a generated Dart
const), add a one-line host test that loads the file and asserts a
known name is present — enough to catch a future merge that
accidentally empties the list.

## Notes

- **Inventory before invention.** The point of the sweep is to keep
  the v1 set grounded in roles people actually use. If a name does
  not show up in the inventory and you cannot point to a builder
  artifact that needs it, leave it out — it can be added later
  through the GitHub-idea process.
- **Naming convention.** Lowercase snake_case (`bank_up`, not
  `BankUp` or `bank-up`) for consistency with existing JSON keys in
  this repo. Reserve the underscore for separating role tokens, not
  decoration.
- **No free-form names.** IDEA-061 explicitly rejected free text
  for community-shared artifacts. Do not introduce a "user defines
  their own name" escape hatch in this v1 — the curated path is the
  GitHub-idea process delivered in TASK-382.
- **Forward references.** TASK-378 (hardware-config schema) and
  TASK-379 (profile schema) consume this file. TASK-382 documents
  the addition process. Keep the inventory note tight; the full
  builder-facing doc is TASK-382's job.
