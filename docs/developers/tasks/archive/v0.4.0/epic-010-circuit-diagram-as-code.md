---
id: EPIC-010
name: circuit_diagram_as_code
title: Circuit diagrams as code (Schemdraw)
status: open
opened: 2026-04-23
closed:
assigned:
effort_actual: XS (<30m)
effort_actual_source: heuristic-v1
---

Replace the Fritzing-exported schematic PNGs with Python-generated SVGs using
Schemdraw. Covers toolchain installation, ESP32 and nRF52840 schematic scripts,
pre-commit hook, CI staleness guard, builder documentation update, and WireViz
artefact removal. Part of the broader wiring-as-code initiative seeded by
IDEA-019 (Wiring as Code); this epic covers the circuit schematic half after
the WireViz harness-diagram approach was retired.

## Tasks

Tasks belonging to this epic reference it via `epic: circuit_diagram_as_code`
in their frontmatter.
