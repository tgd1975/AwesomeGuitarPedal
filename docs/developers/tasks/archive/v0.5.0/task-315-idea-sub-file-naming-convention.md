---
id: TASK-315
title: Idea sub-file naming convention — one IDEA = one OVERVIEW row
status: closed
closed: 2026-04-29
opened: 2026-04-29
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: No
---

## Description

Some ideas (IDEA-027 Circuit-Skill, IDEA-043 task-system rollout)
have multiple frontmatter-bearing files in `docs/developers/ideas/`,
which makes them appear as multiple rows in OVERVIEW. Establish a
naming convention so a complex idea can have supporting notes
without polluting the index.

**Convention:**

- Main file:   `idea-NNN-<short-slug>.md` — has frontmatter with
  `id: IDEA-NNN`, appears in OVERVIEW.
- Sub-files:   `idea-NNN.<sub-slug>.md` — note the **dot** between
  `NNN` and the sub-slug. No frontmatter, never appears in OVERVIEW.
  Linked from the main file.

The dot rule is grep-able: `idea-NNN-*` matches the main, `idea-NNN.*`
matches sub-files. Filename sort still groups them.

## Acceptance Criteria

- [ ] `docs/developers/ideas/README.md` created, documenting the
  convention with a small example.
- [ ] `docs/developers/ideas/OVERVIEW.md` intro paragraph links to
  the README so readers can find the rule. (Update the generator so
  the link survives regeneration.)
- [ ] IDEA-027 and IDEA-043 sub-files renamed (`-` → `.`) and their
  `id:`/`title:`/`category:`/`description:`/`related:` frontmatter
  stripped (or replaced with a one-line "sub-note of IDEA-NNN" header).
- [ ] Main IDEA-027 and IDEA-043 files link to their sub-files.
- [ ] After regeneration, OVERVIEW shows exactly one row per IDEA-NNN.
- [ ] `ts-idea-new` skill mentions the sub-file convention.

## Test Plan

**Host tests** — extend `test_update_idea_overview.py`:

- Sub-file (filename matches `idea-NNN.*`) is skipped by the loader.
- Main file (filename matches `idea-NNN-*`) renders normally.

## Notes

Edit the package copy under `awesome-task-system/` for both the
generator script and the `ts-idea-new` skill, then sync.
