---
id: EPIC-015
name: hardware-selector
title: Hardware-aware configuration
status: open
opened: 2026-04-23
closed:
assigned:
effort_actual: XS (<30m)
effort_actual_source: heuristic-v1
---

Add a hardware field to config.json and propagate it through the full stack:
config schema, firmware boot-time mismatch rejection, CLI upload guard, Flutter
app hardware-aware editing and upload guard, and configuration builder hardware
selector with per-board defaults and pinout diagram. Aligns with the
multi-hardware support direction of IDEA-003 (Additional Hardware Support).

## Tasks

Tasks belonging to this epic reference it via `epic: hardware-selector` in
their frontmatter.
