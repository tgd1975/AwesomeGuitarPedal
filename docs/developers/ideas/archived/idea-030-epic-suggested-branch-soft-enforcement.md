---
id: IDEA-030
title: Epic-Suggested Branch with Soft Enforcement
category: tooling
description: Each epic declares a suggested branch; task/commit flows nudge (not block) the user when work on an epic happens on a different branch, to encourage smaller, focused releases.
---

## Archive Reason

2026-04-29 — Converted to EPIC-021 with TASK-316/317/318.

## Motivation

The current release demonstrates the problem clearly: **way too much is
packed into a single release**. Multiple epics, a long tail of unrelated
tasks, and incidental fixes all land on the same branch and end up
shipping together. This makes CHANGELOGs noisy, review hard, and
regressions expensive to bisect.

The root cause is workflow-level: there is nothing in the task/idea
system that gently steers work on a given epic toward its own branch.
Everything defaults to whatever branch happens to be checked out — often
`main` or a long-lived feature branch collecting many epics.

We already have a precedent for a **soft rule** in this project: the
"maybe it's wise to not use the `main` branch" nudge. It doesn't block
anything; it just asks a question. That pattern fits here well.

## Proposal

Add a **`branch:` field** (or similar — name TBD) to each epic's
frontmatter that names a _suggested_ branch for that epic's tasks.
Example:

```yaml
---
id: EPIC-006
name: LongDoublePress
title: Long-press and double-press detection
branch: feature/long-double-press
status: active
---
```

Then add soft checks at natural friction points:

1. **When a task belonging to an epic is set active** (`/ts-task-active`)
   — if the current git branch does not match the epic's suggested
   branch, print a nag:

   > "TASK-238 belongs to EPIC-006 which suggests branch
   > `feature/long-double-press`, but you are on `main`. Switch branch?
   > (y/N)"

2. **Pre-commit hook** — when staged files include tasks under an epic
   whose suggested branch differs from the current branch, emit a
   warning (not a hard fail). The existing `check-branch` skill is a
   good model: warn when committing to `main`, but allow override.

3. **`/ts-epic-new`** — ask for a suggested branch name upfront, or
   generate one from the epic slug (e.g. `feature/<epic-slug>`).

## Intermingled epics — a honest caveat

Not every epic deserves its own branch. Two cases where sharing one
branch is actually the right call:

- **Interdependent tasks.** Epic A's TASK-X cannot be tested without
  Epic B's TASK-Y being in place. Rebasing two branches to test them
  together is churn.
- **Overlapping topic.** Two epics that touch the same files, same
  subsystem, same release story. Splitting them buys nothing.

So the rule must be **soft**: a nag, not a gate. Ways to express "yes I
know, I'm doing it intentionally":

- Allow multiple branches in the `branch:` field
  (`branch: [feature/foo, feature/foo-bar]`), or a `branch_group:` that
  multiple epics share.
- Let the nag take a one-shot "dismiss for this session" response.
- A task-level override (`branch: feature/other`) wins over its epic's
  default, for the rare genuinely-cross-cutting task.

## Acceptance / success signals

- Future releases are smaller and thematically focused. A release
  typically contains 1–2 epics, not 6.
- CHANGELOG entries are easier to group by epic.
- Git log on a feature branch tells a coherent story.
- The nag fires when I drift, stays quiet when I'm aligned, and does
  not become a thing I routinely override without thinking.

## Open questions

- **Field name.** `branch:`, `suggested_branch:`, `default_branch:`?
  The word "suggested" matters — it communicates soft enforcement.
- **Where to hook the nag.** `/ts-task-active` is the obvious place, but
  is it enough? A commit-time check catches work that bypasses the task
  lifecycle entirely, which is more robust but more intrusive.
- **Multiple-epic branches.** Is a shared `branch_group:` field worth
  the complexity, or is it simpler to just list multiple branches in
  `branch:` and deduplicate?
- **Branch creation.** Should the nag offer to `git checkout -b` the
  suggested branch automatically, or just point the way?
- **Backfill.** Existing epics have no `branch:` field. Do we backfill
  all of them, only active/open ones, or treat the field as opt-in
  going forward?
- **Interaction with `check-branch`.** The existing skill warns on
  `main`. The new nag should compose with it, not duplicate the
  warning.

## Out of scope

- **Hard enforcement.** No CI failures, no pre-push hard stops. This is
  a discipline aid, not a policy enforcer.
- **Release automation.** `/release-branch` already handles the
  squash/merge flow — no change needed there.
