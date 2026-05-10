---
id: TASK-347
title: Fix /commit + commit-pathspec.sh to handle deletions and renames (orphan-deletion bug)
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: commit-atomicity
order: 8
---

## Description

`/commit` and `scripts/commit-pathspec.sh` currently support **adding**
new files and **modifying** tracked files, but they do not properly
support **deletions** or **renames**. The visible symptom is an
accumulation of orphan task files in the source-folder of every move
that has gone through `/ts-task-done` (or `/ts-task-active`,
`/ts-task-pause`, …).

When `/ts-task-done` runs:

1. `housekeep.py` does `git mv active/task-NNN.md closed/task-NNN.md`
   — this stages **both** sides of the rename in the real index
   (delete `active/`, add `closed/`).
2. The skill (or Claude) then assembles a pathspec list and invokes
   the wrapper. In practice, only the destination path
   (`closed/task-NNN.md`) ends up in the pathspec — the prose says
   "the renamed task file" (singular), and `git add closed/...`
   stages only one side.
3. `git commit -m "..." -- closed/task-NNN.md` builds a temp index
   from HEAD + the pathspec, **discarding the staged deletion of
   `active/task-NNN.md`**. The commit only contains the addition.
4. The deletion is left dangling in the working tree (file gone from
   disk, but HEAD still tracks it at the source path) and surfaces as
   `D` in subsequent `git status` runs.

Concrete evidence — at session-start on 2026-04-30, the working tree
showed:

```text
 D docs/developers/tasks/active/task-319-remove-wireviz-precommit-block.md
 D docs/developers/tasks/active/task-320-remove-ble-flag-precommit-check.md
 D docs/developers/tasks/active/task-321-fix-ci-failures-mermaid-clang-tidy.md
 D docs/developers/tasks/open/task-319-remove-wireviz-precommit-block.md
 D docs/developers/tasks/open/task-320-remove-ble-flag-precommit-check.md
 D docs/developers/tasks/open/task-323-convert-commit-skill-to-pathspec-form.md
…
```

Six orphan deletions, one per task that closed without its source-side
move ever landing.

The bug now has a second layer that **TASK-329 inadvertently introduced**.
The post-329 wrapper's untracked-pathspec check uses
`git ls-files --error-unmatch -- "$f"` to decide whether to reject
with exit 2. After `git mv A B`, `A` is no longer in the index — so
the wrapper rejects `A` as "untracked" and refuses to commit. **The
wrapper currently cannot commit a rename at all** when the source
path is named in the pathspec.

## Acceptance Criteria

- [ ] `commit-pathspec.sh` distinguishes "truly untracked" from
      "staged-deletion / rename-source" in its pre-flight check. A
      pathspec entry that is missing from disk is accepted if it was
      in HEAD or is staged-deleted in the index; only a path that is
      not in HEAD, not in the index, and (optionally) on disk as a
      stranger is rejected with exit 2.
- [ ] `scripts/tests/test_commit_pathspec.py` extended with two
      cases:
      - After `git mv A B` in a throwaway repo, the wrapper accepts
        pathspec `A B` and the resulting commit records the rename.
      - After deleting a tracked file from disk (no `git rm`),
        the wrapper accepts the pathspec and the resulting commit
        records the deletion.
- [ ] `.claude/skills/commit/SKILL.md` documents move/rename and
      deletion handling explicitly: BOTH the source and destination
      paths must appear in the pathspec for a rename; for a plain
      deletion the path must appear in the pathspec.
- [ ] `.claude/skills/ts-task-done/SKILL.md` updated: the "stage the
      renamed task file" instruction is replaced with explicit
      language that names BOTH the old (active/paused/open) and new
      (closed) paths in the pathspec. Same fix for
      `/ts-task-active`, `/ts-task-pause`, `/ts-task-reopen` —
      anywhere housekeep does a `git mv`.
- [ ] Awesome-task-system mirror: any skill files mirrored under
      `awesome-task-system/skills/` are updated identically; sync
      check passes.
- [ ] One-shot cleanup commit included in this task: stage and
      commit the existing orphan deletions in working tree (the
      `active/task-*` and `open/task-*` `D` entries for tasks that
      already exist in `closed/`). This proves the new flow works
      end-to-end on real history.

## Test Plan

**Host tests** (`make test-host`): no — bash wrapper + skill prose +
Python script-level tests only.

**Script-level tests** (`python -m unittest scripts.tests.test_commit_pathspec`):

- Two new cases as listed in the acceptance criteria (rename via
  `git mv`, plain deletion).
- Existing cases must continue to pass — TASK-329's invariant
  "wrapper rejects truly-untracked entries" stays.

**Smoke test**:

- Activate / close a task and confirm `git status` is clean of
  source-side `D` entries afterwards.
- Run `git log --diff-filter=D --name-only --since=<this-task>`
  and confirm rename source paths appear in the deletion list.

## Notes

- **This task supersedes part of TASK-329's framing** — but not its
  implementation. TASK-329 was billed as a fix for the
  TASK-319-in-three-folders symptom, but that symptom was primarily
  caused by source-side deletions never reaching commits, **not** by
  the wrapper's auto-`git add`. The auto-add removal was a useful
  separate refinement (a real footgun for the same-path-twice race);
  it stays. After this task, the wrapper still does not auto-`git add`
  for new files; it just stops misclassifying rename sources as
  untracked.
- **Why `git ls-files --error-unmatch` is the wrong primitive.** It
  asks "is this path in the index?". After `git mv A B`, A is not in
  the index — but A is not untracked either; it is a rename source.
  A correct check is closer to: `is this path in HEAD?` plus
  `is this path in the index with a deletion entry?`. The simplest
  correct primitive is probably `git diff-index --cached HEAD
  --name-status -- "$f"` to discover the planned change for `f`,
  combined with a `[ -f "$f" ]` working-tree check.
- **Fail-fast still wins over silent.** A genuinely bogus pathspec
  (path not in HEAD, not in index, not on disk) must still exit
  non-zero. The new check tightens around the legitimate
  rename/delete cases, not around bogus ones.
- **Cleanup commit caveat.** The 6+ orphan deletions visible at
  session-start come from many separate close commits over weeks.
  They are not the work of any single agent and the cleanup is not
  a fix for an in-progress feature — it is a one-shot index repair.
  Land it as the last commit of this task with message
  `cleanup: stage orphan deletions left by pre-TASK-347 close commits`.
- **No relation to TASK-328's housekeep flock.** TASK-328 prevents
  two `housekeep --apply` runs from racing on the index files. This
  task fixes wrapper/skill behaviour around already-staged renames.
  Orthogonal scopes.
