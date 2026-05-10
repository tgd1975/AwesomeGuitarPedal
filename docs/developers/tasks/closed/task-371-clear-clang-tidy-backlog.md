---
id: TASK-371
title: Clear clang-tidy backlog so static-analysis is green on main
status: closed
closed: 2026-05-08
opened: 2026-05-08
effort: Medium (2-8h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
related: TASK-362
---

## Description

The `clang-tidy` job in `.github/workflows/static-analysis.yml` has been
failing on every push to `main` for at least 5 consecutive commits.
[TASK-362](task-362-require-ci-pass-before-merge-to-main.md) (CI merge
gate) defers `clang-tidy` from the initial required-checks list because
the failure is not one bug — it is dozens of independent violations
across `lib/PedalLogic/`. This task clears that backlog so `clang-tidy`
can be promoted to a required check.

Categories observed in the latest log
(run `25080496843`, commit `083c6ef0`):

- `readability-braces-around-statements` — many sites in
  `ble_config_reassembler.{h,cpp}`, `send_action.h`, `config_loader.cpp`,
  `event_dispatcher.cpp`. Mechanical fix.
- `cppcoreguidelines-macro-usage` — `MAX_CONFIG_BYTES` and
  `JSON_DOC_CAPACITY` in `ble_config_reassembler.h`. Convert to
  `constexpr` constants.
- `performance-enum-size` — `UploadCheck` in
  `ble_config_reassembler.cpp`. Specify a smaller base type
  (`std::uint8_t`).
- `clang-diagnostic-implicit-int-conversion` — at least one site in
  `ble_config_reassembler.cpp:61`. Add an explicit cast or widen the
  destination type.
- `readability-function-cognitive-complexity` —
  `populateProfileFromJson` in `config_loader.cpp` (cognitive
  complexity 26, threshold 25). Refactor the function or, if
  decomposition is genuinely worse, raise the threshold globally with
  a written justification.

Once `clang-tidy` is green, return to TASK-362 and add it to the
required-checks list documented in `DEVELOPMENT_SETUP.md`, then update
branch protection on `main` to include the check.

## Acceptance Criteria

- [ ] `clang-tidy` runs cleanly (exit 0) in
      `.github/workflows/static-analysis.yml` on a PR built from this
      branch — every category in the Description is resolved.
- [ ] No global rule disables added to `.clang-tidy`. Per-site
      `// NOLINT(<rule>)` comments each include a one-line
      justification.
- [ ] Host unit tests (`make test-host`) still pass.
- [ ] PR description references TASK-362 and notes that the gate-list
      update is the follow-up there (or in a re-opened TASK-362).

## Test Plan

**Host tests** (`make test-host`):

- All existing host tests under `test/unit/` must still pass after the
  refactors. No new test files are required — the work is internal
  cleanup, not behavioural change.

**Static-analysis** (`/clang-tidy`, then push to PR for CI run):

- `/clang-tidy` locally must report no violations.
- The CI job `Static Analysis › clang-tidy` must report success on the
  PR.

No on-device tests required — every fix lives in platform-agnostic
code under `lib/PedalLogic/`.

## Notes

- Do not silence rules globally in `.clang-tidy`. Fix at the site, or
  add a per-line `// NOLINT(<rule>)` with a one-line reason. The
  reason is the load-bearing part — without it the suppression rots
  silently.
- `populateProfileFromJson` is one over the cognitive-complexity
  threshold. If the decomposition makes the function harder to read,
  the right move may be to raise the threshold rather than refactor —
  call the choice in the PR.
- Until this task closes, TASK-362 documents `clang-tidy` as
  *planned-required* alongside `Schemdraw staleness guard`
  ([TASK-370](task-370-make-schemdraw-svgs-deterministic.md)). When this lands,
  promote `clang-tidy` to a real required check on `main`.
