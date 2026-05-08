# `paused` as a first-class task status

> Sub-note of [IDEA-043](idea-043-coordinated-task-system-rollout.md)

## Archive Reason

2026-04-28 — Implemented as part of IDEA-043 rollout. `docs/developers/tasks/paused/` folder live with 4 tasks; OVERVIEW.md header shows `🟡 Paused: N`; KANBAN.md has a Paused lane; `STATUS_ICON["paused"] = "🟡"` in housekeep.py; `/ts-task-pause` and `/ts-task-active` handle the transitions.

## Motivation

Today `/ts-task-pause` keeps the task in `active/` and encodes the pause via `prerequisites:` plus a `## Paused` body section. The status field stays `active`. This was a pragmatic choice — it preserves the "we already started this" signal that moving back to `open/` would erase — but it overloads `active` with two very different meanings:

- *Actually being worked on right now.*
- *Started, then parked indefinitely waiting on something else.*

Indexes (OVERVIEW, KANBAN, EPICS) can't tell the two apart without parsing body sections. A glance at "active tasks" misleads: TASK-153 has been paused multiple times in the last week, but it counts the same as a task someone is actively touching today.

## Approach

Add `paused` as a fourth status alongside `open` / `active` / `closed`, with matching mechanics:

```yaml
status: paused
```

Filesystem layout: a new `docs/developers/tasks/paused/` folder. `/ts-task-pause` moves the file from `active/` to `paused/` and sets `status: paused`.

**The status field is authoritative.** Folder + frontmatter `status:` is the single source of truth for *whether* a task is paused. The `## Paused` body section becomes legacy — kept as good practice for documenting *why* a task is paused (human-readable context), but tooling and indexes must not consult it to determine state. If body and status disagree, status wins; the body section is treated as stale documentation.

Resuming: `/ts-task-active` already exists and "resumes paused tasks by clearing closed prerequisites" per its description. Extend it to also flip `status: paused` → `status: active` and `git mv` from `paused/` back to `active/`.

Index changes:

- `OVERVIEW.md` gains a "Paused Tasks" section between Active and Open.
- `KANBAN.md` gains a Paused lane.
- `EPICS.md` shows paused-count alongside active/open/closed in epic summaries.
- The header counts (`Active: 5`, `Open: 23`, etc.) gain a `Paused: N` field.

### Decisions

- **Reopen lands in `open`.** Reopening a previously-closed task always lands in `open`. A reopened-then-paused task is two transitions; conflating them muddies the audit trail.
- **Wontfix is `closed`.** No third terminal state. A paused task that is abandoned closes normally with a note in the body. "Not now, maybe never" stays `paused` until eventually closed.
- **Release policy: paused inherits open/active behavior.** Releases today don't gate on open/active task counts, so paused inherits the same: don't block, don't appear as shipped, carry into the next cycle. If gating is ever introduced, paused gates the same as open/active.
- **Migration: skip.** Tasks paused under the old model stay in `active/` and will be (correctly) counted as active until they are touched again — at which point they get re-paused under the new mechanics or progressed normally. Worst case: a small, decaying overcount of active tasks that resolves itself over time.
- **Per-epic progress-bar denominator includes paused.** Paused tasks are still in scope; the burn-up should reflect that. Excluding them would make the bar drift as tasks pause/resume.

## Why this earns its complexity

Three reasons the overload hurts in practice:

1. **Honest dashboards.** "5 active tasks" should mean *5 tasks being worked on*, not "5 active + 3 parked-but-still-shelved-as-active." The number is a planning signal; making it accurate is cheap.
2. **Preserves the original "we already started this" signal.** That's why `/ts-task-pause` exists in the first place. `paused` keeps the signal *and* makes it queryable. Today the signal is buried in body markdown.
3. **Pairs with [IDEA-043-release-burnup-chart](idea-043.release-burnup-chart.md).** A burn-up chart that lumps paused tasks in with active ones overstates progress. If the chart filters on `status: active`, paused tasks correctly fall out without special-casing.

## Impact surface

