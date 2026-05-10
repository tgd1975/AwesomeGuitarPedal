---
id: IDEA-032
title: Extract the circuits-as-docs system into a standalone public GitHub repository
category: tooling
description: Lift the Schemdraw-based schematic generator, pre-commit staleness hook, and docs-as-code wiring pattern into a standalone public repo so other hardware projects can document their circuits the same way.
related: IDEA-019, IDEA-027
---

## Motivation

The circuits-as-docs setup built here — `scripts/generate-schematic.py` driven by board
definitions, SVG outputs committed under `docs/builders/wiring/<target>/`, a pre-commit
hook that regenerates on YAML/config changes (TASK-203), and a CI staleness guard
(TASK-204) — turned out to be surprisingly lightweight and pleasant to work with. IDEA-019
is marked implemented; IDEA-027 ("Circuit-Skill") explores where this could go next
(ERC, BOM, netlist export, AI-assisted authoring).

Other hardware hobbyists and small teams probably have the same need: commit a source
of truth, get a readable SVG, fail CI if they drift. The pattern is generic — nothing
in it is pedal-specific — and a standalone repo would make it discoverable.

## What this would involve (rough, not committed)

- A new public repo — working name `awesome-circuit-docs` (open to alternatives).
- Extract the Schemdraw generator, the board-definition format, the pre-commit hook,
  and the CI staleness guard.
- Keep AwesomeStudioPedal-specific board definitions (ESP32 NodeMCU, nRF52840 Feather)
  as *example* inputs, not core to the tool.
- A README explaining the docs-as-code philosophy: source YAML/Python in, SVG out,
  committed, CI-enforced.
- Probably also: a minimal BUILD_GUIDE-style example showing how to embed the generated
  SVG with a "generated — do not edit" banner.
- Decide whether this repo carries the IDEA-027 extensions (ERC, BOM, netlist) or stays
  minimal — the minimal version is what's proven; the extensions are speculation.

## Open questions (for later)

- Should the board-definition format be YAML (builder-friendly, but needs a parser) or
  Python (what exists today, but higher barrier)? Current code is Python-native.
- Naming — "awesome-circuit-docs" vs. "schemdraw-docs-as-code" vs. something else.
- Does this overlap with existing tools (e.g. KiCad's own SVG export, circuit-diagram,
  WireViz — which we migrated *away* from in TASK-206)? Needs a brief landscape check
  before committing to publishing.
- Relationship to IDEA-027 — is that a follow-up in the extracted repo, or does
  IDEA-027 stay here as an AwesomeStudioPedal-internal exploration?

## Why this is an idea, not a task

Same reasoning as IDEA-031: the scope depends on things only visible later — how stable
the generator feels under more usage, whether the audience exists, whether the minimal
core is enough or whether IDEA-027's extensions need to land first to be interesting.
Convert to a task when the picture is clearer.
