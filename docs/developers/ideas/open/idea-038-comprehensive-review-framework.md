---
id: IDEA-038
title: Comprehensive code & architecture review framework — define the dimensions
category: docs
description: Iteratively define what "thorough review" means for this project — clarity, testability, documentation, usefulness, no dead-ends, extendability — into an actionable rubric.
---

## Motivation

A "thorough code and architecture review" is easy to ask for and hard to
execute consistently. Reviewers (human or AI) drift toward whatever
catches their eye first — style nits, obvious bugs, the part of the
code they happen to know best — and miss the structural questions that
actually matter for a small, long-lived hobby/maker codebase like this
one.

The goal of this idea is **not** to immediately build a review tool or
checklist. The goal is to first answer the prerequisite question:

> What does "all those things" actually mean here?

Once we have a shared, written-down set of dimensions and what each one
means in the context of *this* project (firmware on ESP32/nRF52840 +
Flutter app + docs-as-code), we can decide later how to operationalize
it (skill, checklist, CI check, periodic audit, …).

## First step — brainstorm the dimensions

Initial seed list, copied from the prompt and lightly grouped. This is
**not** the final list — it is the starting point for refinement.

### Code-level

- **Clarity** — can a new contributor read a file top-to-bottom and
  understand intent without a guided tour?
- **Naming** — do identifiers carry their weight, or do they require
  comments to explain what they mean?
- **Local reasoning** — can you understand a function without paging
  in five other files?
- **Testability** — is the code shaped so that the interesting logic
  can be exercised by host tests (no hardware, no shims of shims)?
- **Existing test coverage** — for the parts that matter, do tests
  actually exist and assert behavior (not just "does not crash")?
- **Dead code / dead ends** — unused functions, abandoned branches,
  half-finished abstractions, TODOs older than 6 months.

### Architecture-level

- **Separation of concerns** — does each module have one job, or are
  there hidden cross-couplings (e.g. BLE talking directly to GPIO)?
- **Platform abstraction** — `#ifdef` discipline, HAL boundaries
  (cross-ref IDEA-028).
- **Extendability where it matters** — adding a new button event,
  a new hardware target, or a new action type should be a localized
  change, not a shotgun edit. *Not* every axis needs to be extensible —
  premature flexibility is its own debt.
- **Data flow** — config → runtime state → events → side effects.
  Is the path traceable end-to-end?
- **Failure modes** — what happens on bad config, BLE drop, flash
  corruption, low battery? Is the answer documented or accidental?

### Documentation & discoverability

- **Public-facing docs** — is the BUILD_GUIDE / README accurate vs.
  the current state of the code?
- **Developer docs** — is the architecture documented anywhere, or
  do you have to read the code to learn the shape?
- **Inline docs** — function/class headers where the *why* is
  non-obvious (project policy: no comments for *what*).
- **Cross-links** — do related concepts link to each other (epics,
  tasks, ideas, code, docs)?

### Usefulness / product fit

- **Does it serve a real persona?** — every feature should map back
  to a stakeholder need (cross-ref IDEA-020 / SysML).
- **No dead-end features** — features built once and never touched
  again, with no users, no tests, no docs.
- **Cost vs. value** — does the maintenance burden of a feature
  justify the value it delivers?

### Process / meta

- **Tasks / ideas hygiene** — are open tasks still relevant? Are
  closed tasks actually closed in code, or just in docs?
- **CHANGELOG fidelity** — does the changelog reflect what shipped?
- **CI signal** — do failing checks mean something, or are they
  routinely ignored?

## Open questions (for next iteration)

1. Is this one rubric, or several rubrics (firmware vs. app vs. docs
   vs. process)?
2. What's the unit of review — a PR, a module, the whole repo, an
   epic at close-out?
3. Should each dimension have a 1–5 score, a binary pass/fail, or
   just open-ended notes?
4. Who/what runs the review — Claude on demand, a scheduled agent
   (cross-ref `/schedule`), or a human checklist before tagging a
   release?
5. What does the *output* look like — a Markdown report committed to
   `docs/developers/reviews/`, an issue per finding, or just
   conversation?
6. How do we avoid the rubric becoming bureaucratic theater that
   nobody actually uses?
7. Which dimensions overlap with existing skills (`/review`,
   `/security-review`, `/simplify`, `/clang-tidy`, `/lint`,
   `/doc-check`) and which are genuinely new?
8. "Extendability where it matters" — how do we identify *where it
   matters* without falling into speculative future-proofing?

## Related ideas

- IDEA-020 — MBSE / SysML v2 (stakeholder needs ↔ usefulness)
- IDEA-028 — HAL refactor (architecture / platform abstraction)
- IDEA-034 — Test regime reevaluation (testability / coverage)
- IDEA-021 — Task system concept (process hygiene)

## Status

**Exploratory.** Will be refined over multiple sessions before any
operationalization is proposed.