A walk through the codebase shows the work is bigger than just adding a folder, but smaller than it first looks because some plumbing is already half-in.

### Already done

- [scripts/housekeep.py](../../../../scripts/housekeep.py) already maps `paused` to a yellow icon (`🟡`) in `STATUS_ICON` and renders a `**paused**` label. It just doesn't scan a `paused/` folder yet — `TASK_STATUS_FOLDERS` is hardcoded to `("open", "active", "closed")`.
- [.claude/skills/ts-task-pause/SKILL.md](../../../../.claude/skills/ts-task-pause/SKILL.md) exists and does the right thing under the current model — it just leaves the file in `active/` instead of moving it.

### Skills that move task files or set `status:` (primary surface)

| Skill | Change |
|---|---|
| [ts-task-pause](../../../../.claude/skills/ts-task-pause/SKILL.md) | Set `status: paused` and `git mv active/ → paused/` instead of leaving in `active/`. |
| [ts-task-active](../../../../.claude/skills/ts-task-active/SKILL.md) | Already says it resumes paused tasks; extend to flip `paused → active` and `git mv paused/ → active/`. |
| [ts-task-done](../../../../.claude/skills/ts-task-done/SKILL.md) | Allow closing from `paused`, not only from `active`. |
| [ts-task-reopen](../../../../.claude/skills/ts-task-reopen/SKILL.md) | Reopen always lands in `open`. A reopened-then-paused task is two transitions; conflating them muddies the audit trail this idea is trying to clean up. |
| [ts-task-list](../../../../.claude/skills/ts-task-list/SKILL.md) | Default-hide paused; one flag away (`--paused` / `--all`). |

### Skills with secondary references

| Skill | Change |
|---|---|
| [tasks](../../../../.claude/skills/tasks/SKILL.md) | "Show all open tasks" — decide whether paused is included. |
| [ts-epic-list](../../../../.claude/skills/ts-epic-list/SKILL.md) | Add paused to the per-epic status breakdown. |
| [release](../../../../.claude/skills/release/SKILL.md) | Paused tasks behave like open and active tasks for release purposes. Releases today don't gate on open/active task counts, so paused inherits the same: don't block, don't appear in the release as shipped, carry into the next cycle. If we ever decide to gate releases on open/active, paused gates the same way. |
| [ts-task-new](../../../../.claude/skills/ts-task-new/SKILL.md), [ts-epic-new](../../../../.claude/skills/ts-epic-new/SKILL.md) | Likely no change — new tasks/epics are still created `open`. |

### Scripts

| Script | Change |
|---|---|
| [scripts/housekeep.py](../../../../scripts/housekeep.py) | Add `"paused"` to `TASK_STATUS_FOLDERS`; extend the status→folder mapping. |
| [scripts/update_task_overview.py](../../../../scripts/update_task_overview.py) | Biggest single diff. Currently hardcodes `OPEN_DIR / ACTIVE_DIR / CLOSED_DIR`, status counts, and the `🔵 active` / `⚪ open` badges. Add `PAUSED_DIR`, count, badge, section. |
| [scripts/tests/test_housekeep.py](../../../../scripts/tests/test_housekeep.py) | New cases for the paused folder and status. |

### Configuration

The same opt-out pattern that exists for `active` applies to `paused`, with one rule: **paused depends on active**. A project that has disabled the `active/` folder cannot meaningfully have a `paused/` folder either — pausing is a transition out of "currently being worked on," and that concept doesn't exist without active.

Effective state truth table:

| `active.enabled` | `paused.enabled` (configured) | Effective `paused` |
|---|---|---|
| `true`  | `true`  | enabled |
| `true`  | `false` | disabled |
| `false` | `true`  | **disabled** (forced off — active is gone) |
| `false` | `false` | disabled |

When `paused` is effectively off:

- `housekeep.py` does not scan or create `paused/`.
- `/ts-task-pause` becomes a no-op (or refuses, with a message pointing at the config).
- Indexes do not render a paused section / lane / column.
- The status enum effectively collapses back to `open` / `active` / `closed` (or `open` / `closed` if active is also off).

