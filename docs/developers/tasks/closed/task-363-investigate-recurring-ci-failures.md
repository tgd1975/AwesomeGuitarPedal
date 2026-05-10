---
id: TASK-363
title: Investigate why CI still fails after TASK-321 — fix or scaffold follow-up
status: closed
closed: 2026-05-08
opened: 2026-05-02
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Clarification
---

## Description

[TASK-321](../closed/task-321-fix-ci-failures-mermaid-clang-tidy.md) closed
on 2026-05-01 with the explicit goal of making CI reliably green ("Fix CI
failures — Mermaid lint, clang-tidy — and prevent dirty pushes"). Despite
that, CI is still going red on `main` and on PRs. This task is a triage
pass: figure out what is failing now, why TASK-321 did not cover it, and
either fix it on the spot or scaffold a follow-up task with a clear scope.

This is deliberately a *small* triage task with two exit paths:

1. **Fix in scope** — if the root cause is small (a single workflow
   misconfiguration, an unpinned dependency, a flaky retry, an un-fixed
   lint rule), apply the fix in this task and confirm a clean CI run.
2. **Scaffold a follow-up** — if the root cause is structural (CI matrix
   overhaul, dependency upgrade, new workflow pattern, build-system
   change), scaffold a TASK-NNN follow-up with the diagnosis and proposed
   approach, and close *this* task with the diagnosis as the deliverable.
   The follow-up task carries the actual fix.

The goal is to avoid the trap of starting a "small CI fix" that quietly
balloons into a multi-hour rewrite. If the diagnosis takes longer than
~30 minutes or the fix would exceed Small, exit via path 2.

## Acceptance Criteria

- [x] The currently failing CI check(s) are named, and the underlying
      cause of the failure is documented in the closure note (workflow
      file, job name, error signature, root cause).
- [x] The diagnosis answers: *why did TASK-321 not cover this?* —
      out of scope (different workflow / different check), regressed
      since (something changed after TASK-321 landed), or never green
      to begin with (TASK-321 fixed the named checks but missed
      others).
- [x] **Either** CI is green again on the current branch and a recent
      run on `main` is green, **or** a follow-up task (TASK-NNN) exists
      in `open/` with a clear scope linking back here, and this task
      closes with the diagnosis as its deliverable.
- [x] If the fix landed in scope, a one-line note in the closure
      explains why TASK-321 missed this case (so the same gap is not
      reopened by the next "fix CI" pass).

## Test Plan

No automated tests — change is investigation/triage. Verification is
running the affected workflows in CI:

- `gh run list --branch <current-branch> --limit 5` to confirm the
  failing run signature.
- After the fix (option 1), push and confirm the failing workflow goes
  green: `gh run watch <run-id>`.
- For option 2, no CI verification on this task — the follow-up task
  carries that responsibility.

## Notes

- **Scope discipline.** This is a Small task on purpose. If the
  diagnosis reveals a fix bigger than Small, *stop fixing* and switch
  to option 2 — scaffold the follow-up. Resist the temptation to keep
  pulling the thread.
- **Where to look first.** `gh run list --workflow=<each-workflow>
  --status=failure --limit=10` for each workflow under
  `.github/workflows/`. The failure signature usually points to one
  of: lint regression on a file added since TASK-321, a flaky network
  step, a dependency that drifted, or a workflow TASK-321 did not
  touch (TASK-321 named Mermaid + clang-tidy specifically).
- **Cross-link.** If a follow-up task is scaffolded, add a
  `prerequisites: [TASK-363]` line to the new task and a `Notes` line
  here pointing at it ("Follow-up: TASK-NNN").
- **Adjacent tasks.** [TASK-352](task-352-investigate-pre-commit-hook-latency.md)
  is a sibling investigation focused on the *local* pre-commit hook.
  TASK-363 is the *CI-side* counterpart. They likely share root
  causes but should not be merged — closing one should not require
  closing the other.
- [TASK-362](task-362-require-ci-pass-before-merge-to-main.md) plans
  to enforce green CI as a merge gate on `main`. Without TASK-363
  resolving the chronic redness first, TASK-362 will block all merges
  and become a usability problem rather than a quality gate. Treat
  TASK-363 as an informal prerequisite for activating the gate in
  TASK-362, even though it is not listed in `prerequisites:` (the
  gate work itself can proceed in parallel; it is only the *flip the
  switch* step that should wait).
- Follow-up: [TASK-370](task-370-make-schemdraw-svgs-deterministic.md)
  carries the actual fix for the Schemdraw staleness guard.

## Closing Comment (2026-05-08)

**Currently failing checks on `main`:**

1. **Static Analysis / clang-tidy** — exit 123 in *Run clang-tidy*
   (most recent: [run 25080496843](https://github.com/tgd1975/AwesomeStudioPedal/actions/runs/25080496843)).
   Same brace-style and macro-usage errors in `lib/PedalLogic/src/*.cpp`
   that TASK-321 fixed. Error signature: `readability-braces-around-statements`,
   `cppcoreguidelines-macro-usage`, `performance-enum-size`,
   `clang-diagnostic-implicit-int-conversion`.
2. **Docs / Schemdraw staleness guard** — exit 1 in *Fail if any SVG is stale*
   (most recent: [run 25080496759](https://github.com/tgd1975/AwesomeStudioPedal/actions/runs/25080496759)).
   `git diff` finds non-empty diff on `<dc:date>` element after each
   regeneration, e.g. `-<dc:date>2026-04-24T23:45:42.501318</dc:date>`
   vs. `+<dc:date>2026-04-28T22:16:07.129647</dc:date>`.

**Why TASK-321 did not cover these:**

- *clang-tidy* — TASK-321 *did* fix this, but on `feature/config-extensions`,
  which is now 145 commits ahead of `main` and has not yet been merged
  or released. TASK-321's AC only required CI green on the feature
  branch; nothing carried it to `main`. So this is not a missed scope —
  it is "fix landed on feature branch, not yet on main." It will resolve
  on the next release-branch merge to `main` (no extra fix needed).
- *Schemdraw staleness guard* — out of scope for TASK-321. TASK-321
  named "Mermaid lint, clang-tidy"; the Schemdraw guard lives in a
  different workflow (`.github/workflows/docs.yml`), is checked by a
  different job, and was added by EPIC `circuit_diagram_as_code` /
  TASK-204. It has been failing since it landed because matplotlib's
  SVG backend (used by schemdraw) embeds a non-deterministic
  `<dc:date>` timestamp in the RDF metadata block — every CI
  regeneration produces a "fresh" timestamp and the staleness diff is
  always non-empty. The check is flaky-by-design and has never been
  green; pre-existing breakage, not a regression.

**Disposition (path 2 — scaffold follow-up):**

- *clang-tidy on `main`* — no separate fix task; resolves on the next
  release-branch merge (TASK-321's fix is already on
  `feature/config-extensions`).
- *Schemdraw staleness guard* — fix is structural (suppress matplotlib
  metadata or post-process SVG), too big for this Small task.
  Scaffolded [TASK-370](../open/task-370-make-schemdraw-svgs-deterministic.md)
  with the diagnosis and two viable fix approaches.

**Why the same gap won't reopen.** "Fix CI" passes that name only the
red checks (TASK-321 named "Mermaid + clang-tidy") will continue to
miss workflows that go red later. The pattern fix is to start every
CI-triage task with `gh run list --status=failure` *across all
workflows*, not just the one that prompted the task. TASK-370's AC
explicitly verifies the Docs workflow on push, closing the named gap
for the Schemdraw guard.
