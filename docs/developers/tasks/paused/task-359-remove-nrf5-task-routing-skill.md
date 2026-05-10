---
id: TASK-359
title: Remove nrf5-task-routing skill once nRF52840 hardware is available
status: paused
opened: 2026-05-01
effort: XS (<30m)
complexity: Junior
human-in-loop: Clarification
epic: nrf52840-blocked
order: 1
---

## Description

Cleanup task that activates when the Adafruit Feather nRF52840 hardware becomes physically available again. Removes the [`/nrf5-task-routing`](../../../.claude/skills/nrf5-task-routing/SKILL.md) skill that currently funnels every nRF52840-touching task into [EPIC-025 nrf52840-blocked](epic-025-nrf52840-blocked.md), de-registers it from `.vibe/config.toml`, and re-homes every other task in EPIC-025 back to its original feature epic.

This task gates everything else in EPIC-025 — the routing skill must come down before queued nRF5 tasks (e.g. [TASK-358](task-358-nrf52840-ble-readback-surfaces.md)) can be re-homed and activated normally.

## Acceptance Criteria

- [ ] Adafruit Feather nRF52840 confirmed physically available — one device on the bench, USB-attached, recognised by `pio device list` and (if BLE-relevant) paired per `/ble-reset`.
- [ ] Skill directory removed from disk: `git rm -r .claude/skills/nrf5-task-routing/`.
- [ ] Skill de-registered from [.vibe/config.toml](../../../.vibe/config.toml) — `nrf5-task-routing` removed from the `enabled_skills` list.
- [ ] Every task in EPIC-025 (other than this one) is re-homed to its original feature epic. The routing skill records the original epic in each task's Notes section, so the re-homing is mechanical: read the Notes, set `epic: <feature-epic>`, pick a new `order:` value (max + 1 within the feature epic), un-pause via `/ts-task-active TASK-NNN` if appropriate.
- [ ] EPIC-025 is closed once empty (only this task remains and closes on this commit).

## Test Plan

No automated tests — change is configuration only. After removal, scaffolding a new nRF5-touching task via `/ts-task-new` should pick the feature epic by scope, not `nrf52840-blocked`.

## Paused

- 2026-05-01: Paused — gated on the Adafruit Feather nRF52840 device becoming physically available. There is no task-system prerequisite for this; resume with `/ts-task-active TASK-359` the moment the device is back on the bench. Until then this task sits in `paused/` as the gate for the rest of EPIC-025.

## Notes

- This task only has reason to exist while the routing skill is active. If the skill is removed by other means (e.g. as part of a larger task-system overhaul), close this task as superseded.
- The "re-home every task" AC means: walk every `task-*.md` file with `epic: nrf52840-blocked`, find the original feature epic in its Notes section, update the frontmatter, run `/housekeep`. Don't attempt to recover the original `order:` — assign fresh ones at the end of each target epic's queue.
