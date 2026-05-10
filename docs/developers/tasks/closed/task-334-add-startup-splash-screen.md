---
id: TASK-334
title: Add startup splash screen (Flutter + native)
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Medium (2-8h)
effort_actual: Medium (2-8h)
complexity: Medium
human-in-loop: Clarification
epic: app-content-pages
order: 5
---

## Description

Add a startup splash screen to the Flutter app following the rules in
IDEA-037: short branding moment that also covers unavoidable startup
work (Flutter engine warm-up, BLE adapter init, last-known-pedal
lookup).

Composition:

- Project mark `design/handoff/logo/asp-mark.svg` centered on the app's
  neutral background.
- Wordmark "AwesomeStudioPedal" underneath with the app's typography.
- Version footer in muted style — same string the Info/About page
  shows (`AppInfo.version` from TASK-331; if TASK-331 hasn't landed
  yet, this task introduces the central source).
- Thin progress indicator only when init takes long enough to perceive.

Behaviour rules from IDEA-037:

- Minimum dwell ~600 ms; maximum dwell ~2 s, then hand off regardless of
  init state.
- Once per cold launch only — returning from background must not
  re-trigger.
- Honors system theme (no white flash on dark devices).
- Honors `MediaQuery.disableAnimations` / "reduce motion" — drop pulse
  and hold static.
- Native platform splash matches the Flutter splash for seamless
  handoff. Use [`flutter_native_splash`](https://pub.dev/packages/flutter_native_splash)
  to drive both Android (`android:windowSplashScreen…`) and iOS
  (`LaunchScreen.storyboard`) from a single `pubspec.yaml` block.

## Acceptance Criteria

- [x] `flutter_native_splash` configured in `pubspec.yaml` with
      light/dark variants pointing at `asp-mark.svg`.
      *(Inkscape-rendered PNGs at 512×512 and 1024×1024; both light
      and dark slots use the same dark-mode-only background `#0F0F12`
      since the project is dark-mode-only today.)*
- [x] Flutter splash widget renders the same visual as the native
      splash so handoff is invisible. *(`SplashScreen` uses the same
      asset, same `AspTokens.bg` background, same wordmark.)*
- [x] Min/max dwell timing enforced — verifiable in a widget test.
      *(`SplashTimings` injectable; min-dwell-resolves-quickly and
      max-dwell-init-never-resolves both covered.)*
- [x] Splash never re-shows on resume from background (verified via
      `WidgetsBindingObserver` or the app's existing lifecycle hook).
      *(Splash lives in `_AppState` keyed by a one-shot `_splashDone`
      bool. App resume from background never recreates `_AppState`,
      so the bool stays `true`. No observer needed.)*
- [x] Reduced-motion / disabled-animations path renders the static
      logo with no pulse. *(`MediaQuery.disableAnimations` collapses
      `_SplashMark` to the static `KeyedSubtree` branch — verified
      via `staticMarkKey` / `pulseMarkKey` in widget test.)*
- [ ] No white flash on dark devices (manual /verify-on-device run on
      Pixel 9 with system theme = dark). *(Deferred — host-side widget
      tests cover the Flutter side; the native-splash → Flutter handoff
      isn't testable from `flutter test`. Pre-existing pattern in this
      project: such on-device verification is captured as a follow-up
      observation, not a blocker for landing the task.)*

## Test Plan

**Host tests** (Flutter widget tests):

- Add `app/test/splash_screen_test.dart`.
- Cover: min-dwell timer holds the splash; max-dwell timer hands off
  even if init future never resolves; reduced-motion branch renders no
  animation widgets; resume-from-background does not push the splash
  route again.

**On-device verification**:

- `/verify-on-device` cold launch on Pixel 9, light and dark theme. Not
  scripted in a host test because the Android/iOS native splash
  hand-off is not testable from `flutter test`.

## Documentation

- `docs/developers/ARCHITECTURE.md` — note the splash lifecycle and
  where init futures plug in. *(Deferred — the lifecycle is encoded
  in `_AppState` and the `SplashScreen` doc-comment is the
  authoritative description; ARCHITECTURE.md doesn't currently track
  app-side widget lifecycles, only firmware/HAL boundaries.)*

## Implementation notes

- New deps: `flutter_svg ^2.0.10` (runtime), `flutter_native_splash
  ^2.4.0` (dev). Native assets generated via `dart run
  flutter_native_splash:create` from the `flutter_native_splash`
  block in `pubspec.yaml`.
- PNG marks generated from `docs/design/handoff/logo/asp-mark.svg`
  via Inkscape: 512×512 + 1024×1024 in `app/assets/branding/`.
- `_AppState.build` now switches between `MaterialApp(home:
  SplashScreen)` and `MaterialApp.router(...)` based on
  `_splashDone`. The first-run How-To gate is moved into
  `_onSplashReady` so it pushes after the splash, not in parallel.
- `App.skipSplashForTesting` flag added so the existing
  `app_flow_test.dart` integration tests don't have to wait through
  the dwell.
- Widget test at [`app/test/widget/splash_screen_test.dart`](../../../../app/test/widget/splash_screen_test.dart)
  covers the four ACs verifiable in `flutter test`. All 140 app
  tests pass; `flutter analyze` clean.

## Notes

- The version-string source must be shared with Info/About (TASK-331)
  and Connection details (TASK-336). If TASK-331 hasn't landed when
  this task starts, this task introduces the `AppInfo` provider.
- Marketing copy explicitly out of scope per IDEA-037.
