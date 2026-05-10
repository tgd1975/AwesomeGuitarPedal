---
id: TASK-346
title: Make pre-commit C++ and app checks content-sensitive
status: closed
closed: 2026-04-30
opened: 2026-04-30
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: No
---

## Description

The pre-commit hook in `scripts/pre-commit` currently runs the full host-test
build (`cmake --build … pedal_tests` + `pedal_tests`) on every commit, even
when the staged files are pure docs / task-system / CI changes. clang-tidy
(line 301) and the Flutter checks (line 374) are already path-gated, but
clang-format and the host-test build/run are not.

Gate the heavyweight C++ and app checks on whether the commit actually
touches their input directories, using
`git diff --cached --name-only --diff-filter=ACMR` so only files staged
for *this* commit count — pathspec-form commits routinely leave unrelated
working-tree mods, and those must not trigger checks.

**Scope of gating:**

- **C++ checks** (clang-format, clang-tidy, host-test build+run) — run only
  when staged files include paths under `lib/`, `inc/`, `src/`, or `test/`.
- **App checks** (dart format, flutter analyze) — already gated on `app/`;
  verify and keep.
- **Everything else** (`/commit` provenance, housekeep, sync_task_system,
  markdown lint, doc-check) — unchanged, runs on every commit.

## Acceptance Criteria

- [ ] A docs-only commit (e.g. only `.md` files under `docs/`) skips
      clang-format, clang-tidy, and the host-test build/run, printing a
      green "no C++ paths staged — skipping" message for each.
- [ ] A commit touching any file under `lib/`, `inc/`, `src/`, or `test/`
      runs the full C++ chain exactly as today.
- [ ] A commit touching only `app/` runs the Flutter checks and skips the
      C++ checks.
- [ ] Gating uses `git diff --cached --name-only --diff-filter=ACMR`
      (staged only, no working-tree noise), consistent with the existing
      pattern at `scripts/pre-commit:301`.

## Test Plan

No automated tests required — change is to a developer-environment shell
script. Manual verification covers the AC scenarios on a throwaway branch:

1. Docs-only commit → C++ + Flutter skipped, doc/markdown/housekeep run.
2. `lib/`-only commit → full C++ chain runs, Flutter skipped.
3. `app/`-only commit → Flutter runs, C++ skipped.
4. Mixed `src/` + `app/` commit → both run.
5. Commit with only working-tree mods under `lib/` but pathspec restricted
   to `docs/` → C++ skipped (proves `--cached` gating, not working-tree).

## Notes

- Existing gated check at [scripts/pre-commit:301](scripts/pre-commit#L301)
  (clang-tidy on `^lib/PedalLogic/src/.*\.cpp$`) is the reference pattern.
- Host-test gate should be broader than clang-tidy's — any change under
  `lib/`, `inc/`, `src/`, or `test/` could affect the test build, not just
  `lib/PedalLogic/src`.
- Do **not** gate `/commit` provenance, housekeep, sync_task_system,
  markdown lint, or doc-check — those apply to every commit regardless of
  content.
- The `.git/hooks/pre-commit` shim is one line and delegates to
  `scripts/pre-commit`; all edits land in `scripts/pre-commit`.
