---
id: TASK-348
title: Derive clang-tidy lint set from compile_commands.json
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: No
---

## Description

Both `.github/workflows/static-analysis.yml` and `scripts/pre-commit`
currently hardcode the clang-tidy input set as
`find lib/PedalLogic/src -name "*.cpp"`. This is brittle: any new C++
source dropped under `lib/<NewLib>/src/` or `src/host/src/` is silently
*not* linted, with no warning. The lint set drifts from the build set
on every codebase growth.

Switch both call sites to derive the lint set from
`.vscode/build/compile_commands.json` (which CMake already generates
with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`). That makes the build graph
the single source of truth: anything CMake compiles on host gets
linted, no config update required when files are added.

This complements TASK-346 (path-gating) — TASK-346 decides *whether*
to run the C++ checks; this task makes the lint set itself robust
against codebase growth.

## Acceptance Criteria

- [ ] CI's static-analysis job lints every `.cpp` listed in
      `compile_commands.json`, excluding third-party (`_deps/`,
      `googletest`, `arduinojson`) and test harness (`/test/`).
- [ ] Pre-commit hook's clang-tidy block uses the same derivation;
      outer staged-paths gate (`lib|inc|src|test`) is unchanged.
- [ ] Adding a new file like `lib/AnotherLib/src/foo.cpp` to the host
      CMake build causes both CI and the hook to lint it without any
      further config edits.
- [ ] The narrow `^lib/PedalLogic/src/.*\.cpp$` regex is gone from
      `scripts/pre-commit`.

## Test Plan

No automated tests required — change is to CI config + a
developer-environment shell script. Manual verification:

1. Run the new clang-tidy invocation locally, confirm it lints the
   expected ~30 files (`lib/PedalLogic/src/*` + `src/host/src/*` +
   `src/esp32/src/{button,led_controller}` + top-level
   `src/{pedal_app,ble_pedal_app}.cpp`).
2. Add a temp `lib/SmokeTest/src/foo.cpp` with a deliberate
   clang-tidy violation, confirm the hook flags it without any
   config change. Remove temp file.
3. Push to a branch, verify CI's static-analysis job also flags it.

## Notes

- CI container `mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04` may
  not ship `jq` — add `apt-get install -y jq` to the install step,
  or use a `python3 -c "import json; …"` one-liner instead (no extra
  dep).
- Exclusion list: `_deps/` (FetchContent), `arduinojson`,
  `googletest`, `/test/`. Headers are linted transitively through
  their includer — no header path needed.
- Outer staged-paths gate in pre-commit (`^(lib|inc|src|test)/`)
  stays as-is — it's a yes/no "should we run any C++ check" gate,
  separate from clang-tidy's input list.
- Reference invocation:
  `jq -r '.[].file' .vscode/build/compile_commands.json | grep -v -E '_deps/|/test/|arduinojson|googletest' | xargs clang-tidy -p .vscode/build`
