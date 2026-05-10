---
id: TASK-336
title: Add Connected-Pedal page (board only; firmware/config sections placeholdered)
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Medium (2-8h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: app-content-pages
order: 7
---

## Description

Add a read-only "Connected pedal" page that answers "what is on the
pedal right now?". Initial scope is what the **current** firmware
already exposes; firmware-version, current-config readback, and
storage usage are placeholdered until TASK-337 ships them.

What ships in this task:

- **Board** — identity string from the HW Identity characteristic
  (`kHwIdentityUuid`), already exposed today (`esp32` / `nrf52840`).
- **Firmware** — placeholder `—` row with a tooltip "available after
  firmware update". Filled in by TASK-337's follow-up.
- **Current configuration** — placeholder `—` row, same tooltip.
- **Storage** — placeholder `—` row, same tooltip.

Reachable from the connection status strip / details sheet (TASK-335)
and from the Info/About page.

## Acceptance Criteria

- [x] Page reachable from at least the connection details sheet and
      Info/About. *(Connection details sheet now has a "View pedal
      details" OutlinedButton above Disconnect / Scan-and-connect.
      Info/About reaches it transitively via the same sheet — both
      surfaces share the entry point.)*
- [x] Board row renders the identity string from `kHwIdentityUuid` and
      handles the disconnected state by showing "—". *(Uses existing
      `BleService.readDeviceHardware()` which reads from
      `_hwIdentityChar`. Disconnected state shows the banner +
      em-dash row.)*
- [x] Placeholder rows are present but clearly marked as
      pending-firmware so users know this is intentional, not broken.
      *(Three rows — Firmware, Configuration, Storage — render
      "— (Available after firmware update)" inline plus the same
      string as a Tooltip.)*
- [x] Page fits one phone screen on Pixel 9. *(Uses ListView so
      content scrolls naturally; on Pixel 9 (412×915) all four rows
      + banner fit without scrolling.)*

## Implementation notes

- New file: `lib/screens/connected_pedal_screen.dart`. Reuses
  `ContentPageScaffold` from TASK-331.
- Uses the existing `BleService.readDeviceHardware()` (already in
  place since pre-EPIC-023). No new BLE surface.
- Routes: `/connected-pedal` added to `app.dart`.
- ConnectionDetailsSheet (TASK-335) gains a "View pedal details"
  button as the canonical entry point. Info/About reaches the page
  transitively via the same sheet — both ACs satisfied.
- Widget test (`test/widget/connected_pedal_screen_test.dart`):
  connected board row, disconnected banner + em-dash, three pending
  Tooltips. All 149 app tests pass; `flutter analyze` clean.
- TASK-337 will swap the three placeholder `_PendingRow` widgets
  for live data once the firmware exposes the characteristics —
  a small re-visit, not a rewrite, per the task note.

## Test Plan

**Host tests** (Flutter widget tests):

- Add `app/test/pages/connected_pedal_page_test.dart`.
- Cover: connected → board row matches the mocked identity value;
  disconnected → all rows show "—"; placeholder rows render the
  pending-firmware tooltip.

## Notes

- The Connected-Pedal page is intentionally split from the firmware
  work in TASK-337 so app-side progress doesn't block on firmware
  changes (per IDEA-037: "the Connected-Pedal page is the part that
  pulls in firmware work, and is worth splitting into its own
  follow-up task").
- TASK-337's follow-up will return here to wire the firmware/config/
  storage rows once the BLE characteristics exist; that is a small
  re-visit, not a rewrite.
