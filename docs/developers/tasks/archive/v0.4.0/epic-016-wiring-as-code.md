---
id: EPIC-016
name: wiring-as-code
title: Wiring harness diagrams as code (WireViz)
status: open
opened: 2026-04-23
closed:
assigned:
effort_actual: XS (<30m)
effort_actual_source: heuristic-v1
---

Replace Fritzing breadboard-layout PNGs with WireViz YAML harness diagrams
that CI renders automatically. Covers WireViz/Graphviz toolchain installation,
ESP32 and nRF52840 harness files, SVG and BOM generation, pre-commit hook, CI
staleness guard, BUILD_GUIDE.md embedding, and Fritzing artefact retirement.
Seeded by IDEA-019 (Wiring as Code). Note: this epic was subsequently
superseded for circuit schematics by EPIC-010 (circuit_diagram_as_code /
Schemdraw) after the WireViz approach was retired for that use case.

## Tasks

Tasks belonging to this epic reference it via `epic: wiring-as-code` in their
frontmatter.
