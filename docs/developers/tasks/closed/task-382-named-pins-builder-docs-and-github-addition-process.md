---
id: TASK-382
title: Named pins — builder docs and GitHub process for proposing additions
status: closed
closed: 2026-05-11
opened: 2026-05-11
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: Clarification
epic: named-pins
order: 6
prerequisites: [TASK-377, TASK-378, TASK-379]
---

## Description

Close out the named-pins epic with the builder-facing documentation
and the contribution process for extending the standard set.

Two deliverables:

1. **Builder doc** under `docs/builders/` (persona check via
   `/doc-check` before commit). Covers:
   - **What named pins are and when to use them.** Plain language,
     no schema-speak: "if you want to share a profile with another
     builder whose board is wired differently, use names instead
     of pin numbers."
   - **The two reference forms** — direct (`D23`) and named
     (`pedal_a`) — with one worked example per form, side by side.
   - **How to map a pin to a name** in the hardware-config editor
     (screenshot or step-by-step; reuse the editor delivered by
     TASK-381).
   - **What happens when a profile uses an unmapped name** — the
     warning surfaces from TASK-381 and the end-of-load summary
     from TASK-380, with screenshots if available.
   - **Mixed mode is fine.** Encourage builders to use names for the
     portable parts of a profile and direct IDs for one-off
     prototype pins.
   - **The v1 standard set**, rendered from the canonical file
     (TASK-377) — preferably generated rather than hand-copied, so
     the doc cannot drift. If generation is overkill for a small
     list, hand-copy with a comment naming the source file as the
     authority.

2. **GitHub-addition process**. A short, opinionated path for a
   builder who needs a role not in the v1 set:
   - Open a GitHub issue using the "Propose a standard pin name"
     template (create the template under `.github/ISSUE_TEMPLATE/`
     if one fits the project's conventions; otherwise a documented
     issue prefix is acceptable).
   - Required fields: proposed name (lowercase snake_case),
     one-line role description, why an existing name does not fit,
     and at least one builder context where this role exists.
   - Project response: triage → accept (add to v1 set in a follow-up
     PR) or decline (close with a one-line reason). The doc should
     name "lightweight, fast turnaround" as the goal — this is
     builder-scope friction we are accepting, not bureaucracy we are
     imposing.

## Acceptance Criteria

- [ ] Builder doc exists under `docs/builders/` and covers: what
      named pins are, the two reference forms, how to map, the
      missing-mapping warning behaviour, mixed-mode encouragement,
      and the rendered v1 set with the canonical file named as the
      authority.
- [ ] `/doc-check` returns a Match verdict for the doc's persona
      placement.
- [ ] A short "Propose a standard pin name" path is documented and
      either (a) an issue template under `.github/ISSUE_TEMPLATE/`
      exists, or (b) the doc points at an existing convention.
- [ ] The doc links forward to the configurator pages it describes
      (delivered by TASK-381) and backward to the canonical v1 set
      file (delivered by TASK-377).
- [ ] OVERVIEW / KANBAN reflect EPIC-029 as fully closeable once
      this task lands (`/housekeep` after the impl commit).

## Test Plan

No automated tests required — change is documentation + a GitHub
issue template. Run `/lint` and `/fix-markdown` over the new doc
before commit. If an issue template is added, sanity-check it loads
correctly in the GitHub "New issue" UI (one manual verification
counts; CI does not validate this).

## Prerequisites

- **TASK-377** — canonical v1 set file rendered/linked by the doc.
- **TASK-378** — hardware-config `pinNames` shape that the doc
  walks the builder through.
- **TASK-379** — profile-side reference forms shown in the doc's
  worked examples.

## Notes

- **Persona check.** This doc is for builders (people who solder
  and configure hardware), not musicians (people who play through
  the pedal) and not developers. `/doc-check` must agree.
- **Generated vs hand-copied v1 set.** If the canonical file is a
  JSON Schema enum, a tiny script that renders the enum as a
  markdown table during `/housekeep` (or as a pre-commit step) is
  ideal — eliminates drift. If that adds more machinery than it is
  worth for a list of ≤30 names, hand-copy with a `<!-- generated
  from data/pin_names.json -->` marker so the next editor knows
  not to drift it.
- **Friction tone.** The GitHub-addition path is described as
  "lightweight" deliberately. IDEA-061 explicitly accepted builder-
  scope friction; do not document a heavyweight review process. A
  one-line "is this role meaningfully different from existing
  ones?" check is sufficient.
- **Cross-links.** The doc should be findable from the builder
  hardware-config docs (a "See also: named pins" link) so a builder
  setting up their first board discovers it without a search.