| File | Change |
|---|---|
| [docs/developers/task-system.yaml](../../task-system.yaml) | Add `paused: { enabled: true }` under `tasks:`, with a comment explaining the dependency on `active.enabled` and that the effective value is `active.enabled AND paused.enabled`. |
| [scripts/task_system_config.py](../../../../scripts/task_system_config.py) | `DEFAULTS["tasks"]` gains `"paused": {"enabled": True}`. Add a small helper (e.g. `paused_enabled(cfg)`) that returns the effective value (`active AND paused`) so individual scripts and skills don't reimplement the dependency check. |

### Generated indexes (regenerated, not edited by hand)

| File | Change |
|---|---|
| [OVERVIEW.md](../../tasks/OVERVIEW.md) | New "Paused Tasks" section between Active and Open; counts header gains `Paused: N`; per-epic progress-bar denominator includes paused. |
| [KANBAN.md](../../tasks/KANBAN.md) | New Paused lane. |
| [EPICS.md](../../tasks/EPICS.md) | Per-epic paused count. |

### Documentation

| File | Change |
|---|---|
| [awesome-task-system/TASK_SYSTEM.md](../../../../awesome-task-system/TASK_SYSTEM.md) | Canonical description of statuses and folders — must document `paused`. |
| [awesome-task-system/LAYOUT.md](../../../../awesome-task-system/LAYOUT.md) | Folder-tree doc — add `paused/`. |
| [docs/developers/CONTRIBUTING.md](../../CONTRIBUTING.md) (if it covers the task workflow) | Mention the new state. |

### The big surprise: `awesome-task-system/` is a separate package

The repo ships a sibling [awesome-task-system/](../../../../awesome-task-system/) directory that contains **its own copies** of the scripts and skills above:

- [awesome-task-system/scripts/housekeep.py](../../../../awesome-task-system/scripts/housekeep.py)
- [awesome-task-system/scripts/update_task_overview.py](../../../../awesome-task-system/scripts/update_task_overview.py)
- [awesome-task-system/scripts/task_system_config.py](../../../../awesome-task-system/scripts/task_system_config.py)
- [awesome-task-system/scripts/update_idea_overview.py](../../../../awesome-task-system/scripts/update_idea_overview.py)
- 11 mirrored skill folders under [awesome-task-system/skills/](../../../../awesome-task-system/skills/)

**Every change above must be applied twice** — once in the project, once in `awesome-task-system/` — *or* this task should also resolve the source-of-truth question (e.g. project copies are generated from the package, or vice versa). Easy to forget; effectively doubles the diff if not addressed.

### Cost summary

Realistic estimate: **half a day to a full day**, depending on how the `awesome-task-system/` duplication is handled.

- Mechanical work (scripts + indexes + folder, no migration): ~2–3 hours.
- Skills (six edited, all small): ~1 hour.
- Tests (`scripts/tests/test_housekeep.py` + new cases for `update_task_overview.py`): ~1 hour.
- Mirroring into `awesome-task-system/`: ~1–2 hours, *or* a separate prerequisite task to consolidate the duplication.
- Index regeneration + verification across OVERVIEW / KANBAN / EPICS: ~30 min.
- Remaining decisions: `/ts-task-list` defaults, epic-rollup behavior.

## Open questions

- **Per-epic paused state.** An epic with all sub-tasks paused is itself paused. Worth surfacing in `EPICS.md`, or is the sub-task view enough? Probably enough — don't propagate.
- **`/ts-task-list` visibility.** Should the default list show paused tasks? Probably no (matches the "what should I work on?" intent), but they should be one flag away (`--paused` or `--all`).

## Non-goals

- No new lifecycle transitions beyond pause/resume. Don't invent `blocked`, `parked`, `deferred` etc. as further statuses — `paused` covers them, and the body's prerequisites + reason captures the nuance. (`wontfix` is `closed`, not paused.)
- No GitHub Projects-style automation. The folders and frontmatter remain the source of truth; tooling (housekeep, indexes) reads them.
