---
id: IDEA-054
title: Cross-version app↔firmware compatibility — policy and verification
description: Decide how to handle the case where users run app vN with firmware vM (M ≠ N), and how to *prove* that combination works. Today we sidestep it by saying "same version only".
category: 🛠️ tooling
---

## Context

TASK-260 unified version numbers across all deliverables (firmware,
app, CLI, simulator) — every release ships them at the same version.
TASK-331 (Info/About page) follows up by stating in-app: *"Compatible
with firmware of the same version."*

This is true today because we test the matching pair before each
release. But in the wild, users do not always update both sides at
once:

- App auto-updates from Play Store / TestFlight / sideload, firmware
  stays on whatever was last flashed.
- A musician flashes a release-build firmware, then opens an older
  cached app build.
- We accumulate users on N different version pairs we never tested.

The Info/About copy currently sidesteps this with "same version only";
that is honest, but it doesn't help the user diagnose mismatch, and
it doesn't tell us whether `(app v0.5, firmware v0.4)` actually works
or not.

## The two sub-questions

1. **Policy** — what do we *promise*?
   - Strict ("only same version is supported, unsupported pairs may
     misbehave") — easy to communicate, frustrating if v0.4 firmware
     happens to work fine with v0.5 app.
   - Compatibility range ("app vN supports firmware vN-1..vN") —
     more forgiving, requires us to actually verify the range.
   - SemVer-style ("breaking changes only at major bumps; minors and
     patches are forward-compatible") — closest to industry norm,
     forces us to discipline ourselves about what counts as breaking.

2. **Verification** — how do we *know*?
   - Manual: maintain a compatibility matrix in `docs/`, update by
     hand each release. Cheap, error-prone, decays.
   - CI: run a smoke test of the latest app against each of the last
     N firmware versions on a real or virtual pedal. Expensive, but
     the only honest answer.
   - In-app: the app reads the pedal's firmware version on connect
     and shows a clear "this combination is untested" banner if it
     falls outside the declared range. Cheap *and* informative,
     doesn't claim things we can't verify.

## Rough approach

The minimum viable answer is probably:

1. Pick policy = **SemVer-ish** ("same MAJOR.MINOR is supported;
   PATCH differences are forward-compatible") — keeps the door open
   without overcommitting.
2. Add a `minFirmware` and `maxTestedFirmware` constant in the
   `AppInfo` provider. Update both at release time alongside the
   version bump.
3. The Info/About page shows the range. The Connection details sheet
   (TASK-335) shows a banner when the connected pedal falls outside
   the tested range.
4. CI is out of scope for v1 — the matrix is small and we can verify
   manually until it isn't.

## Open questions

- Where does the compatibility range live as a single source of
  truth? Same `version.h` that holds firmware version (as a
  `MIN_COMPATIBLE_APP_VERSION` constant)? A separate
  `COMPATIBILITY.json`? Both sides need to read it.
- Is "Connection details sheet shows untested-pair banner" a TASK-335
  sub-bullet, or a follow-up task once this idea lands?
- Should we surface the same banner in the web simulator and the
  config builder, or is it app-only?
- Does this affect community profiles? A profile created on v0.5
  may rely on a feature firmware v0.4 doesn't have — already
  partially modeled via `minFirmware` in
  `community-profiles/index.json` per IDEA-017.

## Why deferred

The Info/About page (TASK-331) has to say *something* about
compatibility today. "Same version only" is the most honest
placeholder — anything stronger requires deciding the policy above
and probably wiring a `minFirmware` constant into `AppInfo`.
Resolving this idea is the proper way to upgrade that wording later.

Seeded by TASK-331.
