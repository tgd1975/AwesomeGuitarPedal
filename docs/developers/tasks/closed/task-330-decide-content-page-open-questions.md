---
id: TASK-330
title: Decide content-page open questions (source-of-truth, i18n, context-awareness, first-run)
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Main
epic: app-content-pages
order: 1
---

## Description

IDEA-037 leaves four design questions unresolved. Resolve them before any
UI is built so all subsequent content-page tasks share the same
foundation.

The four questions:

1. **Source of truth for content** — hand-write per page in Dart strings,
   or pull from existing developer docs (`docs/users/...`) so we don't
   maintain two copies.
2. **Localization** — English only for now, or scaffold `flutter_intl` /
   `flutter_localizations` from day one.
3. **How-To context-awareness** — static walkthrough, or context-aware
   (show "pair your pedal" only while no pedal is connected).
4. **First-run flow** — auto-show How-To on first launch, or purely
   opt-in via the menu.

Output: a short decision document under
`docs/developers/decisions/` (or appended to ARCHITECTURE.md if that's
the convention) capturing the decision and one-line rationale per
question. Subsequent tasks (TASK-331..TASK-338) reference this.

## Acceptance Criteria

- [x] All four questions answered with a one-line rationale each.
- [x] Decision document committed (or rationale appended to existing
      design doc) and linked from this task.
- [x] If localization is "scaffold from day one", a follow-up scaffolding
      step is named (in this task or as a new prerequisite for TASK-331).
      *(Resolved as English-only; deferred work captured in IDEA-052.)*

## Decision

See [APP_CONTENT_PAGES_DECISIONS.md](../../APP_CONTENT_PAGES_DECISIONS.md):

1. **Content source of truth** — re-author per page in Dart strings (app-specific copy, not sourced from `docs/users/`).
2. **Localization** — English only; revisit later via [IDEA-052](../../ideas/open/idea-052-app-localization.md).
3. **How-To context-awareness** — static walkthrough; smart helper deferred to [IDEA-053](../../ideas/open/idea-053-context-sensitive-helper-system.md).
4. **First-run flow** — auto-show How-To with one-tap "Got it, don't show again" dismiss.

## Test Plan

No automated tests required — change is non-functional.

## Notes

- This is a decision task, not an implementation task. Keep it small.
- The user is the decision-maker; agent's role is to surface trade-offs
  and write up the chosen path.
