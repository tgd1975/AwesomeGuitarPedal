---
id: TASK-372
title: Configure CodeQL to ignore vendored test-framework code
status: closed
closed: 2026-05-08
opened: 2026-05-08
effort: Small (<2h)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
---

## Description

GitHub Advanced Security CodeQL currently scans the CMake `_deps/` directory
that gets created during the host-test build at `.vscode/build/_deps/`, which
pulls in GoogleTest source as a transitive dependency. CodeQL then reports
high-severity alerts (`cpp/path-injection`, `cpp/uncontrolled-process-operation`)
against the GoogleTest source itself — code we do not maintain and never ship.

Five such false-positive alerts currently block PR merges because the repo has a
code-scanning gate that fails when high-severity alerts exist. The gate
discovered this during the v0.5.0 release PR (#9).

The fix is to tell CodeQL to skip vendored / generated build directories: add a
`.github/codeql/codeql-config.yml` with a `paths-ignore` list and reference it
from `.github/workflows/codeql-analysis.yml`.

## Acceptance Criteria

- [ ] `.github/codeql/codeql-config.yml` exists with `paths-ignore` entries for
      `**/_deps/**` and `.vscode/build*/**`
- [ ] `.github/workflows/codeql-analysis.yml` references the config file via the
      `config-file:` input on `github/codeql-action/init@v3`
- [ ] On the next CodeQL run after this task lands on `main`, no new alerts
      are reported under `.vscode/build/_deps/googletest-src/`
- [ ] The five existing GHAS alerts (#3, #4, #5, #6, #7) are dismissed as
      "won't fix — vendored code, out of analysis scope" once paths-ignore is
      verified working

## Test Plan

No automated tests required — change is non-functional. Validation is via the
CodeQL workflow run on the merge PR for this task: a successful run with zero
alerts under `_deps/` confirms `paths-ignore` is effective.

## Notes

- This is one half of a two-part fix to clear the GHAS code-scanning gate that
  blocked the v0.5.0 release PR. TASK-373 covers the other half: the two real
  `cpp/comparison-with-wider-type` findings in `lib/PedalLogic/`.
- The five GHAS alerts will not auto-close when `paths-ignore` lands — they
  must be dismissed via the GitHub Security tab. Dismissal should reference
  this task in the comment.
- Out of scope: changing the build directory layout, switching test framework,
  or adjusting the CodeQL query suite. Only the analysis path scope is touched.
