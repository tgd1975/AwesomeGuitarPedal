# CI Pipeline

## Pre-commit hook

The pre-commit hook runs `make test-host` before every commit. If host tests fail, the commit is
blocked. Do not bypass this with `--no-verify`.

It also runs markdownlint on staged `.md` files.

## GitHub Actions — CI

File: `.github/workflows/test.yml`

Triggers on every push and on pull requests to `main`.

| Job | What it does |
|-----|-------------|
| `cpp-format` | Runs clang-format check across C++ sources |
| `markdown-lint` | Runs markdownlint-cli2 on root `*.md` files |
| `unit-tests` | CMake build + GoogleTest run + publishes test results |

All jobs must pass before a PR can be merged.

## GitHub Actions — Docs

File: `.github/workflows/docs.yml`

Triggers on push to `main` and on manual dispatch.

| Job | What it does |
|-----|-------------|
| `build-docs` | Installs Doxygen, runs `doxygen Doxyfile`, uploads Pages artifact |
| `deploy-docs` | Deploys the artifact to GitHub Pages |

**Prerequisite:** GitHub repo → Settings → Pages → Source must be set to **GitHub Actions**.
This is a one-time manual step in the repository settings.

## Doxygen configuration

The `Doxyfile` at the repository root configures Doxygen. Input directories:

- `src/`
- `include/`
- `lib/PedalLogic/`

Output: `docs/api/html/`. The `docs/api/` directory is gitignored — generated output is never
committed.

## Release process

Releases are cut via the `/release vX.Y.Z` skill. It bumps the version in every
deliverable, archives closed tasks, updates `CHANGELOG.md` and the README firmware
section, tags, pushes, builds firmware for both targets, and publishes a GitHub
Release with the artifacts attached. See `.claude/skills/release/SKILL.md` for the
exact step list.

### Version policy

**All deliverables share one version number. We do not skip-bump unchanged
artifacts.** When firmware goes from `v0.4.1` to `v0.5.0`, the Flutter app, root
`package.json`, and `awesome-task-system/VERSION` all move to the same number —
even if no code in those layers changed since the previous release.

The simplification is worth more than the technical correctness of "this artifact
really is unchanged since v0.2.0." It removes a class of "which version of which
piece is in this build?" questions, makes bug reports unambiguous, and lets the
release skill be a one-button operation.

**Canonical source:** `include/version.h` (`FIRMWARE_VERSION`). Every other file
is a projection of that `MAJOR.MINOR.PATCH` triple into its ecosystem's format.

**Per-deliverable formats:**

| Deliverable | File | Format |
|---|---|---|
| Firmware | `include/version.h` | `vX.Y.Z` (literal `v` prefix) |
| Root npm package | `package.json` | `X.Y.Z` |
| Flutter app | `app/pubspec.yaml` | `X.Y.Z+B` (see build-counter rule below) |
| Task system tooling | `awesome-task-system/VERSION` | `X.Y.Z` |

The Android `versionCode` / `versionName` in `app/android/app/build.gradle.kts`
are derived from Flutter's `flutter.versionCode` / `flutter.versionName`, so no
manual edit is needed there.

**Build counter `+B`** in `pubspec.yaml`: increments by 1 on every release,
**never resets** on a version bump. This becomes the Android `versionCode`, which
Google Play requires to be strictly monotonic for any future store upload
(see TASK-160).

**Adding new deliverables:** when the planned CLI or simulator (TASK-119, TASK-120
and friends) lands, add its version-bearing manifest to the table above and to
the bump step in `.claude/skills/release/SKILL.md`. The web simulator at
`docs/simulator/` is part of the docs site and has no separate version file.
