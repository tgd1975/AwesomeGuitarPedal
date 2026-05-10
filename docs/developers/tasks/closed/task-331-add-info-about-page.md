---
id: TASK-331
title: Add Info/About page to the Flutter app
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Medium (2-8h)
effort_actual: Medium (2-8h)
complexity: Medium
human-in-loop: Clarification
epic: app-content-pages
order: 2
prerequisites: [TASK-330]
---

## Description

Add an Info/About page to the Flutter app, reachable from a single entry
point (top-bar info icon or overflow menu — pick once, reuse for all
content pages). The page is the first content surface added under
EPIC-023, so it also establishes the shared "content page" chrome that
the How-To, Profiles explainer, Troubleshooting, and Legal pages will
reuse.

What the page shows:

- One-paragraph project pitch (what the pedal is, why it exists).
- Version, build, firmware-compatibility range — sourced once from a
  central `AppInfo` provider so the splash and Connection details sheet
  use the same string.
- Project links (tappable, open in browser): GitHub repository, project
  website / docs site, release notes / changelog, "Report a bug" →
  GitHub issues.
- License and credits line (full third-party licenses live on the Legal
  page — TASK-335).

## Acceptance Criteria

- [x] Info/About page reachable from the chosen single entry point.
      *(Top-bar `Icons.info_outline` icon on the home screen,
      pushing `/info`. Decision: dedicated icon over overflow menu —
      single content page today, dedicated icon is more discoverable;
      later content pages will move into a unified menu when the
      count justifies it.)*
- [x] Version string sourced from `package_info_plus` (or equivalent),
      not hard-coded — same source the splash and Connection details
      sheet will read. *(`AppInfo.load()` wraps `PackageInfo.fromPlatform`
      with caching + a test-override hook.)*
- [x] All external links open in the system browser via `url_launcher`
      (`launchUrl` with `LaunchMode.externalApplication`).
- [x] Page fits one phone screen without scrolling on a Pixel 9 form
      factor. *(412×915 surface size in widget test passes with no
      overflow exception.)*
- [x] Shared content-page chrome (scaffold + back nav + typography) is
      extracted into a reusable widget the later content tasks can use.
      *(`ContentPageScaffold` in `lib/widgets/content_page_scaffold.dart`.)*

## Implementation notes

- Firmware compatibility line reads "Compatible with firmware of the
  same version." per the unified-version policy from TASK-260. The
  harder cross-version compatibility question is captured in
  [IDEA-054](../../ideas/open/idea-054-cross-version-app-firmware-compatibility-policy.md).
- New deps: `package_info_plus: ^8.0.0`, `url_launcher: ^6.2.5`.
- Widget test at [`app/test/widget/info_about_screen_test.dart`](../../../../app/test/widget/info_about_screen_test.dart)
  covers all four AC verification points (render, version, link
  channel, Pixel 9 fit). All 127 app tests pass; `flutter analyze`
  is clean.
- TASK-349 ("Display unified version in mobile app") becomes a
  verify-only step — `AppInfo` already exposes `versionDisplay`
  rendered on this page.

## Prerequisites

- **TASK-330** — content source-of-truth and i18n decisions; determines
  whether copy lives in Dart strings or a Markdown asset, and whether
  i18n is scaffolded now.

## Test Plan

**Host tests** (Flutter widget tests, `flutter test`):

- Add `app/test/pages/info_about_page_test.dart`.
- Cover: page renders without overflow; version string matches
  `AppInfo.version`; tapping a link invokes `url_launcher`'s
  platform-channel mock with the expected URL.

## Documentation

- `docs/users/` — short pointer that the app has an Info/About page (if
  the user-docs decision in TASK-330 lands on "single source of truth").

## Notes

- Hold the choice of icon (info vs overflow) until this task — once
  picked, all subsequent content-page tasks reuse it.
- Keep copy short and factual per IDEA-037's "no marketing copy" rule.
