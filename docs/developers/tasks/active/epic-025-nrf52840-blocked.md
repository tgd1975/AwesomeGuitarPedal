---
id: EPIC-025
name: nrf52840-blocked
title: nRF52840 hardware-blocked work
status: open
opened: 2026-05-01
closed:
assigned:
---

Holding pen for every task that requires the Adafruit Feather nRF52840 hardware target for verification while the device is physically unavailable. Mirrors the [EPIC-019 iphone-app](epic-019-iphone-app.md) pattern: tasks that cannot be on-device verified live here in `paused/` rather than scattered across feature epics where they would block per-epic progress.

While this epic exists, the [`/nrf5-task-routing`](../../../.claude/skills/nrf5-task-routing/SKILL.md) skill funnels every newly-scaffolded nRF52840-touching task into this epic instead of the feature epic that would otherwise own it by scope. Each routed task records its original feature epic in its Notes section so the link back is preserved.

When nRF52840 hardware becomes available again:

1. Activate [TASK-359](task-359-remove-nrf5-task-routing-skill.md) — removes the routing skill, de-registers it from `.vibe/config.toml`, and re-homes every other task in this epic back to its feature epic.
2. This epic closes once it is empty.

## Tasks

Tasks belonging to this epic reference it via `epic: nrf52840-blocked` in their frontmatter.
