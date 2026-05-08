---
id: EPIC-018
name: circuit-skill-diagrams
title: Circuit-skill documentation diagrams
status: open
opened: 2026-04-25
closed:
assigned:
effort_actual: Medium (2-8h)
effort_actual_source: heuristic-v1
---

Add diagrams to the IDEA-027 design documents — pipeline, dataflow, decision flows,
static structure, contributor workflow, and the slot vocabulary's spatial layout —
so the dense prose specifications become readable at a glance.

The diagram set was scoped against a per-doc analysis: each candidate was rated on
comprehension uplift vs. effort, and only the ones where a picture replaces multiple
paragraphs of prose were kept. Tier-3 candidates (whose prose is already clear) and
skipped candidates are deliberately left out.

Tasks are grouped by drawing medium and topic so each is one coherent sitting:
Mermaid Tier-1 dataflow diagrams together, Mermaid structure diagrams together,
the hand-authored SVG on its own (different medium), the lone sequence diagram on
its own (different shape), and Mermaid decision flows together.

## Tasks

Tasks belonging to this epic reference it via `epic: circuit-skill-diagrams` in
their frontmatter.
