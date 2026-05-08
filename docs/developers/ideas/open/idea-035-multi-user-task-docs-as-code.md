---
id: IDEA-035
title: Multi-User Workflow for Docs-as-Code Task System
category: tooling
description: How to evolve the docs-as-code task and idea system from "single user, current branch is the truth" to "two or more people working in parallel branches" without losing the things that make the current system pleasant.
---

## The problem

Today the system is built around a single assumption: **my current branch is the source of truth.** That assumption powers a lot of nice properties:

- Task IDs (`TASK-NNN`) are allocated by scanning the working tree.
- `housekeep.py` regenerates `OVERVIEW.md`, `EPICS.md`, `KANBAN.md` deterministically from local files.
- The pre-commit hook can confidently move files between `open/` / `active/` / `closed/` without coordinating with anyone else.
- Closing a task = a commit on my branch, immediately reflected in the dashboards I look at.
- "What is the project state?" is answered by "what does my checkout look like".

This breaks the moment a second person works in parallel. Concretely the failure modes are:

1. **ID collisions.** Both branches independently allocate `TASK-260`. Whoever merges second has to renumber and rewrite their file (and any references to it).
2. **State race.** Person A moves `TASK-200` from `active/` to `closed/` on branch A. Person B, on branch B from the same base, sees it as `active/` and still operates on it — maybe even adds notes — and on merge we get the file in two places, or the housekeep hook fights itself.
3. **Generated-file conflicts.** `OVERVIEW.md` / `EPICS.md` / `KANBAN.md` are deterministic outputs of the file tree. They are also the files that change in *every* task-touching commit. So almost every parallel branch produces a merge conflict in these files, even when the underlying tasks don't conflict at all.
4. **No global "current state".** Today I look at `OVERVIEW.md` on `main` and trust it. With two people, `main` is only as fresh as the last merged PR, and the actually-current state is "main + N open PRs". There is no place that shows that.
5. **Active-state semantics get fuzzy.** `status: active` on a feature branch isn't visible to the other person. They could assign themselves the same task without knowing.

The question is: which of these do we accept, which do we engineer around, and what's the minimum change that keeps the system feeling lightweight?

## What "multi-user" actually means here

Worth being explicit because the answer changes a lot:

- **Scale:** still 2–5 people, not 50. We don't need GitHub Issues or Jira.
- **Topology:** people work on feature branches off `main`, open PRs, merge back. Same as the code workflow. There is no shared long-running "tasks" branch.
- **Latency:** it is OK if "I started TASK-200" only becomes visible to my collaborator when I push, not the second I run `/ts-task-active`. We are not building a real-time multiplayer system.
- **What must work:** parallel task creation, parallel state changes on *different* tasks, merging without rewriting history.
- **What can be a soft norm:** two people don't pick up the same task at the same time. We don't need a lock; we need visibility.

## Options

These are not mutually exclusive — most real solutions will combine pieces. I've tried to keep each option small and orthogonal so we can mix.

### Option 1 — "main is the truth" (push state to a shared branch eagerly)

