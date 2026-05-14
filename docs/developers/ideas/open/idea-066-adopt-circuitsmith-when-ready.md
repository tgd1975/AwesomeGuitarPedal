---
id: IDEA-066
title: Adopt gdt1975/circuitsmith for circuit-design tooling when ready
category: 🛠️ tooling
description: Watch upstream circuitsmith, evaluate when it stabilises, and adopt it as the source of YAML circuits / SVG / ERC / BOM / netlist for ASP — replacing the locally-planned Circuit-Skill (archived IDEA-027).
status: open
related: IDEA-011, IDEA-018, IDEA-019, IDEA-022, IDEA-027
---

## Motivation

[IDEA-027](../archived/idea-027-circuit-skill.md) proposed building a comprehensive
AI-assisted circuit-design skill in-tree: YAML → schematic, ERC, rule catalog, layout
engine, BOM, netlist, Markdown integration, eventually a standalone repo. That is a
substantial body of work to maintain alongside the pedal itself.

[`gdt1975/circuitsmith`](https://github.com/gdt1975/circuitsmith) is an independent
project covering the same problem space. Rather than duplicate the effort, AwesomeStudioPedal
should adopt circuitsmith once it stabilises — the project keeps its focus on the pedal,
and circuit-design tooling improvements flow in from upstream.

## What this idea is *not*

- **Not "use circuitsmith right now"** — at the time of writing it is too early-stage to
  bet the existing schemdraw pipeline on. This idea exists to track readiness, not to
  trigger immediate adoption.
- **Not a re-opening of IDEA-027 in disguise** — if circuitsmith never matures, the
  fallback is to keep the current `scripts/generate-schematic.py` pipeline (from IDEA-019),
  not to revive IDEA-027.

## Adoption signal — when is circuitsmith "ready"?

Open question. Candidate criteria, derived from IDEA-027's capability list as the
evaluation rubric:

- **Declarative YAML input** — equivalent expressiveness to IDEA-027's `.circuit.yml`
  (components, pin connections, buses).
- **Deterministic SVG output** — readable diff on circuit changes; suitable to commit
  alongside the source YAML.
- **ERC** — at minimum the structural checks (unconnected nets, undeclared pins) and
  electrical sanity (missing LED resistors, floating inputs, I2C pull-ups).
- **BOM export** — Markdown or CSV, suitable to embed in `docs/builders/`.
- **Netlist export** — KiCad-compatible `.net` to seed [IDEA-011](idea-011-pcb-board-design.md).
- **Stable enough to pin** — released versions / tags, or a clearly trackable main branch.
- **License compatibility** — usable in this project's MIT-licensed context.

If any of these are missing, document the gap rather than block adoption — some
capabilities (e.g. netlist) only become load-bearing when downstream ideas activate.

## Integration shape (sketch — not a plan)

When adoption fires:

1. Replace `scripts/generate-schematic.py` with a thin wrapper that drives circuitsmith
   against `data/*.circuit.yml` files (translated from `data/config.json`).
2. Wire circuitsmith into the existing pre-commit hook and CI staleness guard
   (the slots IDEA-019 already established).
3. Decide submodule vs pinned copy vs published package — same trade-off as
   archived IDEA-027 Phase 7.
4. Retire `docs/builders/wiring/<target>/main-circuit.svg` regeneration if circuitsmith
   produces equivalent output to the same paths.

## Open questions

- Does circuitsmith plan an extensible component library, or is the catalogue fixed?
  (Custom MCU/connector profiles are a hard requirement for this project.)
- What is circuitsmith's release cadence and stability policy?
- Is there an upstream contribution path for ASP-specific needs (e.g. nRF52840 profile,
  pedal-specific ERC rules), or should they live downstream?
- How does circuitsmith handle the Markdown-block embedding case (IDEA-022 / Phase 5 of
  IDEA-027)?

## Action — until adoption fires

- No code change. Existing `scripts/generate-schematic.py` (IDEA-019) keeps producing
  the committed SVGs.
- Periodic check-in: review circuitsmith readme / release notes when working in
  `scripts/generate-schematic.py` or the `docs/builders/wiring/` area.
- When the adoption signal looks credible, open a follow-up task to evaluate against
  the rubric above and propose an integration PR.
