---
id: TASK-373
title: Fix cpp/comparison-with-wider-type findings in config_loader and config_loader_merge
status: open
opened: 2026-05-08
effort: Small (<2h)
complexity: Junior
human-in-loop: No
prerequisites: [TASK-372]
---

## Description

GitHub Advanced Security CodeQL reports two high-severity
`cpp/comparison-with-wider-type` alerts on profile-iteration loops in
[lib/PedalLogic/src/config_loader.cpp:154](lib/PedalLogic/src/config_loader.cpp#L154)
and
[lib/PedalLogic/src/config_loader_merge.cpp:65](lib/PedalLogic/src/config_loader_merge.cpp#L65).

Both loops use a `uint8_t` counter compared against `JsonArray::size()` which
returns `size_t`:

```cpp
for (uint8_t i = 0; i < profiles.size() && i < hardwareConfig.numProfiles; i++)
```

If `profiles.size()` ever exceeded 255, the `uint8_t` counter would wrap to
zero and the loop would never terminate. The companion guard
`i < hardwareConfig.numProfiles` (where `numProfiles` is itself `uint8_t`,
capped at 255) prevents this in practice on all current hardware, but the
CodeQL finding is structurally correct: the comparison itself is unsafe.

These two alerts (and TASK-372's vendored-deps config fix) together must
clear before the v0.5.0 release PR #9 can pass the GHAS code-scanning gate.

## Acceptance Criteria

- [ ] Both loops use `size_t` (or another type at least as wide as the value
      returned by `JsonArray::size()`) for the loop counter
- [ ] Inside the loop body, narrowing casts to `uint8_t` at API boundaries
      (`profileManager.addProfile(i, ...)`) are explicit and safe by
      construction (counter is bounded by `hardwareConfig.numProfiles`,
      which is `uint8_t`)
- [ ] CodeQL alerts #1 (config_loader.cpp) and #2 (config_loader_merge.cpp)
      close on the next scan after merge — confirmed via the merge PR's
      CodeQL run reporting zero remaining `cpp/comparison-with-wider-type`
      findings
- [ ] Existing host tests pass (`make test-host`)
- [ ] No new clang-tidy violations introduced

## Test Plan

**Host tests** (`make test-host`):

- Existing `test/unit/test_config_loader*.cpp` coverage of profile iteration
  must continue to pass.
- No new regression test added: this is a type-correctness fix only; runtime
  behaviour is unchanged because the existing
  `&& i < hardwareConfig.numProfiles` guard already capped `i` at `uint8_t`
  range. A test attempting to load > 255 profiles is not meaningful — the
  hardware config (`numProfiles`) is itself `uint8_t`, so the upstream cap
  applies before the loop runs.

## Prerequisites

- **TASK-372** — Configures CodeQL `paths-ignore` for vendored deps. Without
  it, the five GoogleTest false-positive alerts continue to fail the GHAS
  gate even after this task fixes the two real findings, so the v0.5.0
  release PR cannot merge.

## Notes

- Both alerts have the same shape, so the fix is duplicated across two files
  with identical structure.
- The CodeQL alerts will not auto-close on the same PR that fixes them —
  they only re-evaluate on a fresh scan against `main`. After this task
  merges to `main`, alerts #1 and #2 should disappear from the open list.
