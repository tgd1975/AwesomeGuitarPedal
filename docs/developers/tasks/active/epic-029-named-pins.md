---
id: EPIC-029
name: named-pins
title: Named pins — portable pin vocabulary for shareable profiles
status: open
opened: 2026-05-11
closed:
assigned:
branch: feature/firmware
---

Seeded by IDEA-061 (Named pins — portable pin vocabulary for shareable profiles).

Today profiles reference pins by hardware identifier (e.g. `D23`). That
couples a profile to a single builder's wiring: a profile authored on
one board layout is meaningless on another's, which makes the
shareable-profile flow regress to "works only on the exact build it
was authored on." This epic introduces a second, optional reference
mode: a curated standard set of role-based pin names (`pedal_a`,
`expression_1`, `bank_up`, …) that profiles can reference symbolically
and the hardware config maps to physical pins per build.

Two modes live side by side, builder picks per pin:

a) **Direct** — `D23` (current behaviour, always works, unchanged).
b) **Named** — hardware config declares `D23 → pedal_a`; profiles
reference `pedal_a`. The name must come from the strongly encouraged
standard set baked into the schema so autocomplete and validation push
builders toward convergence.

Free-form names were considered and rejected: in a community-shared
artifact, free text needs curation to keep inappropriate strings out,
and the curation cost outweighs the flexibility. Builders who need a
role not in the standard set file a GitHub idea to extend it — that
is acceptable builder-scope friction.

## Scope

- The v1 standard name set (inventory + canonical file).
- Hardware-config schema: optional `pinNames` mapping from physical
  pin to standard name.
- Profile schema: profiles may reference either a direct pin ID or a
  standard name; mixed within one profile is allowed.
- Firmware: resolve named pins to physical pins at config-load time
  (ESP32). Resolution failures emit a warning (missing-mapping is not
  fatal — a profile may use names the hardware config hasn't mapped
  yet, because the builder only cares about a subset).
- Configurator/app UX: autocomplete from the standard set, validation
  against it, warning (not error) when a profile references a name
  with no mapping in the active hardware config.
- Builder docs: how to use named pins, when to prefer them over
  direct IDs, and the GitHub-idea process for proposing new standard
  names.

## Out of scope

- **nRF52840 firmware resolution.** Deferred to EPIC-025 follow-up
  (TASK-360 parity audit) until the hardware is reachable again.
- **Auto-mapping suggestions.** The configurator does not guess
  mappings; the builder maps explicitly. A future enhancement could
  propose mappings from a recognised hardware preset, but that is its
  own idea.
- **Locking the standard set behind schema validation that rejects
  unknown names outright.** Unknown names produce a warning, not a
  hard error — the GitHub-idea process is the curated path for
  extending the set, but a builder can still author and test locally
  before submitting.

## Tasks

Tasks are listed automatically in the Task Epics section of
`docs/developers/tasks/OVERVIEW.md` and in `EPICS.md` / `KANBAN.md`.
