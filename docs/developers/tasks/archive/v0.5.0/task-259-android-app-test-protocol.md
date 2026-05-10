---
id: TASK-259
title: Android app test protocol — record device and Android version per test run
status: closed
closed: 2026-04-30
opened: 2026-04-26
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: Clarification
---

## Description

The Android app has so far only been tested on a single phone, and there is no record anywhere of *which* phone or *which* Android version. Defects like the recent BLE permission/scan issues (TASK-250, TASK-258) show that Android-version differences matter — what works on one OS version may silently fail on another.

This task creates a lightweight, central test-protocol document so that every future Android test run leaves a trace of the device and OS context. The shape of that document is intentionally unsettled — start minimal (probably just OS-version rows), upgrade to per-device rows only if and when that resolution proves insufficient.

## Acceptance Criteria

- [ ] A new file `docs/developers/ANDROID_TEST_MATRIX.md` exists, with a short intro and a markdown table of recorded test runs.
- [ ] The currently used phone + Android version is recorded as the first row, retroactively (with whatever date / app version makes sense — best-effort, not invented).
- [ ] The doc states the rule: "When testing the Android app, append a row before closing the related task / PR."
- [ ] The doc is linked from `docs/developers/TESTING.md` (or wherever the project's test-policy entrypoint lives) so people stumble onto it.

## Test Plan

No automated tests required — change is non-functional (documentation only).

## Notes

- **Open question on granularity.** Two plausible table shapes:
  - **OS-version rows only** (`Android 13 | app v0.4.0 | 2026-04-26 | pass`). Simplest. Sufficient if defects cluster by OS version and not by vendor.
  - **Per-device rows** (`Pixel 7 | Android 13 | …`). More complete but more bookkeeping. Worth it only if vendor-specific bugs start appearing (e.g. Samsung BLE stack quirks, Xiaomi battery-saver killing scans).

  Start with OS-version rows. If a defect ever depends on the manufacturer rather than the OS version, retro-fit a Device column at that moment.
- **Scope is deliberately open.** Don't over-engineer the matrix upfront. The doc should be one table and one paragraph of intro. Anything more elaborate (CI integration, automated device-info capture, BrowserStack-style fleet) belongs in a separate follow-up task if it ever earns its complexity.
- **Related defects.** TASK-250 (Android manifest BLE permissions) and TASK-258 (in-app scan empty) — both Android-version-sensitive. The matrix would have made the "did we test this on the same OS version?" question trivially answerable.
- **Not a test-suite task.** This records *that* a manual test happened on a given device + OS, not the test cases themselves. The cases live in feature-test tasks under `epic-014-feature-test`.
