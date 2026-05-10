---
id: TASK-179
title: Add the Android app to the GitHub release pipeline
description: Build a debug-signed APK in release.yml on tag push and attach it to the GitHub Release alongside firmware artifacts; fix the broken nRF52840 firmware path while we're in there
status: closed
closed: 2026-05-02
opened: 2026-04-10
effort: Medium (2-8h)
effort_actual: Medium (2-8h)
complexity: Junior
human-in-loop: No
epic: distribution
order: 1
---

## Description

When this task was opened, "release the Android app via GitHub" was an open
question. Discovery established two facts that reshape the scope:

1. The intended release path is `release.yml` (GitHub-native CI). The local
   `/release` skill that has been producing all firmware artifacts to date is
   a workaround — its `pio run` + `gh release create` steps duplicate what
   `release.yml` should be doing, and the workflow path has been silently
   failing since v0.3.0 due to an nRF52840 rename bug (copying `firmware.bin`
   from a build env that produces `.hex` + `.zip`).
2. Real Android signing is **out of scope** here. With no current user base
   and no Play Store onboarding pending, sideload distribution is the only
   target, and a debug-signed APK satisfies it. (Real signing planning lives
   in TASK-160, including the rationale captured in its design notes.)

So this task fixes `release.yml` end-to-end so it actually produces the
expected artifact set — including the new APK — and leaves the `/release`
skill thinning to TASK-364 (which is gated on this task landing first).

### Decisions reached during planning

- **Architecture**: `release.yml` is the canonical release path. The
  `/release` skill becomes a thin "bump version, archive, push tag" wrapper
  in TASK-364; everything else moves to CI.
- **Signing**: debug-signed APK for sideload. A stable debug keystore is
  committed to the repo so all builds (CI and local) sign with the same key,
  which lets sideload users upgrade in place between releases.
- **Format**: APK only on the GitHub Release. AAB is a Play-Store-only format
  added in TASK-160 (uploaded as a workflow artifact, never to the public
  Release page).
- **Naming convergence**: workflow adopts the skill's existing naming
  (`firmware-<board>-<ver>.<ext>`) so existing README links and external
  bookmarks keep working.
- **`applicationId` rename**: deferred to TASK-160 (KISS — no users to break).

## Acceptance Criteria

- [x] `release.yml` nRF52840 build packages `firmware.hex` + `firmware.zip`
      (DFU/OTA), not the nonexistent `firmware.bin`
- [x] `release.yml` ESP32 build packages both `firmware-nodemcu-32s-vX.Y.Z.bin`
      and `firmware-nodemcu-32s-vX.Y.Z-debug.zip` (zipped `.elf` for symbols)
- [x] `release.yml` has a new `flutter-quality` job (analyze + test) that
      gates the new `build-android` job
- [x] `release.yml` has a new `build-android` job that runs
      `flutter build apk --release` and uploads `awesome-studio-pedal-vX.Y.Z.apk`
- [x] `release.yml` `release` job depends on all three build jobs and uploads
      the full 5-artifact set: ESP32 `.bin` + debug `.zip`, nRF52840 `.hex`
      \+ `.zip`, Android `.apk`
- [x] A stable debug keystore is committed at
      `app/android/keystores/debug.keystore` so CI and local APKs share a
      signing key (sideload users can upgrade in place between releases)
- [x] `app/android/app/build.gradle.kts` points the `debug` signingConfig
      at the committed keystore; `release` buildType continues to use it
- [x] `app/android/.gitignore` allows the committed keystore via negation
      while keeping all other `.keystore`/`.jks` files ignored
- [x] Local `flutter build apk --release` succeeds and the APK is signed by
      the committed keystore (verified with `apksigner verify --print-certs`)
- [x] Verify the workflow end-to-end with a throwaway tag — done on
      `feature/config-extensions` with `v0.4.2-rc1` (run 25244551900):
      all 5 artifacts produced and uploaded with the expected names
      (`firmware-nodemcu-32s-v0.4.2-rc1.bin` 693 KB,
      `firmware-nodemcu-32s-v0.4.2-rc1-debug.zip` 6.0 MB,
      `firmware-feather-nrf52840-v0.4.2-rc1.hex` 525 KB,
      `firmware-feather-nrf52840-v0.4.2-rc1.zip` 187 KB,
      `awesome-studio-pedal-v0.4.2-rc1.apk` 55.7 MB). Release + tag
      deleted afterwards. Final archival step failed cleanly because
      `organize_closed_tasks.py` requires strict `vX.Y.Z` — see notes
      below for follow-up suggestion.

## Test Plan

No automated tests — the change is in CI workflow YAML, build configuration
(Gradle), a `.gitignore` exception, and a committed keystore binary.

Local verification done during implementation:

- `python -c "import yaml; yaml.safe_load(...)"` on `release.yml` — passes
- `flutter build apk --release` from `app/` — succeeds (55.8 MB APK)
- `apksigner verify --print-certs` on the resulting APK — confirms it's
  signed by the committed keystore (DN `CN=Android Debug, O=AwesomeStudioPedal, C=DE`,
  SHA-256 `cc:1d:62:43…`)
- `flutter analyze` from `app/` — clean
- `flutter test` from `app/` — 159 tests pass

End-to-end workflow verification deferred to the post-merge AC: push a
`vX.Y.Z-rc1` tag, watch `release.yml`, confirm the produced GitHub Release
has all 5 artifacts with the expected names, then delete the tag and the
release.

## Notes

- TASK-364 ("Thin out /release skill") was scaffolded to cover removing the
  now-redundant local firmware-build + `gh release create` steps from the
  skill. TASK-364 has TASK-179 as a prerequisite — release.yml has to be
  proven before the skill's safety-net path is removed.
- Design context that bears on TASK-160 (Play Store work) was extracted into
  a "Design notes (extracted from TASK-179 planning, 2026-05-02)" section in
  that task's body, so the AAB / Play App Signing / dual-channel discussion
  doesn't get lost.
- The committed debug keystore is intentionally non-secret (well-known
  password `android`, alias `androiddebugkey`). Anyone signing their own APK
  with this same keystore could publish a fake "AwesomeStudioPedal" sideload
  build that Android would treat as upgrade-compatible with the official one.
  This risk is acceptable for sideload distribution at this scale; the moment
  Play Store onboarding happens (TASK-160), AAB signing uses a real keystore
  held in GitHub Secrets and the attack surface narrows naturally.
- Pre-existing `release.yml` quirks left untouched (out of scope here): the
  Node 20 deprecation warnings on `actions/checkout@v4` and
  `actions/setup-python@v5`, and the post-release `organize_closed_tasks.py`
  step that has never actually run because the release job was always
  unreachable. Both are candidates for a separate cleanup task if they
  become problematic.
- Verification surfaced one minor follow-up: the `Archive closed tasks`
  step in release.yml fails on prerelease tags (`*-rc*`, `*-beta*`)
  because `organize_closed_tasks.py` enforces strict `vX.Y.Z`. Cheapest
  fix is `if: ${{ !contains(steps.tag.outputs.value, '-') }}` on the
  archive + commit steps so prereleases skip them. Not blocking — real
  releases use `vX.Y.Z` and work fine — but worth a small task if more
  prerelease verifications are anticipated.
