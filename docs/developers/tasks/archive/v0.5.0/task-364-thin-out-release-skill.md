---
id: TASK-364
title: Thin out /release skill — drop firmware build and gh release create, let CI handle release artifacts
status: closed
closed: 2026-05-02
opened: 2026-05-02
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: No
epic: distribution
order: 3
prerequisites: [TASK-179]
---

## Description

Once TASK-179 has fixed `release.yml` and made it the canonical path for building
firmware + APK artifacts and creating the GitHub Release, the local `/release`
skill should be thinned to stop duplicating that work.

Today the skill (`.claude/skills/release/SKILL.md`) does both:

- Bumps version, archives closed tasks, updates CHANGELOG, updates README, commits,
  tags, pushes — local prep work that has to happen on the maintainer's machine.
- Then *also* runs `pio run` for ESP32 + nRF52840 locally, and calls
  `gh release create` to attach the firmware artifacts to the GitHub Release.

The second half is what `release.yml` should do (and after TASK-179, will reliably
do). The skill keeps the first half — those are not CI-suitable steps because they
depend on local repo state, the version bump must precede the tag push, and the
CHANGELOG/README edits must be in the commit that the tag points to.

## Acceptance Criteria

- [ ] `.claude/skills/release/SKILL.md` no longer contains the `pio run -e nodemcu-32s`
      / `pio run -e feather-nrf52840` build steps
- [ ] `.claude/skills/release/SKILL.md` no longer contains the `gh release create` step
      that attaches firmware artifacts
- [ ] `.claude/skills/release/SKILL.md` no longer contains the artifact-cleanup `rm`
      step (since no local artifacts are produced)
- [ ] The skill ends after `git push origin main && git push origin vX.Y.Z` with a
      note explaining that `release.yml` takes over from there (and a one-liner on
      how to monitor the workflow run, e.g. `gh run watch`)
- [ ] `awesome-task-system/` mirror copy is updated alongside the live copy
      (per the project's task-system source-of-truth rule); `sync_task_system.py
      --check` passes
- [ ] First release after this change produces the same artifact set on the GitHub
      Release as today (verified against TASK-179's published artifacts)

## Test Plan

No automated tests — this is a skill (markdown) change with no executable code path.

Verification is end-to-end on the next real release: cut a release using the thinned
skill, confirm `release.yml` produces the expected artifact set on the GitHub Release
page, and confirm no local `firmware-*.bin` / `firmware-*.zip` files are left in the
working tree afterwards.

If a dry-run is wanted before a real release, the throwaway-tag mechanism from
TASK-179's verification step (`vX.Y.Z-rc1` tag, inspect, delete) can be re-used here.

## Prerequisites

- **TASK-179** — fixes `release.yml` (nRF52840 rename bug), adds zip artifacts, adds
  the Android APK build, and makes the workflow the de facto canonical path for
  release artifact production. Removing the duplicate steps from the skill is only
  safe once the workflow path actually works end-to-end and matches the artifact
  set the skill produces today.

## Notes

- This is a coordination/cleanup task, not a feature change. After TASK-179 ships,
  the skill is duplicated work that risks drift between the two paths. Land this
  promptly to avoid a window where both paths produce artifacts (and possibly with
  conflicting filenames or contents).
- Reminder per CLAUDE.md "Task-system source-of-truth": edit the
  `awesome-task-system/` package copy of the skill first, then run
  `python scripts/sync_task_system.py --apply` to mirror to the live copy under
  `.claude/skills/release/SKILL.md`. Don't edit the live copy directly.
- Out of scope here: any deeper refactor of the skill (splitting it, reorganising
  the version-bump table, etc.). The task is strictly "remove the build + upload
  steps that release.yml now owns."
