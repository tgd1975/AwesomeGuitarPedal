# Android Test Matrix

The Flutter app (`app/`) has historically only been tested on a single phone, and that
context was nowhere on record. Defects like TASK-250 (manifest BLE permissions) and
TASK-258 (in-app scan empty) are sensitive to Android version — what works on one OS
release can silently fail on another. This document records the Android OS version that
each manual test run was performed against, so the "did we actually test this on the
same OS we're shipping to?" question stays answerable.

The matrix intentionally starts minimal. We track **OS-version rows only** — one row
per (Android version × app version × date) combination. If a defect ever turns out to
be vendor-specific rather than OS-version-specific (e.g. Samsung BLE stack quirks,
Xiaomi battery-saver killing scans), retro-fit a Device column at that moment. Don't
pre-engineer.

## Rule

**When you run any manual test of the Flutter app, append a row to the table below
before closing the related task or PR.** Date, app version, and Android version are
the minimum; add a free-form note for anything noteworthy (defects found, scenarios
deferred, environment quirks).

## Test runs

| Date | Android version | App version | Result | Notes |
|------|----------------|-------------|--------|-------|
| 2026-04-26 / 2026-04-27 | 16 (SDK 36) | `1.0.0+1` (pubspec) | pass (with defects) | Retroactive first row. All FEATURE_TEST_PLAN.md app tests (TASK-153/154/155/156) executed against Pixel 9 / Android 16. Surfaced TASK-261, TASK-264, TASK-265, TASK-266, TASK-267, TASK-273, TASK-276, TASK-277. See [FEATURE_TEST_PLAN.md](FEATURE_TEST_PLAN.md) for per-scenario detail. |
