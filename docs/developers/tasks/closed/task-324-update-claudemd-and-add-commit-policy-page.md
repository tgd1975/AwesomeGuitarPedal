---
id: TASK-324
title: Update CLAUDE.md and add COMMIT_POLICY.md rationale page
status: closed
closed: 2026-04-30
opened: 2026-04-30
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: commit-atomicity
order: 3
prerequisites: [TASK-322]
---

## Description

Codify the operational rule and document the rationale.

**CLAUDE.md** gets a new section (or extension of "Parallel sessions —
commit only your own work") stating unambiguously:

> Never run `git add` + `git commit` directly. Always use `/commit`.
> The pre-commit hook enforces this — bypass requires `<bypass mechanism
> from TASK-322>` and is reserved for `<allowed cases from TASK-322>`.

**`docs/developers/COMMIT_POLICY.md`** is a new page that owns the *why*:
the parallel-session race, why pathspec form is atomic, why the hook is
load-bearing rather than advisory, and the bypass policy. CLAUDE.md
links to it for the rationale; CLAUDE.md keeps only the rule.

This is Pillar 2 of IDEA-051.

## Acceptance Criteria

- [x] CLAUDE.md states the `/commit`-only rule with a link to
      `docs/developers/COMMIT_POLICY.md` for rationale.
- [x] `docs/developers/COMMIT_POLICY.md` exists, explains the race that
      pathspec eliminates, names the provenance signal and bypass
      mechanism from TASK-322, and lists who/when bypass is acceptable.
- [x] `/doc-check` passes on the new file (developer persona).

## Test Plan

No automated tests required — change is non-functional (docs).

## Prerequisites

- **TASK-322** — provides the chosen provenance signal and bypass
  mechanism that this documentation must name.

## Documentation

- `CLAUDE.md` — add the `/commit`-only rule and link to COMMIT_POLICY.md.
- `docs/developers/COMMIT_POLICY.md` — new page, the rationale home.

## Notes

- CLAUDE.md owns the operational rule; COMMIT_POLICY.md owns the why.
  This split keeps CLAUDE.md scannable and gives developers a stable
  place to find the design reasoning.
- Run `/doc-check` after creating COMMIT_POLICY.md to validate persona
  placement.
