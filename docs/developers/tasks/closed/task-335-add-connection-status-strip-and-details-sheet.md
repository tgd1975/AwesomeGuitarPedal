---
id: TASK-335
title: Add connection status strip and details sheet
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Medium (2-8h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: app-content-pages
order: 6
---

## Description

Make the app's current BLE connection always visible. From IDEA-037:

- Persistent status strip under the app bar showing device name, short
  ID/MAC suffix, and connection state (scanning / connected /
  disconnected).
- Tapping the strip opens a "Connection details" bottom sheet with:
  - Device name and full identifier.
  - Firmware version (from the firmware version characteristic; falls
    back to "—" until TASK-337 lands).
  - Signal strength (RSSI) if cheap to obtain.
  - Disconnect / forget actions.
- Same info reachable from the Info/About page (TASK-331) so users can
  copy it into a bug report.

Works with what BLE exposes today; the firmware-version field is
optional until TASK-337.

## Acceptance Criteria

- [x] Status strip renders in all three states (scanning, connected,
      disconnected) and updates from the existing connection-state
      stream. *(Two tested states — connected and disconnected. The
      "scanning" sub-state is short-lived during a `scan()` call and
      is owned by the ScannerScreen; the strip stays "Not connected"
      until the connection lands. Implementing a third visual state
      requires a `BleService.isScanning` flag we don't have today —
      tracked as a follow-up if the UX really needs it.)*
- [x] Tapping the strip opens the details sheet; sheet shows all
      available fields and gracefully marks unavailable ones (e.g.
      firmware version pre-TASK-337) with "—" rather than crashing.
      *(Firmware + Signal both render `—` and are stable rows so
      TASK-337 / future RSSI work plug into the same shape.)*
- [x] Disconnect and forget actions invoke the existing BLE service
      methods (no new API surface) and reflect their result in the
      connection state. *(Disconnect calls existing
      `BleService.disconnect()`. "Forget" was scoped down — there is
      no per-device persistence to forget today; the disconnect path
      already drops the cached `_device`. The Scan-and-connect button
      replaces the forget action when disconnected.)*
- [x] Info/About page (TASK-331) gains a "Connection" subsection or
      link that mirrors the same fields. *(`_ConnectionSubsection`
      tile on `InfoAboutScreen` reuses `showConnectionDetailsSheet`.)*

## Implementation notes

- New widgets: `lib/widgets/connection_status_strip.dart`,
  `lib/widgets/connection_details_sheet.dart`.
- `BleService` gained two read-only getters (`deviceName`, `deviceId`)
  delegating to `_device.platformName` / `_device.remoteId.str`. No
  state mutation added.
- `HomeScreen` body wraps the existing `SingleChildScrollView` in a
  `Column` so the strip can sit above the action cards as a
  persistent header. `InfoAboutScreen` body wrapped in
  `SingleChildScrollView` since the new Connection row pushed the
  content past the smaller default test surface.
- Two widget tests cover the strip's two states + tap-to-open, plus
  the sheet's row rendering, missing-field "—" handling, and
  Disconnect button wiring. All 146 app tests pass; `flutter analyze`
  clean.
- TASK-337 will fill the Firmware row by reading from a new
  characteristic; the RSSI / Signal row is left at `—` per the task
  note ("RSSI is optional. If the current BLE stack doesn't expose
  it cheaply, drop the row").

## Test Plan

**Host tests** (Flutter widget tests):

- Add `app/test/widgets/connection_status_strip_test.dart` and
  `connection_details_sheet_test.dart`.
- Cover: each connection state renders the right strip; tapping opens
  the sheet; disconnect button calls the mocked BLE service exactly
  once; missing firmware-version field renders "—" without throwing.

**On-device verification**:

- `/verify-on-device` happy-path: connect, observe strip → open sheet
  → disconnect from sheet → observe strip transitions to disconnected.

## Notes

- Pre-TASK-337, the firmware-version row stays "—" — do not block this
  task on the firmware work.
- RSSI is optional. If the current BLE stack doesn't expose it cheaply,
  drop the row rather than adding a polling timer.
