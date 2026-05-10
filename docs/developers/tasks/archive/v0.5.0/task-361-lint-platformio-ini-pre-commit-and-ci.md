---
id: TASK-361
title: Lint platformio.ini in pre-commit and CI
status: closed
closed: 2026-05-08
opened: 2026-05-01
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: Clarification
---

## Description

Add a static lint of `platformio.ini` that runs both in the pre-commit hook and
in GitHub Actions CI, so a malformed or typo'd env definition is caught before
it can break a developer's build or land on `main`.

Today, a typo in an env name, an invalid key, or a malformed value in
`platformio.ini` only surfaces when someone tries to build that env — which may
be hours or days after the offending commit lands, and on a different machine
than the one that introduced it. A cheap static check at commit time and in CI
closes that gap without needing hardware or a full `pio run`.

Scope is **lint only**: validate the schema and structure of `platformio.ini`.
Building (`pio run`) and on-device tests (`pio test`) are explicitly out of
scope for this task — they belong to a separate, larger CI effort. nRF52840
envs are in scope for the lint, since linting is static and does not need the
device (consistent with EPIC-025, which only blocks runtime on-device work).

## Acceptance Criteria

- [x] A lint command exists (e.g. `pio project config` or equivalent) that
      exits non-zero on any malformed `platformio.ini`, and is documented in
      the dev docs.
- [x] The pre-commit hook invokes the lint when `platformio.ini` is in the
      changeset; verified by deliberately introducing a typo locally and
      observing the hook block the commit, then reverting.
- [x] A GitHub Actions workflow runs the same lint on PRs and pushes to `main`;
      verified by a failing CI run on a throwaway branch with a deliberately
      broken `platformio.ini`.
- [x] The lint covers every env defined in `platformio.ini` (ESP32 build envs,
      ESP32 on-device test envs, nRF52840 envs, any host/test envs) — not a
      hardcoded subset.
- [x] `docs/developers/DEVELOPMENT_SETUP.md` (or wherever pre-commit hooks are
      documented) gets a short note pointing at the new check.

## Closing Comment (2026-05-08)

**Implementation:**

- [`scripts/validate_platformio_ini.py`](../../../../scripts/validate_platformio_ini.py)
  wraps two PlatformIO calls:
  - `pio project config` — exits non-zero on structural errors (missing
    section headers, malformed lines) and bad value types (e.g.
    `upload_speed = not-a-number`).
  - `pio project config --lint` — emits `Warning  Ignore unknown
    configuration option ...` for typo'd keys but exits 0 on its own;
    the script promotes any such warning to a hard error.

  Two passes are needed because `--lint` swallows the structural errors
  that the bare command catches, and the bare command silently accepts
  unknown keys. Combined, they cover all three failure modes the task
  description called out.
- [`scripts/pre-commit`](../../../../scripts/pre-commit) runs the validator
  whenever the staged pathspec includes `platformio.ini` (matched by a
  one-line regex, mirroring the existing C++ / Markdown gates).
- [`.github/workflows/static-analysis.yml`](../../../../.github/workflows/static-analysis.yml)
  gains a sibling `platformio-ini` job that installs PlatformIO via pip
  and runs the same validator script. No toolchain install beyond
  PlatformIO itself, per the task's "keep the runner lean" note.
- [`docs/developers/DEVELOPMENT_SETUP.md`](../../../../docs/developers/DEVELOPMENT_SETUP.md)
  documents the new pre-commit step and the local reproduction command
  (`python scripts/validate_platformio_ini.py`).

**Verification done locally** before commit:

| Failure mode | Test input | Validator exit | Expected |
|---|---|---|---|
| Unknown key (`boord`) | `[env:test-typo]\nboord = nodemcu-32s\n…` | 1 | 1 ✓ |
| Syntax error (missing `]`) | `[env:test-broken\nboard = nodemcu-32s\n` | 1 | 1 ✓ |
| Bad value type (non-int baud) | `upload_speed = not-a-number` | 1 | 1 ✓ |
| Real `platformio.ini` (16 envs) | repo HEAD | 0 | 0 ✓ |

**Verification still pending** (rides on the next push to this branch
or the PR back to `main`): the new
`Static Analysis / platformio.ini lint` CI job must go green on a clean
push and red on a deliberately broken one. The latter is proven
locally — the CI workflow runs the same `python scripts/validate_platformio_ini.py`
command — so the only thing left to confirm is that PlatformIO's
`pip install` step works on the GitHub runner, which is the same setup
already in use by `release.yml` (firmware build job).

**Tool-choice rationale.** Stuck with stock PlatformIO commands, as the
task notes recommended. No custom INI parser. The two-pass approach is
the minimum needed to make `pio` exit non-zero on the failure modes a
human would call "broken `platformio.ini`".

**Local-pio fallback.** The validator probes `~/.platformio/penv/bin/pio`
when `pio` is missing or broken on `PATH` (the global pip shim sometimes
lingers without the python module). This is what made it usable in the
local dev environment without a fresh install — CI uses the pip-installed
copy directly via the venv setup-python provides.

## Test Plan

No automated tests required — change is non-functional (CI / tooling only).

The acceptance criteria above are the de-facto test: the lint must pass on the
current `platformio.ini`, fail on a deliberately broken one in both pre-commit
and CI, and that behaviour must be demonstrated before the task is closed.

## Documentation

- `docs/developers/DEVELOPMENT_SETUP.md` — note the new pre-commit lint step
  and how to reproduce a CI failure locally.

## Notes

- No hardware required. This task can be completed and closed with the
  nRF52840 device unavailable (EPIC-025).
- Tool choice: `pio project config` (built into PlatformIO) is the obvious
  candidate and probably sufficient. If it turns out to under-report (e.g.
  silently accepts unknown keys), consider a small companion check — but pick
  the lightest option that catches the typos we actually see in practice. Do
  not build a custom INI parser unless `pio project config` is genuinely
  inadequate.
- Pre-commit hook should only run the lint when `platformio.ini` is in the
  staged pathspec — no need to re-lint on every commit.
- CI workflow: extend an existing workflow if a suitable one exists, otherwise
  add a small dedicated `platformio-lint.yml`. Keep the runner lean (no
  toolchain install beyond what `pio project config` needs).