Keep the current model but redefine the source of truth: **`main` is the truth, not my branch.** On every state change (`/ts-task-active`, `/ts-task-pause`, `/ts-task-done`), push a tiny commit directly to `main` (or to a dedicated `task-state` branch that's permissive about direct pushes), bypassing the PR flow for *task metadata only*.

- Pros: state is globally visible immediately. ID allocation is safe because allocation also happens against `main`. No merge conflicts in generated files because only one person at a time is committing to `main` for state changes.
- Cons: violates the "all changes go through PRs" norm. Pushes to main are scary. Also splits the natural unit of work: closing a task that ships with code now means two commits in two places — the code change in a PR, the `closed/` move on `main`. Loses the "task close = atomic commit on the branch that did the work" property.
- Variants:
  - 1a — push state to `main` directly (least ceremony, most policy violation).
  - 1b — push to a dedicated `task-state` branch that is allowed to be force-pushed and is purely metadata. Everyone rebases their feature branch's task changes onto it before merging.

### Option 2 — Treat `OVERVIEW.md` / `EPICS.md` / `KANBAN.md` as build artifacts, not committed files

The biggest source of merge pain is the generated files. They are derived from the task files, so committing them is duplication.

- Move them out of git: add to `.gitignore`, generate on demand (`make tasks-overview`), and/or render them in CI as a Pages artifact.
- Pros: kills 90% of the merge conflicts overnight. Removes a class of "merged the PR but the dashboard doesn't reflect it because someone forgot to run housekeep" bugs.
- Cons: GitHub PR reviews lose the lovely property that you can see "this PR closes TASK-X" reflected in the OVERVIEW diff. Anyone browsing the repo on the web doesn't see the dashboard. Mitigation: render to GitHub Pages from CI on `main`, link from README.
- This is probably a no-regret move regardless of which other option we pick.

### Option 3 — ID allocation that doesn't depend on local state

Replace the "scan the working tree for the highest TASK-NNN" allocator with something that works under concurrency.

Sub-options:

- **3a — UUID-ish IDs:** use timestamp+random (`TASK-20260426-a3f`) or a short hash. No collisions, ever. But: ugly, unmemorable, breaks `/ts-task-list` aesthetics, and `TASK-260` is genuinely nicer to type and remember.
- **3b — Reserved ranges per developer:** Tobias gets 1–999, Alice gets 1000–1999. Set in `task-system.yaml`. Trivial to implement, ugly to look at, and it caps you per developer, but for 2–5 people it works fine and IDs stay short.
- **3c — Allocate at PR-merge time, not creation time:** the task file is created with a placeholder (`TASK-DRAFT-foo`) and renamed to its real `TASK-NNN` by a CI job that holds the global counter (or by a merge-queue script). Branch-local references use the slug, which is rewritten on merge.
- **3d — Central counter file with rebase discipline:** keep a `tasks/.next-id` file. Whoever wants a new ID increments it locally; on merge, conflicts on this file are resolved by "take both, increment by N." This is the simplest evolution of today's system but pushes all the conflict pain into one file that conflicts on every parallel task creation.
- **3e — Just stop using sequential IDs in filenames:** filenames become `task-<slug>.md`, the `id:` field in frontmatter is the canonical reference, and the ID can be assigned either at creation (3a/3b) or at merge (3c). Decouples filesystem layout from numbering, which removes the rename-on-collision problem entirely.

### Option 4 — Make `housekeep.py` merge-aware

Today `housekeep.py` runs as a pre-commit hook and "fixes" file locations to match `status:`. In a multi-user world it could also:

- Detect when a task file appears in two folders post-merge (because both branches moved it) and resolve to the most recently `closed:` or `opened:` date.
- Detect duplicate IDs and rename one according to a rule (smaller commit hash wins, etc.).
- Run as a *post-merge* hook on `main` to clean up any drift introduced by the merge.

Pros: incremental, doesn't change the model, just patches the failure modes.
Cons: implicit magic in a script that today is dead simple. Auto-resolution of "two people closed the same task differently" is exactly the kind of thing that should surface as a conflict, not get silently picked.

### Option 5 — Lean on GitHub: tasks become PRs / Issues, not files

Question the premise. Maybe at 2+ people the docs-as-code task system stops paying for itself, and the right move is to use GitHub Issues (or Projects) for the things that need multi-user coordination — assignment, state, "who's working on what" — and keep only the *content* (description, acceptance criteria, test plan) as files.

- Hybrid form: each task file has a `github_issue: 142` field. State lives on the Issue. The file remains the durable, version-controlled detail.
- Pros: solves multi-user perfectly because GitHub already solved it. Reuses tools the team already has open.
- Cons: a big philosophical shift away from "everything in git, no external dependencies", which is exactly what makes the current system pleasant. Also harder to extract as a standalone reusable thing (per IDEA-031) if it depends on GitHub.

### Option 6 — "Soft ownership" via the `assigned:` field + a stale-PR check

Don't try to prevent two people picking the same task technically. Instead:

- Require `assigned:` to be set when moving a task to `active`.
- A CI check on every PR scans open PRs for any `TASK-NNN` that is `active` in *another* open PR, and warns (not blocks).
- `/ts-task-active` skill could fetch open PRs (`gh pr list`) and warn if the task is already active elsewhere.

This doesn't solve ID collisions or generated-file conflicts. It only solves the "two people accidentally working on the same task" problem, and only as a soft warning. But that's actually the failure mode I worry about most for a 2-person team. The other failure modes are merge mechanics; this one is wasted human effort.

## Discussion

A few observations from looking at the options together:

- **Option 2 is almost certainly correct regardless.** Generated files in git are the worst source of friction and the easiest to fix. The cost is "the GitHub web UI no longer shows a live dashboard", which we can buy back with CI rendering to Pages.
- **The ID problem is small but visible.** Of the 3a–3e variants, 3b (reserved ranges) is the lightest and keeps IDs human. 3e (decouple filename from ID) is more invasive but eliminates a whole category of rename pain. 3c (allocate at merge) is the most "correct" but adds CI complexity that doesn't pay for itself at 2–3 people.
- **Option 1 (push to main) is tempting but probably wrong.** It buys real-time visibility at the cost of breaking the PR-only norm. For 2 people, we don't need real-time; we need "visible by tomorrow".
- **Option 5 is the bailout option.** If the multi-user friction starts dominating actual work, that's the signal to fold state management into GitHub. But starting there throws away the simplicity that makes the system worth keeping.
- **Option 4 (merge-aware housekeep) is dangerous.** Auto-resolving "two people closed this task" silently is exactly the kind of helpful-magic that bites later. Better to let the merge surface the conflict and resolve it manually — it'll happen approximately once a year per project.
- **Option 6 is the cheapest concrete improvement** and addresses the failure mode that actually wastes time. It works *with* whatever ID and state strategy we end up with, not instead of one.

## A plausible minimum package

If forced to commit to something today, the smallest combination that genuinely supports a second person without rewriting the system:

1. **Stop committing generated files** (Option 2). `OVERVIEW.md` / `EPICS.md` / `KANBAN.md` move to `.gitignore`, get rendered in CI on `main` to GitHub Pages.
2. **Reserved ID ranges per developer** (Option 3b), configured in `task-system.yaml`. Cheap, ugly-but-fine, no rename pain.
3. **Soft-ownership warning** (Option 6) in `/ts-task-active` and as a CI check.
4. **Don't change anything else.** Keep the pre-commit housekeep, keep `main` as eventually-consistent, keep state changes on feature branches, accept that "what is the project state right now" is "main + open PRs" and that's life.

What this *doesn't* address: ID-range exhaustion (irrelevant for years), people deeply caring about looking at the dashboard via the GitHub web UI (mitigated by Pages), and the genuinely-rare case of two people closing the same task on different branches (manual merge-conflict resolution, which is correct).

## Open questions

- Is the loss of "OVERVIEW.md visible in the GitHub web UI" actually painful, or does a CI-rendered Pages site fully replace it?
- Reserved ID ranges: per-person in `task-system.yaml`, or per-branch (e.g. `feature/*` branches get the 5000–5999 range)? Per-person is simpler and matches actual behaviour.
- Should `/ts-idea-new` get the same treatment? Idea numbering also collides under concurrent branches, but ideas are created less frequently and the consequence of renaming an idea ID is much smaller (no code references it).
- Does this interact with IDEA-031 (extracting the task system as a standalone repo)? Yes — multi-user support is exactly what makes the system attractive to other small teams, so the answer here directly shapes what gets extracted.
- Is there a "merge queue / labeled PR" lightweight pattern that gives us Option 3c (allocate-at-merge) without a real merge queue tool? E.g. a GitHub Action that, on merge to main, scans the diff for `TASK-DRAFT-*` files and renames them to the next free ID with a follow-up commit.
