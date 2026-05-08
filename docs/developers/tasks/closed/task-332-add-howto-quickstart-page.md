---
id: TASK-332
title: Add How-To quickstart page (3–5 steps)
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: Clarification
epic: app-content-pages
order: 3
prerequisites: [TASK-331]
---

## Description

Add a How-To quickstart page that explains how to use the pedal in 3–5
steps, each one line, each with a screenshot or icon. Reuses the shared
content-page chrome from TASK-331.

Reference flow: power on → pair → pick profile → press button.

## Acceptance Criteria

- [x] Page reachable from the same content-page entry point as
      Info/About. *(Home AppBar overflow menu: "How to use" + "About".)*
- [x] 3–5 steps total, each one line plus one icon or screenshot.
      *(5 steps with `Icons.power_settings_new`,
      `Icons.bluetooth_searching`, `Icons.tune`, `Icons.upload`,
      `Icons.touch_app`.)*
- [x] Page fits one phone screen without scrolling on a Pixel 9.
      *(Body wrapped in `SingleChildScrollView` so small phones
      degrade gracefully; on Pixel 9 (412×915) the 5-step list fits.)*
- [x] If TASK-330 chose "context-aware How-To", the "Pair your pedal"
      step shows or hides based on current connection state. Otherwise
      static. *(Static — TASK-330 decision; smart helper deferred to
      [IDEA-053](../../ideas/open/idea-053-context-sensitive-helper-system.md).)*

## Implementation notes

- First-run auto-show: on app start, `_AppState.initState` checks
  `FirstRunGate.shouldAutoShowHowTo()` (reads
  `shared_preferences` key `howto_first_run_dismissed`) and pushes
  `/how-to?firstRun=1` if not yet dismissed. The first-run variant
  shows a prominent **"GOT IT, DON'T SHOW AGAIN"** button that flips
  the flag and pops back to home — per the TASK-330 decision.
- Home AppBar entry was migrated from a single info-icon button to a
  `PopupMenuButton` now that two content pages exist. Subsequent
  pages (Profiles explainer, Troubleshooting, Legal) will hang off
  the same menu.
- New dep: `shared_preferences: ^2.2.3`.
- Widget test at [`app/test/widget/how_to_screen_test.dart`](../../../../app/test/widget/how_to_screen_test.dart)
  covers: 5-step render, first-run dismiss button visibility +
  persistence, and `FirstRunGate` flag round-trip via
  `SharedPreferences.setMockInitialValues`. All 131 app tests pass;
  `flutter analyze` clean.

## Prerequisites

- **TASK-331** — provides the shared content-page chrome and the
  navigation entry point.

## Test Plan

**Host tests** (Flutter widget tests):

- Add `app/test/pages/howto_page_test.dart`.
- Cover: all step rows render; step count is between 3 and 5; (if
  context-aware) the pair step toggles correctly when a connection
  state stream emits connected/disconnected.

## Notes

- Per IDEA-037, this is "reduced to the max" — resist the urge to
  explain edge cases; that's the Troubleshooting page's job.
- First-run auto-show behavior depends on the TASK-330 decision and
  may add a small follow-up step here (read-and-set a "shown once"
  preference) if the answer was "auto-show".
