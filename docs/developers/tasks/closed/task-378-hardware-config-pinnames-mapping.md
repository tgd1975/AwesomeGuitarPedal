---
id: TASK-378
title: Hardware config — pinNames mapping (physical pin → standard name)
status: closed
closed: 2026-05-11
opened: 2026-05-11
effort: Small (<2h)
effort_actual: Medium (2-8h)
complexity: Medium
human-in-loop: No
epic: named-pins
order: 2
prerequisites: [TASK-377]
---

## Description

Extend the hardware config so a builder can declare, *per physical
pin*, the standard name that pin represents on their board. This is
the bridge between a physical layout (which differs per build) and
the portable vocabulary (which is the same across builds).

Schema-side change to `data/config.schema.json`:

- Add an optional `pinNames` property: an object mapping physical
  pin identifier (string form of the GPIO number, or a stable
  identifier consistent with the rest of the schema) to a standard
  name from the v1 set (TASK-377). Values are validated against the
  v1 set via `$ref` to the canonical file from TASK-377; unknown
  values are rejected at schema-validate time.
- `additionalProperties: false` on the inner object so typos in the
  name surface as validation errors, not silent acceptance.
- Document in the field's `description`: this is an optional
  vocabulary translation layer; a hardware config with no
  `pinNames` is still valid; a hardware config that maps *some*
  pins and not others is still valid.

App-side change to `app/lib/models/hardware_config.dart`:

- Add `Map<String, String> pinNames` to the model, default to an
  empty map when absent during `fromJson`, round-trip through
  `toJson`.
- Expose a lookup helper (`String? nameOf(int pin)` /
  `int? pinOf(String name)`) so call sites are not all rewriting
  the same map-walk.

The canonical example `data/config.json` should grow a small
`pinNames` block demonstrating one or two mappings, to help
community-shared configs discover the field. Keep the example minimal
— the goal is "this is what one looks like", not "exhaustive
documentation in the example."

## Acceptance Criteria

- [ ] `data/config.schema.json` declares an optional `pinNames`
      object whose values are constrained to the v1 standard-name
      set via `$ref` to the canonical file from TASK-377.
- [ ] Unknown names are rejected by schema validation; the schema
      test exercises at least one valid mapping, one missing name,
      and one unknown name.
- [ ] `HardwareConfig` exposes `pinNames` and lookup helpers
      (`nameOf` / `pinOf`); helpers are covered by Dart unit tests
      where Dart tests already exist for this model, or by a small
      host-side test if logic moves into C++ during firmware
      consumption (TASK-380).
- [ ] `data/config.json` includes a minimal example mapping to
      seed builder familiarity.
- [ ] Configs without `pinNames` parse unchanged (no behavioural
      regression for existing builders).

## Test Plan

**Host tests** (`make test-host`):

- Extend the hardware-config tests to cover: valid mapping parses,
  missing `pinNames` defaults to empty, unknown name is rejected,
  duplicate target name (two pins mapped to the same standard name)
  — decide here whether that is an error or a warning; document the
  choice in the task close comment.
- Register any new test files in `test/CMakeLists.txt`.

**App-side**: where Dart tests already exist for `HardwareConfig`,
add coverage for `nameOf` / `pinOf` and round-trip.

## Prerequisites

- **TASK-377** — provides the canonical v1 standard-name file the
  schema references. Without it, the schema cannot constrain the
  vocabulary.

## Notes

- **Direction of the map.** Pin → name (not name → pin). Direction
  matters: each physical pin has exactly one role, but a standard
  name might not be present at all on a given build (and that is
  fine — the missing-mapping warning lives in TASK-381). Mapping
  pin → name preserves the "one role per pin" invariant naturally.
- **One-to-one vs. one-to-many.** Disallow mapping two pins to the
  same standard name unless the v1 set explicitly allows multi-
  instance roles (e.g. `pedal_a` is single-instance; expression
  controls may legitimately have several). The decision belongs in
  TASK-377's set design. Here, encode whatever TASK-377 chose.
- **No firmware change yet.** This task lands the data contract; the
  firmware-side resolution at config load is TASK-380.
- **String keys for pin identifiers.** Use stringified GPIO numbers
  (`"23"`) rather than numeric keys, because JSON objects key on
  strings anyway. Be explicit in the schema description so future
  readers do not assume numeric keys are valid.

## Close note

**Duplicate target name (one-to-many) — warning, not schema error.** JSON Schema's `patternProperties` cannot natively express value-uniqueness, and a custom schema keyword would couple the data contract to a single validator implementation. The Dart `HardwareConfig` model surfaces duplicates via `duplicatePinNames` (a `Set<String>`); UX layers in TASK-381 turn that into a warning banner. `pinOf(name)` returns the lowest-numbered pin deterministically when duplicates exist. The reasoning: a schema error would block legitimate local experimentation (a builder hand-editing `pinNames` while debugging a wiring change), and the broader policy — unknown names at profile-load time, missing mappings at config-time — already lands warnings instead of hard errors. Keep the rule consistent.

**Schema-resolution mechanics.** Python `_validate_against_schema` registers every sibling `*.schema.json` in `data/` into a `referencing.Registry` so `$ref: pin-names.schema.json` resolves without a network fetch. The Dart `SchemaService` does the same via `RefProvider.sync`, pre-loading `assets/pin-names.schema.json` and intercepting refs to that filename. The JS configurator (TASK-381) will need the same wiring.

**Mirror discipline.** `data/*.schema.json` is the source of truth; `app/assets/*.schema.json` is a hand-maintained mirror (matching the prior `debounceMs` work — see commit `cbc162f`). When editing one, edit the other; the pre-commit hook's JSON-schema linter validates both copies. A future task could automate the sync (existing `sync_task_system.py` is the natural model) but that is out of scope here.
