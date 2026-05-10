---
id: EPIC-020
name: config-driven-runtime-customisation
title: Config-driven runtime customisation
status: open
opened: 2026-04-29
closed:
assigned:
---

Extend `config.json` and `profiles.json` so users can shape pedal behaviour
without recompiling firmware: a top-level `independentActions` block that
fires alongside whatever profile is active.

Seeded by IDEA-024 (Profile-Independent Actions) and IDEA-025 (Configurable
BLE Device Name).

The IDEA-025 half (TASK-310, TASK-311, TASK-312) was abandoned on
2026-04-29 after investigation: the BLE adapter on ESP32 is constructed
during static initialization, before LittleFS is mounted, so a
`config.json`-driven device name would require restructuring
`BlePedalApp`'s constructor contract — not worth the cost for a cosmetic
feature. See archived IDEA-025 for the full postmortem. The advertised
BLE name stays hard-coded in the platform adapters.

## Tasks

Tasks belonging to this epic reference it via
`epic: config-driven-runtime-customisation` in their frontmatter.
