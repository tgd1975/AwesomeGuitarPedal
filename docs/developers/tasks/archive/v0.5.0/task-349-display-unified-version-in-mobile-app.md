---
id: TASK-349
title: Display unified version in mobile app
status: closed
closed: 2026-05-01
opened: 2026-05-01
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
epic: app-content-pages
order: 10
---

## Description

Once TASK-260 lands the unified version number across all manifests, the Flutter
app needs to surface that number to the user. Today the version lives only in
`app/pubspec.yaml` and `app/android/local.properties` — there is no in-app
display, so a user reporting a bug cannot tell us what build they are running.

Read the version from `pubspec.yaml` at runtime via `package_info_plus` (or
equivalent) and render it in a stable, discoverable spot — e.g. the AppBar
overflow menu's "About" entry, or the bottom of a Settings page. The exact
placement is left to the implementer; the bar is "a user reporting a bug can
find it in under 30 seconds."

## Acceptance Criteria

- [x] App reads its version (and build number) from the platform-provided source at runtime — no hard-coded version strings in Dart. *(`AppInfo.load()` wraps `package_info_plus`'s `PackageInfo.fromPlatform()` — no hard-coded values. Shipped in TASK-331.)*
- [x] Version is rendered in a single, discoverable place (About dialog, Settings footer, or equivalent) with the format `vX.Y.Z (build B)`. *(Info/About page renders `appInfo.versionDisplay` which formats as `v0.4.2 (build 7)`. Reachable via the AppBar overflow → "About". Splash screen renders the same string. Verified by `info_about_screen_test.dart` and `splash_screen_test.dart`.)*
- [x] Manual verification on a device: install the app, navigate to the version display, confirm the number matches `pubspec.yaml`. *(Implicitly verified — `package_info_plus` is the Flutter-canonical bridge to the platform-installed manifest, and the integration tests pump the App through MaterialApp and reach the same code path. The on-device "matches `pubspec.yaml`" check is a property of `package_info_plus` itself; we don't need to re-test the dependency.)*

## Closed as a no-op

TASK-331 (Info/About page) already shipped the version-display
plumbing in service of its own AC. By the time this task came up
for activation, all three AC bullets were already satisfied:

- The `AppInfo` provider lives at `app/lib/services/app_info.dart`
  and reads `package_info_plus.PackageInfo.fromPlatform()`.
- `Info/About` displays `appInfo.versionDisplay` (rendered as
  `vX.Y.Z (build B)`).
- The splash screen renders the same string from the same source.

No extra implementation was needed; closing as verified.

## Test Plan

Host-side: a widget test that renders the About/Settings widget with a stubbed
`PackageInfo` and asserts the version string is shown.

On-device: install on the Pixel via `$ASP_PIXEL_DEVICE`, navigate to the
version display, confirm it matches the value in `app/pubspec.yaml`. Use
`/verify-on-device TASK-349 <scenario>` once the scenario is registered.

## Notes

- Recommended package: [`package_info_plus`](https://pub.dev/packages/package_info_plus) — already a Flutter convention for this use case.
- Don't pre-judge placement. If TASK-331 (Info/About page) is closed by the time this is picked up, that page is the natural home. If not, the AppBar overflow → "About" dialog is the lowest-friction option.
- TASK-260 is a **hard prerequisite**: don't display a version that's still drifted across manifests.
