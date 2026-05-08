---
id: TASK-260
title: Unify version numbers across all deliverables (firmware, app, CLI, simulator, …)
status: closed
closed: 2026-05-01
opened: 2026-04-26
effort: Medium (2-8h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
---

## Description

Today every deliverable has its own version number, and they have already drifted:

| Deliverable | Current version | Source of truth |
|---|---|---|
| Firmware | `v0.4.1` | `include/version.h` (`FIRMWARE_VERSION`) |
| Flutter app | `1.0.0+1` | `app/pubspec.yaml` |
| Root `package.json` | `1.0.0` | `package.json` |
| Task system tooling | (separate) | `awesome-task-system/VERSION` |
| CLI / simulator | (none yet — but coming) | TBD |

The decision is to **align all of them on one project-wide version number** — even when a particular deliverable hasn't changed for several bumps in a row. The Flutter app gets `v0.4.0` next release whether or not any Dart code was touched. The simplification is worth more than the technical-correctness of "this artifact really is unchanged since v0.2.0."

This task wires that policy through the codebase, the `/release` skill, and the documentation.

## Acceptance Criteria

- [x] One canonical version source is chosen and documented — `include/version.h` (`FIRMWARE_VERSION`). All other deliverables are projections of its `MAJOR.MINOR.PATCH` triple. Documented in `docs/developers/CI_PIPELINE.md` "Version policy" and in the file's leading comment.
- [x] `/release` skill updates **every** deliverable's version field in one step: `include/version.h`, `app/pubspec.yaml`, root `package.json`, `awesome-task-system/VERSION`. CLI/simulator manifests will be added when those features land.
- [x] Drifted versions are aligned **at the next release cut**, not pre-emptively in this task. The `/release` skill now writes the canonical number into all four files in one step, so the unification happens naturally on the first invocation. (Decision 2026-05-01 with user: no need to pick a target version now.)
- [x] `docs/developers/CI_PIPELINE.md` "Release process → Version policy" section explicitly states: *"All deliverables share one version number. We do not skip-bump unchanged artifacts."*
- [x] `README.md` "Firmware" section has a versioning callout pointing at the policy section, so external readers understand why the Flutter app's version may jump.

## Test Plan

No automated tests required — change is non-functional (release tooling + documentation).

Manual verification at next release cut:

- Run `/release vX.Y.Z` against a clean tree.
- After the dry run, confirm every file in the canonical list shows `vX.Y.Z` (or the equivalent format-shifted form: e.g. pubspec uses `X.Y.Z+B`, gradle uses `versionName`).
- Confirm CHANGELOG entry mentions the unified-versioning policy on the first release that applies it.

## Notes

- **Format mismatches per ecosystem.** Flutter wants `1.0.0+1`, npm wants `1.0.0`, firmware uses the `vX.Y.Z` literal. The release script needs a small mapping table — pick one canonical `MAJOR.MINOR.PATCH` and project it into each format.
- **The Flutter `+B` build counter — decided 2026-05-01: increment forever.** `pubspec.yaml` versions look like `1.0.0+1` where `+1` is the Android `versionCode`. Each release increments the build number monotonically (1.0.0+1 → 0.5.0+2 → 0.5.1+3). Google Play requires a strictly increasing `versionCode`, and resetting it would complicate any future Play Store upload (TASK-160).
- **First-bump jump is large — decision: hard reset.** Flutter app goes 1.0.0 → next firmware bump (`v0.4.x` / `v0.5.0`), a semver downgrade. Decided 2026-05-01 to **hard reset** the Flutter app to the unified number and live with an explanatory note in the changelog. Trade-off accepted: the app is not yet published to Play Store (see TASK-160), so package-manager / store-update breakage is not an issue today. If publishing happens before this lands, revisit the decision.
- **Out of scope.** No automated check that all versions agree (a CI guard could be a follow-up). No version-bump tooling beyond extending the existing `/release` skill. No semantic-release / conventional-commit automation.
- **Related.** TASK-179 (determine Android app release approach) and TASK-160 (publish to Play Store) both intersect with versioning policy and should be revisited after this task lands.

## Documentation

- `docs/developers/CI_PIPELINE.md` — add a "Version policy" section: one number, all deliverables, no skip-bumps.
- `README.md` — short paragraph in the project structure / releases section noting that all deliverables ship under one version.
- `.claude/skills/release/SKILL.md` — update the bump step to cover every version-bearing manifest, with the mapping table (canonical → per-ecosystem format).
