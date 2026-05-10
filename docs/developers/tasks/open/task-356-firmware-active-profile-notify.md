---
id: TASK-356
title: Firmware — active-profile-index notify characteristic
status: open
opened: 2026-05-01
effort: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: pedal-details-app-pages
order: 3
prerequisites: [TASK-353]
decision_doc: docs/developers/BLE_READBACK_IMPACT.md
---

## Description

Add a small read+notify BLE characteristic that tracks the active
profile index, so the app can reflect SELECT-button profile
switches in real time without re-reading the whole config.

**Platform scope: ESP32 only.** nRF52840 has no custom GATT
service today and `Nrf52840PedalApp::saveProfile()` is a no-op
(profile index doesn't persist across reboot — always boots to 0),
so this characteristic is deferred on nRF52840 along with TASK-355.
Full rationale in
[BLE_READBACK_IMPACT.md §3.3](../../BLE_READBACK_IMPACT.md#33-active-profile-index-notify-characteristic--task-356).

What ships (ESP32):

- Read+notify char at UUID `516515c6-4b50-447b-8ca3-cbfce3f4d9f8`
  in `BleConfigService` with a single-byte payload (active profile
  index, 0..N-1).
- Notification fires within one BLE connection-interval of the
  profile actually changing in firmware. The hook lives in
  `ProfileManager::switchProfile` / `setCurrentProfile`.

App-side:

- Subscribe to the notify on connect; expose
  `BleService.activeProfileStream` so the configurator and the
  Connected-Pedal page can observe live profile state.

## Acceptance Criteria

- [ ] Read+notify characteristic exists on **ESP32** at UUID
      `516515c6-…`. nRF52840 deferred (see Description).
- [ ] Notification fires within one BLE connection-interval of a
      profile switch (verified via on-device test + observed
      timestamp delta).
- [ ] App `BleService` exposes a stream of active-profile indices
      that the rest of the app can `context.watch` against, with a
      no-op fallback on nRF52840-connected pedals (the platform
      lacks the characteristic).
- [ ] No new consumer UI is required for this task to land — it's
      infrastructure for whichever surface needs it next. The
      Connected-Pedal page's "current profile" row may pick this
      up in a follow-up; not in scope here.

## Prerequisites

- **TASK-353** — feasibility analysis covers the per-platform
  notify mechanics and the firmware event-boundary location.

## Test Plan

**On-device tests** — extend the existing BLE test on each target
to subscribe to the notify, trigger a profile switch (via the
test rig's button-press injection or whatever the existing
profile-switch test uses), and assert the notification arrives
with the correct payload within the one-connection-interval
budget.

**Host tests** — the notification-emission logic in firmware is
mostly hardware-bound; don't force a host-side test here.

**App-side unit test** — extend `BleService` tests to verify
`activeProfileStream` re-emits on subsequent notify events
without leaking subscriptions.

## Notes

- This was carved out of the original TASK-337 — independent of
  the version-char and config-readback work, low risk, but with
  no immediate UI consumer. Sized Small because the firmware
  side is a small char + a one-line emit at the existing
  profile-switch site, and the app side is a stream subscription.
- Could legitimately defer until a UI consumer exists. Decided at
  TASK-353 time whether to ship now or park.
- Hardware: ESP32 connected via USB. nRF52840 notify char
  deferred to TASK-358 (no device available; see
  [BLE_READBACK_IMPACT.md](../../BLE_READBACK_IMPACT.md)).
