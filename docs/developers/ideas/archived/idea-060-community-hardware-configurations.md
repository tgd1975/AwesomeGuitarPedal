---
id: IDEA-060
title: Add debounce time to the hardware config
description: Hard-coded 100 ms debounce is a property of the physical switch, not the firmware. Move it into the hardware config so different builds can set the right value without recompiling.
category: ⚡ firmware
---

## Archive Reason

2026-05-11 — Promoted to EPIC-028 (configurable-debounce) with TASK-375 and TASK-376.

## Motivation

We already have a hardware config (`data/config.json`, schema in
`data/config.schema.json`, model in
[`app/lib/models/hardware_config.dart`](../../../../app/lib/models/hardware_config.dart))
that captures pin assignments, button/LED counts, and the BLE pairing
pin. **Debounce time is the obvious omission** — it is hard-coded at
100 ms in
[`src/esp32/include/button.h:29`](../../../../src/esp32/include/button.h)
and
[`src/nrf52840/include/button.h:23`](../../../../src/nrf52840/include/button.h).
It is a property of the physical switch, not the firmware — different
builds with different switch models need different values, and there
is no way to set it without recompiling.

## Rough idea

Add a **single project-wide `debounceMs`** field to the hardware-config
schema and the `HardwareConfig` model. Plumb it through to `Button` so
`debounceDelay` is read from config at boot instead of the hard-coded
100 ms. Default to 100 ms when the field is absent so existing configs
still work.

Granularity: one value, KISS. Per-button or per-input-type overrides
can come later if a real build needs them — until then, the schema
stays flat.

## Follow-up

Making the config itself **community-shareable** (mirroring the
community-profiles flow) is tracked separately in IDEA-063, which
depends on this idea landing first so shared configs are complete
descriptions of a build.
