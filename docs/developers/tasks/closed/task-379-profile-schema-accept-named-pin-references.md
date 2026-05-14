---
id: TASK-379
title: Profile schema — accept named-pin references alongside direct pin IDs
status: closed
closed: 2026-05-11
opened: 2026-05-11
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: No
epic: named-pins
order: 3
prerequisites: [TASK-377]
---

## Description

Profiles currently reference pins by their hardware ID (e.g. `D23`).
This task extends the profile schema to accept *either* a direct pin
ID *or* a standard name from the v1 set (TASK-377), with mixed-mode
allowed within a single profile.

Schema changes:

- Wherever the profile schema today expects a pin reference, change
  the type from "direct ID only" to "one of: direct ID, standard
  name." Use a `oneOf` / `anyOf` clause that references the v1
  standard-name file (TASK-377) for the named case.
- A profile may freely mix both forms: a builder might author
  `pedal_a` for their primary footswitch (portable) and `D27` for a
  one-off prototype pin (build-specific). The schema must not force
  all-or-nothing.
- Add a clear `description` next to the modified field naming the
  two acceptable forms and pointing to the named-pin builder doc
  (TASK-382).

App-side model:

- The profile model gains a way to represent a pin reference as
  *either* a direct ID *or* a name. A simple sealed/sum type
  (`PinRef.direct(int)` / `PinRef.named(String)`) is preferable to
  stuffing both into one nullable field — it forces call sites to
  handle both arms.
- Serialisation: round-trip preserves the form the builder
  authored. `D23` stays `D23` after a save; `pedal_a` stays
  `pedal_a`. Do **not** auto-resolve named → direct on save — that
  destroys portability and is the whole point of the named form.

Validation reach: at schema-validate time, named values are
constrained to the v1 set. *Resolution* of a named ref against a
specific hardware config (and the warning when no mapping exists) is
TASK-381 (app/configurator UX) and TASK-380 (firmware boot path) —
not here.

## Acceptance Criteria

- [ ] Profile schema accepts both forms at every pin-reference site,
      constrained by the v1 standard-name file for the named case.
- [ ] Mixed direct + named within one profile validates.
- [ ] An unknown standard name in a profile is rejected at
      schema-validate time (with a clear error message naming the
      bad value).
- [ ] Profile model represents pin references as a sum type
      (direct vs named); round-trip serialisation preserves authored
      form.
- [ ] Existing profiles in the repo (and any committed community
      examples) parse unchanged.

## Test Plan

**Host tests** (`make test-host`):

- Profile parsing tests covering: direct-only profile, named-only
  profile, mixed profile, profile with an unknown name (must fail
  with a useful message), round-trip preserving authored form.
- Register new test sources in `test/CMakeLists.txt`.

**App-side**: where Dart profile-model tests already exist, mirror
the host coverage there for the `PinRef` sum type.

## Prerequisites

- **TASK-377** — provides the canonical v1 standard-name set that
  this task references from the profile schema.

## Notes

- **Mixed mode is deliberate.** A builder may use named refs for the
  portable parts of a profile (the footswitches everyone has) and
  direct IDs for build-specific extras (an unusual aux LED on a
  spare pin). Forcing all-or-nothing would push builders back to
  direct refs the moment one pin doesn't fit the standard set.
- **Do not auto-resolve on save.** Saving `pedal_a` and reading
  back `D23` (because that is what the active hardware config maps
  to) silently strips portability. Resolution happens at *use*
  time (firmware boot, configurator preview) — not at serialisation
  time.
- **`PinRef` is the right shape.** Resist nullable Int + nullable
  String — a sum type prevents the "what if both are set?" /
  "what if neither?" classes of bug at compile time.
- **No firmware change yet.** Firmware-side resolution of named
  refs is TASK-380.
