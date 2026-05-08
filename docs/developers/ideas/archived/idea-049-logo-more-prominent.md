---
id: IDEA-049
title: Make the logo more prominent across apps and tools
category: apps
description: Use the project logo more consistently — especially in the simulator and the config builders, which currently have none.
---

## Archive Reason

2026-05-01 — Shipped via EPIC-024 logo-branding (TASK-339, TASK-340,
TASK-341, TASK-342, TASK-343, TASK-345). All concrete rollout decisions
identified in this idea are now in production.

The project logo is underused. Some surfaces show it, others don't, and the
overall feel is inconsistent. The simulator and the config builders in
particular have no logo at all, which makes them feel detached from the rest
of the project.

## Motivation

- Stronger visual identity across the touchpoints a builder/musician sees.
- Helps users recognise that the simulator, config builders, mobile app, and
  docs all belong to the same project.
- A small polish item with a disproportionate effect on perceived
  cohesiveness.

## Rough scope

Surfaces to audit and add the logo to where missing or under-displayed:

- Web simulator (currently no logo).
- Web config builders — both the action-editor / profile builder variants
  (currently no logo).
- Mobile app (verify it's prominent enough on the main screen and About/Info).
- Docs site (header / favicon consistency).
- README and GitHub social preview (sanity check).

## Open questions

- Where exactly should it sit in each tool — header, footer, splash, or all?
- Should it link back to the project landing page from each surface?
- Do we want a single canonical SVG asset all surfaces import, or
  per-surface copies? (One source of truth is probably right.)
- Is there a "small/icon" variant vs. a "wordmark" variant, and which goes
  where?

## Not in scope

- Redesigning the logo itself.
- Building a brand guide. (Could become a follow-up if this surfaces enough
  inconsistencies.)
