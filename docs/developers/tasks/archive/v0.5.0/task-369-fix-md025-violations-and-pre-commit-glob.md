---
id: TASK-369
title: Sweep MD025 violations across docs/ and fix the pre-commit hook glob
status: closed
closed: 2026-05-07
opened: 2026-05-02
effort: Medium (2-8h)
effort_actual: Medium (2-8h)
complexity: Medium
human-in-loop: No
---

## Description

[scripts/pre-commit:173](../../../../scripts/pre-commit) lints `"*.md"` —
the unquoted glob in markdownlint-cli2 only matches `.md` files in the
**repo root** (no recursion). As a result, every `.md` file under
`docs/` has been silently skipped by the pre-commit hook for as long as
that line has existed.

The latent backlog surfaced during the IDEA-053 review (2026-05-02): an
IDE diagnostic flagged MD025/single-title/single-h1 on
`docs/developers/ideas/open/idea-053-context-sensitive-helper-system.md`,
and a fleet scan with `npx markdownlint-cli2 "**/*.md"` showed the
same violation on essentially every idea and task file that combines a
YAML frontmatter `title:` field with an explicit `# H1` line. The
project's `.markdownlint.json` enables MD025 with defaults — its
`front_matter_title` regex treats the YAML `title:` as the document's
H1, so any explicit `# H1` after the frontmatter is a duplicate.

This task:

1. Fixes the hook glob so the rule is actually enforced going forward.
2. Clears the existing backlog by dropping the redundant `# H1` line
   on every violating file.
3. Verifies that introducing a fresh violation would now be blocked.

The Makefile's `lint-markdown` target already uses the correct
`"**/*.md"` recursive glob with the right ignore patterns
(`#node_modules` `#.pio` `#build` `#.vscode`) — the pre-commit hook
should match.

## Acceptance Criteria

- [ ] `scripts/pre-commit` line 173 is updated to lint
  `"**/*.md" "#node_modules" "#.pio" "#build" "#.vscode"` (matching
  the Makefile target).
- [ ] `npx markdownlint-cli2 "**/*.md" "#node_modules" "#.pio" "#build" "#.vscode"`
  reports 0 errors across the whole repo.
- [ ] Verified that a test commit reintroducing an MD025 violation
  (an explicit `# H1` line on any `.md` file under `docs/`) is
  rejected by the pre-commit hook. Revert the test change without
  committing.
- [ ] No frontmatter `title:` fields are touched. The fix removes the
  redundant body-level H1 line only.
- [ ] `python scripts/housekeep.py --apply` followed by
  `make lint-markdown` still passes — the regenerated index files
  (OVERVIEW.md, EPICS.md, KANBAN.md, ideas OVERVIEW.md) are clean.

## Test Plan

No automated tests required — change is non-functional (lint-only).
Verification is via the AC above:

- `npx markdownlint-cli2 "**/*.md" "#node_modules" "#.pio" "#build" "#.vscode"`
  → must report 0 errors.
- Reintroduce one H1 line in a sample file (e.g. an idea), attempt
  `git commit` via `/commit`, observe pre-commit rejection, revert.
- Re-run `python scripts/housekeep.py --apply` and confirm no new lint
  errors on regenerated indexes.

If `housekeep.py` or `update_idea_overview.py` emit a redundant H1 in
their generated output, fix the generator as part of this task — a
hook that rejects regen output is worse than the current silent skip.

## Notes

- **Bundle the H1 sweep into one script.** Per the
  `feedback_one_script_for_parsing` memory, a single Python pass that
  walks the markdownlint-cli2 violation report and rewrites each
  offending file is preferable to one Edit per file. The transform is
  mechanical: locate the first H1 line (a line starting with `#`
  followed by a space) after the closing `---` of the frontmatter,
  drop it and the blank line that follows if present, leave the rest
  of the body untouched.
- **Pre-commit hook is load-bearing.** See
  [docs/developers/COMMIT_POLICY.md](../../COMMIT_POLICY.md) and the
  CLAUDE.md sections on parallel sessions and pathspec form. A hook
  that silently skips most of the docs tree produces false confidence,
  so this fix matters beyond the cosmetic lint cleanup.
- **Other lint rules may surface.** When the recursive glob is enabled
  for the first time, expect markdownlint to report rules beyond
  MD025 across `docs/` (e.g. MD034 bare URLs, MD041 first-line H1,
  MD007 list indentation). Out of scope for this task — file
  follow-up tasks per rule if they appear, do not silently fix them
  here. The AC is "0 errors" because the sweep should clear them all,
  but if the volume is large, narrow the scope to MD025 only and open
  follow-ups for the rest.
- **Project stance:** "obey the rules" (memory:
  `feedback_obey_lint_rules`). Do not soften MD025 in
  `.markdownlint.json` to make the violations vanish — the
  frontmatter `title:` field is the canonical H1 already, and
  removing the redundant body H1 is the correct fix.
- **Don't touch closed task files unless lint forces you to.** Closed
  tasks are historical record — if MD025 fires on them, fix them, but
  do not "tidy up" anything else. Edit minimally.
