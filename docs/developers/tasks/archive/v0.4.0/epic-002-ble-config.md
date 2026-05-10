---
id: EPIC-002
name: BLEConfig
title: BLE Configuration Service
status: open
opened: 2026-04-23
closed:
assigned:
effort_actual: XS (<30m)
effort_actual_source: heuristic-v1
---

Implement the end-to-end BLE configuration pipeline: a GATT service on the
firmware side, a JSON schema for configs and profiles, a Python CLI tool for
profile upload over BLE, and the supporting host and on-device integration
tests. Seeded by IDEA-001 (Mobile App Configuration) and IDEA-002 (CLI Tools).

## Tasks

Tasks belonging to this epic reference it via `epic: BLEConfig` in their
frontmatter.
