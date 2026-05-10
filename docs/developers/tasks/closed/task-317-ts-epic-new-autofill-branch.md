---
id: TASK-317
title: `/ts-epic-new` auto-fills `branch: feature/<epic-slug>`
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
epic: epic-suggested-branch
order: 2
prerequisites: [TASK-316]
---

## Description

Update the `/ts-epic-new` skill so newly scaffolded epics carry a
`branch:` field auto-filled with `feature/<epic-slug>`. The slug is the
same one used in the epic filename (lowercase, hyphenated, max 50
chars), so the resulting branch name is short and valid by
construction.

The user can edit or remove the field in the file before committing —
this is opt-out, not opt-in, but never a prompt during scaffolding.

## Acceptance Criteria

- [ ] `awesome-task-system/skills/ts-epic-new/SKILL.md` instructs the
      agent to write `branch: feature/<slug>` into the new epic's
      frontmatter, where `<slug>` is the same string used in the
      filename.
- [ ] Running `/ts-epic-new foo "Foo bar"` produces a file whose
      frontmatter contains `branch: feature/foo`.
- [ ] If the epic name contains characters that are valid in a slug but
      not in a git branch name (none today, but defensive), the slug is
      still safe — document the rule, do not invent a sanitization
      step.

## Test Plan

No automated tests required — skill instruction change. Manual
verification:

- Run `/ts-epic-new sample-epic "Sample"` in a scratch branch, confirm
  the frontmatter has `branch: feature/sample-epic`, then revert.

## Prerequisites

- **TASK-316** — defines the `branch:` field; this task only sets a
  default when scaffolding a new epic.

## Documentation

- `awesome-task-system/skills/ts-epic-new/SKILL.md` — update the
  template and steps to include the auto-filled `branch:` line.
- Sync to live copy.

## Notes

- The slug is already computed in step 2 of the skill ("Build the
  filename: `epic-NNN-<slug>.md`"). Reuse that value verbatim — do not
  derive the branch from the title.
