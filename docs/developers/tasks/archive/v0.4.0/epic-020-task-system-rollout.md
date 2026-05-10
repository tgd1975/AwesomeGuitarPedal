---
id: EPIC-020
name: task-system-rollout
title: Coordinated task-system rollout — single source of truth, paused, effort actuals, burn-up
status: open
opened: 2026-04-26
closed:
assigned:
effort_actual: XS (<30m)
effort_actual_source: heuristic-v1-low-confidence
---

Sequenced rollout for the burn-up dashboard, post-hoc effort reassessment,
paused-as-first-class status, and the `awesome-task-system/` source-of-truth
carve-out — landed in dependency order so each task's data prerequisites are
in place when it lands.

Seeded by IDEA-043 (Coordinated task-system rollout — burn-up dashboard,
post-hoc effort, paused status, and awesome-task-system carve-out). The
parent idea consolidates three sub-pages (release burn-up chart,
LLM effort reassessment on close, paused as first-class task status) and
adds the `awesome-task-system/` source-of-truth prerequisite that surfaced
from reviewing them together.

## Execution order

1. Single source of truth for `awesome-task-system/` (prerequisite for everything else).
2. Paused as first-class task status (depends on 1).
3. `effort_actual` on close (depends on 1; sequenced after 2 to settle the canonical effort vocabulary first).
4. Release burn-up chart (depends on 1, 2, 3 — both data sources in place).

## Tasks

Tasks belonging to this epic reference it via `epic: task-system-rollout` in
their frontmatter.
