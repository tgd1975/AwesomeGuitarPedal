---
id: EPIC-001
name: task-system
title: Task and idea management system
status: open
opened: 2026-04-23
closed:
assigned:
effort_actual: XS (<30m)
effort_actual_source: heuristic-v1-low-confidence
---

Realize the docs-as-code task and idea management system designed in
[IDEA-021](../../ideas/open/idea-021-task_system_concept_for_small_teams.md):
file-based tasks and ideas, status-from-folder conventions, a central
`housekeep.py` engine that moves files to match their `status:`
field and regenerates overview documents, lifecycle skills
(`/ts-task-active`, `/ts-task-pause`, `/ts-task-reopen`, `/ts-idea-new`,
`/ts-idea-archive`, `/ts-epic-new`), and generated dashboards
(`OVERVIEW.md`, `EPICS.md`, `KANBAN.md`).

## Tasks

Tasks belonging to this epic reference it via `epic: task-system` in
their frontmatter. They are listed automatically under the
`### task-system` section of `docs/developers/tasks/OVERVIEW.md`.
