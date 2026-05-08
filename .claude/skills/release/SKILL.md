---
name: release
description: Bump version, archive tasks, update CHANGELOG, tag, and push — release.yml then builds firmware + APK and publishes the GitHub Release
---

# release

Guide a release following the process in `docs/developers/CI_PIPELINE.md`.

The user invokes this as `/release vX.Y.Z` (e.g. `/release v1.3.0`).
If no version is given, read the current version from `include/version.h` and ask what the
new version should be.

Steps:

1. **Verify clean state**: run `git status`. If there are uncommitted changes, warn the
   user and stop — a release must be made from a clean working tree on `main`.

2. **Verify branch**: run `git branch --show-current`. If not on `main`, warn and stop.

3. **Read current version**: find the `#define FIRMWARE_VERSION` line in `include/version.h`
   and show it. This file is the **canonical source-of-truth** for the project-wide
   version. All other deliverables are bumped in lockstep with it (see TASK-260 and
   the "Version policy" section in `docs/developers/CI_PIPELINE.md`).

4. **Confirm the new version** with the user before making any changes. Remind the
   user, if their target deviates, that **all deliverables share one number** — even
   ones that did not change since the last release. We do not skip-bump unchanged
   artifacts.

5. **Bump version in every deliverable**. Canonical input is the `MAJOR.MINOR.PATCH`
   triple (e.g. `0.5.0`); each deliverable receives its format-projected form.

   Mapping table — canonical `X.Y.Z` projects per ecosystem:

   | File | Format | Projection of `X.Y.Z` |
   |---|---|---|
   | `include/version.h` | `#define FIRMWARE_VERSION "vX.Y.Z"` | `vX.Y.Z` (literal v-prefix) |
   | `package.json` | JSON `"version": "X.Y.Z"` | `X.Y.Z` (no v-prefix) |
   | `app/pubspec.yaml` | YAML `version: X.Y.Z+B` | `X.Y.Z+B` where `B = previous +B + 1` (monotonic; never reset) |
   | `awesome-task-system/VERSION` | plain text, single line | `X.Y.Z` (no v-prefix, newline-terminated) |
   | `docs/tools/version.js` | JS literal `const ASP_VERSION = 'vX.Y.Z'` | `vX.Y.Z` (literal v-prefix) — drives the stamp shown in the simulator and both builders |

   **Build counter `+B`** in `pubspec.yaml`: read the current `+B`, increment by 1.
   This becomes the Android `versionCode`. Google Play requires it to be strictly
   increasing, so **never reset** on a version bump.

   **Android `build.gradle.kts`** auto-derives `versionCode` and `versionName` from
   Flutter (`flutter.versionCode` / `flutter.versionName`); no manual edit needed.

   Edits to make:

   ```c
   // include/version.h
   #define FIRMWARE_VERSION "vX.Y.Z"
   ```

   ```json
   // package.json — bump the "version" field only
   "version": "X.Y.Z",
   ```

   ```yaml
   # app/pubspec.yaml — bump version, increment +B by 1
   version: X.Y.Z+B
   ```

   ```text
   # awesome-task-system/VERSION — replace contents
   X.Y.Z
   ```

   ```js
   // docs/tools/version.js — bump the ASP_VERSION literal only
   const ASP_VERSION = 'vX.Y.Z';
   ```

   **CLI / simulator manifests**: TASK-260 anticipates a CLI and standalone simulator
   that do not exist yet. When they materialize, add their version-bearing files to
   this table (e.g. `cli/setup.py`, simulator `package.json`). The web simulator at
   `docs/simulator/` is part of the docs site; its displayed version comes from
   `docs/tools/version.js` (already in the table above), not a separate manifest.

   **Verify lockstep**: after the five edits, `grep -E '0\.[0-9]+\.[0-9]+' include/version.h package.json app/pubspec.yaml awesome-task-system/VERSION docs/tools/version.js`
   should show the new `X.Y.Z` (with the `+B` suffix on pubspec, with the `v`
   prefix on `version.h` and `version.js`) in all five files. If any one is
   missing, fix it before continuing.

6. **Archive closed tasks**: move every flat `.md` file in `docs/developers/tasks/closed/`
   into `docs/developers/tasks/archive/vX.Y.Z/` using `git mv`. Tasks in `open/`,
   `active/`, and `paused/` are **not** archived — they carry forward into the next
   release. Paused inherits open/active behavior here: a paused task does not block
   the release, does not appear shipped, and stays in `paused/` until it is resumed
   or closed in a future release.

   ```bash
   mkdir -p docs/developers/tasks/archive/vX.Y.Z
   for f in docs/developers/tasks/closed/*.md; do
     git mv "$f" docs/developers/tasks/archive/vX.Y.Z/
   done
   ```

