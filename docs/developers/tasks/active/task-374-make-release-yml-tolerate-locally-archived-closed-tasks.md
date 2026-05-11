---
id: TASK-374
title: Make release.yml tolerate locally-archived closed tasks
status: active
opened: 2026-05-11
effort: XS (<30m)
complexity: Junior
human-in-loop: No
---

## Description

The `/release` skill's Phase 4 step 6 archives closed tasks locally
(`docs/developers/tasks/closed/*.md` → `archive/vX.Y.Z/`) before
the release tag is pushed. `release.yml` then also has an
"Archive closed tasks for this release" step that runs
`scripts/organize_closed_tasks.py` after publishing the GitHub
Release — duplicating the local archiving.

When the local archive ran first, `closed/` is empty (or in the
v0.5.0 case, doesn't exist at all). The workflow step crashes with
`FileNotFoundError: [Errno 2] No such file or directory:
'docs/developers/tasks/closed'` (see
[organize_closed_tasks.py:42](scripts/organize_closed_tasks.py#L42)),
which paints the whole `Create GitHub Release` job red even though
the actual `Create GitHub Release` step (softprops/action-gh-release)
succeeded.

Confirmed on the v0.5.0 release: all 5 assets attached, release
published, but workflow shows failed. Same red status will recur on
every future release until fixed.

Two layers to address:

1. **Script-level guard**: make `organize_closed_tasks.py` treat a
   missing or empty `closed/` directory as a no-op (print message,
   exit 0). Already has `if not md_files: print(...); return` for
   the empty case — just needs `os.path.isdir(CLOSED_DIR)` first.

2. **Workflow design**: decide whether the workflow's archive step
   is needed at all. If `/release` always archives locally in Phase
   4, the workflow step is duplicative and only adds noise. Either
   remove the step from `release.yml`, or formally make `/release`
   skip the local archive (and rely on the workflow). Pick one
   canonical archiver.

## Acceptance Criteria

- [ ] `scripts/organize_closed_tasks.py` exits 0 with a "nothing to
      archive" message when `docs/developers/tasks/closed/` is
      missing or contains no `.md` files
- [ ] `release.yml`'s archive job is either removed (preferred) or
      tolerant of the local-archive-already-done case
- [ ] Decision recorded in the task close commit: which side is
      canonical — `/release` skill (local) or `release.yml`
      (workflow)
- [ ] Next release after this lands shows a fully-green workflow
      status

## Test Plan

No automated tests strictly required — the change is small and the
validation runs on the next real release workflow.

Optional: add `scripts/tests/test_organize_closed_tasks.py` that
creates a temp working tree without `closed/` and verifies the
script exits 0 with the "nothing to archive" message. Worth doing
if the script grows other branches.

## Notes

- Discovered during v0.5.0 release: GitHub Actions run
  [25641026116](https://github.com/tgd1975/AwesomeStudioPedal/actions/runs/25641026116),
  job "Create GitHub Release", step "Archive closed tasks for this
  release"
- The release succeeded despite this — `gh release view v0.5.0`
  shows all 5 assets attached. Pure cosmetics + future-noise issue.
- Related: the `/release` skill's Phase 4 step 6 archive could
  itself be moved into a script (`scripts/release_archive.py`) so
  there's one canonical archiver invoked from both contexts. Out
  of scope here; capture as a follow-up if the duplication keeps
  causing problems.
