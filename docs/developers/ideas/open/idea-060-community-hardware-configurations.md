---
id: IDEA-060
title: Add debounce time to hardware config — and make hardware configs community-shareable
description: Add per-build debounce time to the existing hardware config, then mirror the community-profiles flow so builders can share known-good hardware configs.
category: 📱 apps
---

## Motivation

We already have a hardware config (`data/config.json`, schema in
`data/config.schema.json`, model in
[`app/lib/models/hardware_config.dart`](../../../../app/lib/models/hardware_config.dart))
that captures pin assignments, button/LED counts, and the BLE pairing
pin. We also already have a community-profiles flow (service in
[`app/lib/services/community_profiles_service.dart`](../../../../app/lib/services/community_profiles_service.dart),
screen in
[`app/lib/screens/community_profiles_screen.dart`](../../../../app/lib/screens/community_profiles_screen.dart),
contribution guide in
[`profiles/CONTRIBUTING.md`](../../../../profiles/CONTRIBUTING.md)).

Two gaps remain:

1. **Debounce time is hard-coded** at 100 ms in
   [`src/esp32/include/button.h:29`](../../../../src/esp32/include/button.h)
   and
   [`src/nrf52840/include/button.h:23`](../../../../src/nrf52840/include/button.h).
   It is a property of the physical switch, not the firmware — different
   builds with different switch models need different values, and there
   is no way to set it without recompiling.
2. **Hardware configs are not shareable.** Two builders with the same
   parts list have to re-derive the same `config.json` independently.
   Profiles get the community treatment; hardware configs do not.

## Rough idea

### Part 1 — debounce time in the hardware config

Add a `debounceMs` field (per input or as a single value, TBD) to the
hardware-config schema and the `HardwareConfig` model. Plumb it through
to `Button` so `debounceDelay` is read from config at boot instead of
the hard-coded 100 ms. Default to 100 ms when the field is absent so
existing configs still work.

### Part 2 — community hardware configs

Mirror the community-profiles flow for hardware configs:

- A `hardware-configs/` (or similar) directory in the same docs-site
  publishing pipeline.
- An index file the app can fetch.
- An app screen analogous to `community_profiles_screen.dart` for
  browsing and importing.
- A `CONTRIBUTING.md` analogous to
  [`profiles/CONTRIBUTING.md`](../../../../profiles/CONTRIBUTING.md).

A builder publishes their working `config.json` (now including
`debounceMs`) tagged with the parts list / build it matches. Other
builders reproducing the same build can drop it in.

Schema validation already exists and just needs to be updated for the
new field — no design work there. Profile ↔ hardware-config pairing is
explicitly out of scope; the two stay independent in the UI as today.

## Open questions

- **Granularity of `debounceMs`** — single project-wide value, per-button
  override, or per-input-type? The simplest first step is one value;
  per-button can come later if real builds need it.
- **Should community hardware configs and community profiles share a
  repo / index, or stay separate?** They are different kinds of
  artefact; keeping them parallel-but-separate is probably cleaner, but
  sharing infra (index format, fetch service) is worth checking.