7. **Regenerate task overview** (regenerates OVERVIEW.md, EPICS.md, KANBAN.md, and `archive/vX.Y.Z/OVERVIEW.md`):

   ```bash
   python scripts/housekeep.py --apply
   git add docs/developers/tasks/
   ```

8. **Snapshot OVERVIEW / EPICS / KANBAN into archive/vX.Y.Z/.** Freezes the
   post-archive state of the three top-level overviews into the per-release
   folder, renamed with the version as a suffix, with auto-generation
   markers and notices stripped so they read as static historical
   artifacts (housekeep will not touch them on later runs):

   ```bash
   python scripts/release_snapshot.py vX.Y.Z
   git add docs/developers/tasks/archive/vX.Y.Z/
   ```

   Result: `archive/vX.Y.Z/{OVERVIEW,EPICS,KANBAN}_vX.Y.Z.md` next to
   the existing `archive/vX.Y.Z/OVERVIEW.md` (the per-release closed-tasks
   listing — unchanged).

9. **Update CHANGELOG**: read `CHANGELOG.md`. Find the `## [Unreleased]` section.

- If the section is **empty**, note it to the user and still update the heading —
     the section will be empty in the versioned entry.
- Replace `## [Unreleased]` with:

     ```
     ## [Unreleased]

     ---

     ## [vX.Y.Z] — YYYY-MM-DD
     ```

   Keep all existing `[Unreleased]` content under the new versioned heading.
   Use today's date (`date +%Y-%m-%d`). Stage the file:

   ```bash
   git add CHANGELOG.md
   ```

1. **Update README firmware section**: replace the block between
   `<!-- RELEASE_SECTION_START -->` and `<!-- RELEASE_SECTION_END -->` in `README.md`
   with the real download links. Read the existing block first to detect whether
   previous releases are already listed, then write the updated block:

   ```
   <!-- RELEASE_SECTION_START -->
   **Current stable: vX.Y.Z**

   - ESP32: [awesome-pedal-esp32-vX.Y.Z.bin](../../releases/download/vX.Y.Z/awesome-pedal-esp32-vX.Y.Z.bin)
   - nRF52840: [awesome-pedal-nrf52840-vX.Y.Z.bin](../../releases/download/vX.Y.Z/awesome-pedal-nrf52840-vX.Y.Z.bin)

   **Previous releases** (current + 2 kept):

   <keep up to 2 previous release entries here, remove older ones>
   <!-- RELEASE_SECTION_END -->
   ```

   Stage the file:

   ```bash
   git add README.md
   ```

2. **Commit the bump, archive, changelog, and README**.

   This is a multi-file release operation that does not fit the
   `/commit` pathspec model (the archive directory contents are not
   knowable as a flat file list). Use the
   `ASP_COMMIT_BYPASS=<reason>` mechanism documented in
   [docs/developers/COMMIT_POLICY.md](../../../docs/developers/COMMIT_POLICY.md);
   the bypass is logged to `.git/asp-commit-bypass.log` for review:

   ```bash
   git add include/version.h package.json app/pubspec.yaml awesome-task-system/VERSION docs/tools/version.js
   ASP_COMMIT_BYPASS="release: version bump + archive + CHANGELOG" \
       git commit --no-verify -m "chore: bump version to vX.Y.Z, archive closed tasks, update CHANGELOG"
   ```

   `--no-verify` is kept here because `/release` runs the full release
   build separately; re-running the pre-commit chain (tests + clang-tidy
   - mermaid) would duplicate that work.

3. **Create annotated tag**:

    ```bash
    git tag -a vX.Y.Z -m "Release vX.Y.Z"
    ```

4. **Show a final summary** of what will be pushed (commit + tag), then ask the user to
    confirm before pushing.

5. **Push commit and tag**:

    ```bash
    git push origin main
    git push origin vX.Y.Z
    ```

    The tag push triggers `.github/workflows/release.yml`, which builds the
    ESP32 + nRF52840 firmware and Android APK, creates the GitHub Release,
    and attaches all artifacts. Nothing further runs locally — no `pio run`,
    no `gh release create`, no temp-file cleanup. See
    [docs/developers/CI_PIPELINE.md](../../../docs/developers/CI_PIPELINE.md)
    for the workflow's responsibilities.

    Watch the workflow run to confirm it succeeds:

    ```bash
    gh run watch
    ```

    If the run fails, the GitHub Release will not be created (or will be
    incomplete). Re-running is safe once the workflow is fixed: delete the
    failed release and re-trigger by re-pushing the tag, or push a patch
    bump.

Do not push without explicit user confirmation after step 4.
