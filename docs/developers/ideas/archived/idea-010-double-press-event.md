---
id: IDEA-010
title: Double Press Event
category: firmware
description: Additional functionality triggered by quick successive button presses
status: closed
closed: 2026-04-21
---

## Details

Add support for double press events to enable quick successive button presses for additional functionality.

## Resolution

Fully implemented across the entire stack — hardware detection (300ms configurable window,
single-press suppression), `EventDispatcher`, profile/JSON config, `main.cpp` loop,
Flutter app UI (`DoublePressSlot`), and unit tests. nRF52840 remains a stub.
