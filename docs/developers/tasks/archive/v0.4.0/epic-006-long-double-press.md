---
id: EPIC-006
name: LongDoublePress
title: Long-press and double-press detection
status: open
opened: 2026-04-23
closed:
assigned:
effort_actual: XS (<30m)
effort_actual_source: heuristic-v1
---

Implement long-press and double-press event detection across the full stack:
firmware detection logic, EventDispatcher multi-event API, config schema
extension, main.cpp wiring, simulator support, profile configurator fields,
Flutter UI slots, and host tests. One on-device integration test for the ESP32
(TASK-109) remains open. Seeded by IDEA-009 (Long Press Event) and IDEA-010
(Double Press Event).

## Tasks

Tasks belonging to this epic reference it via `epic: LongDoublePress` in their
frontmatter.
