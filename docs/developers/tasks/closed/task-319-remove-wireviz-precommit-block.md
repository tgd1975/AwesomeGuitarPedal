---
id: TASK-319
title: Pre-commit hook — remove WireViz regen block and verify no remnants
status: closed
closed: 2026-04-30
opened: 2026-04-29
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
---

## Description

WireViz is no longer used in the project. The pre-commit hook still
regenerates outputs from `*.yml` sources under `docs/builders/wiring/`
that no longer exist — only `.svg` files remain in that tree. The
block is dead code that prints "Regenerating WireViz outputs…" on
every commit and falls through silently because no `.yml` files match.

Remove the block from both the canonical hook source
(`scripts/pre-commit`) and the live hook (`.git/hooks/pre-commit`),
then sweep the repo for stale references (scripts, Makefile, CI
workflows, developer docs, `requirements-wiring.txt`).

## Acceptance Criteria

- [ ] WireViz regen block removed from `scripts/pre-commit`
- [ ] `wireviz` referenced from no active script, Makefile, CI workflow,
      or developer doc (archived task/idea files may still mention it —
      leave those alone)
- [ ] `requirements-wiring.txt` removed if it exists
- [ ] No `.yml` sources or `.bom.tsv` artefacts left under
      `docs/builders/wiring/`

## Test Plan

No automated tests required — change is non-functional.

Manual: stage any tracked file and run `git commit --dry-run` (or the
real commit). Confirm the hook no longer prints "Regenerating WireViz
outputs…" or "wireviz not found — skipping regeneration".

## Notes

The hook's "wireviz not found" branch suggests `pip install -r
requirements-wiring.txt`. Verify the file is gone too.

Idea files under `docs/developers/ideas/open/` historically mention
WireViz. Those are speculative records, not current state — leave them
alone.
