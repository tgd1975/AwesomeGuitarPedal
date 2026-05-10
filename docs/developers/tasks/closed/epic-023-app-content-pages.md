---
id: EPIC-023
name: app-content-pages
title: App content pages and connection visibility
status: open
opened: 2026-04-30
closed:
assigned:
branch: feature/config-extensions
---

Grow the Flutter app from configurator-only into a self-explanatory app:
short Info/About, How-To, Profiles explainer, Troubleshooting, and Legal
pages; a startup splash; persistent connection visibility; a Connected-
Pedal read-only summary page; and a Live-keystroke diagnostic page driven
by a generated HID usage display table.

Seeded by IDEA-037 (App content pages — Info/About, How-To, and
supporting screens).

The epic groups two layers of work that share scope but differ in cost:

1. **Pure client-side pages** (Info/About, How-To, Profiles explainer,
   Troubleshooting, Legal, splash, connection status strip) — ship
   without firmware changes.
2. **Connected-Pedal page** — initial version shippable on board
   identity (`kHwIdentityUuid`) alone; the firmware-version,
   readable-config, and live-active-profile rows are placeholdered
   until the matching firmware surfaces land.

A decision task lands first to resolve the open questions in IDEA-037
(content source of truth, i18n scaffolding, context-aware How-To,
first-run flow) before any UI is built.

The third layer originally tracked here — **firmware / BLE readback
surfaces** (firmware-version char, config readback, active-profile
notify) — was carved out into its own
[EPIC-026 pedal-details-app-pages](epic-026-pedal-details-app-pages.md)
because that work has a different cost profile (firmware diffs +
on-device verification gated on the same nRF52840 deferral) from the
client-side content pages here.

## Tasks

Tasks are listed automatically in the Task Epics section of
`docs/developers/tasks/OVERVIEW.md` and in `EPICS.md` / `KANBAN.md`.
