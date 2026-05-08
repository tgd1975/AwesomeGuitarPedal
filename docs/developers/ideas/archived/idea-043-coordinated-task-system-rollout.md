---
id: IDEA-043
title: Coordinated task-system rollout — burn-up dashboard, post-hoc effort, paused status, and awesome-task-system carve-out
category: tooling
description: Sequenced rollout for the burn-up dashboard, post-hoc effort reassessment, paused-as-first-class status, and the awesome-task-system source-of-truth carve-out.
related: IDEA-031
---

## Archive Reason

2026-04-28 — All four sub-pieces shipped: TASK-PRE single-source-of-truth carve-out (sync_task_system.py + divergence guard live), paused-as-first-class status (paused/ folder + lane), effort_actual on close (frontmatter on closed tasks), release burn-up block in OVERVIEW.md (scripts/release_burnup.py).

A single coordinated rollout for three interlocking sub-ideas plus one prerequisite that surfaced from reviewing them together.

## Sub-pages

- [IDEA-043-release-burnup-chart](idea-043.release-burnup-chart.md) — auto-generated burn-up section in `tasks/OVERVIEW.md`, the dashboard.
- [IDEA-043-llm-effort-reassessment-on-close](idea-043.llm-effort-reassessment-on-close.md) — `effort_actual` written by `/ts-task-done`, the data the dashboard needs.
- [IDEA-043-paused-as-first-class-task-status](idea-043.paused-as-first-class-task-status.md) — `paused/` folder and status, cleans up the lifecycle the dashboard summarizes.

Plus one prerequisite, scoped here:

- **TASK-PRE** — Make `awesome-task-system/` the single source of truth for scripts and skills (see §2 below). Pairs with [IDEA-031](idea-031-extract-task-system-standalone-repo.md), which is the eventual carve-out.

## 1. Dependency graph

```
TASK-PRE (carve-out source-of-truth)
    │
    ├── paused-as-first-class-task-status
    │       │
    │       └── needed by release-burnup-chart (so the burn-up filters paused out)
    │
    ├── llm-effort-reassessment-on-close
    │       │
    │       └── needed by release-burnup-chart (the actual-hours line)
    │
    └── release-burnup-chart — lands last
```

Why this order:

1. **TASK-PRE first.** Otherwise every change has to be applied twice (live + package), and the two sides are already drifting (`housekeep.py`, `update_task_overview.py`, `ts-task-pause`, `ts-task-active` all differ today). With the package marked for carve-out, the right move is to flip the model now and avoid paying the doubling cost three times in a row.
2. **paused-as-first-class before release-burnup-chart.** The burn-up filters on `status: active` (or "anything not closed since the tag"). With paused as a first-class status, paused tasks correctly fall out without special-casing the script. Landing the burn-up first would mean writing filter logic that becomes obsolete the moment paused lands.
3. **llm-effort-reassessment-on-close before release-burnup-chart.** The actuals line in the chart is only meaningful once `effort_actual` exists. Landing the burn-up first ships a chart with one of its two lines empty, undermining the synthetic example that motivates the design.
4. **The two non-burn-up sub-ideas are independent of each other** — could land in either order, or in parallel, once TASK-PRE is done.

---

## 2. TASK-PRE — Single source of truth for `awesome-task-system/`

### Premise

`awesome-task-system/` is destined to be carved out as a self-sufficient package (see [IDEA-031](idea-031-extract-task-system-standalone-repo.md)). It must therefore be the canonical source. Today the live project copies (`scripts/`, `.claude/skills/`) are ahead of the package — the package is stale, not customized. Actual drift confirmed by `diff -rq` on 2026-04-26:

- `scripts/housekeep.py` — live copy has `STATUS_ICON` (incl. `paused: 🟡`), `_progress_bar`, `_status_badge`, `_graph_direction` auto-layout. Package copy has none of these.
- `scripts/update_task_overview.py` — diverged.
- `.claude/skills/ts-task-pause/SKILL.md` — live copy implements the current "stay in active, record prerequisites" model. Package copy reflects an older "back to open/" model.
- `.claude/skills/ts-task-active/SKILL.md` — diverged.
- `.claude/skills/ts-task-new/SKILL.md` — diverged.
- `.claude/skills/ts-epic-new/SKILL.md` — diverged.

Files that are **already byte-identical** (no copy needed):

- `scripts/task_system_config.py`, `scripts/update_idea_overview.py`
- `.claude/skills/ts-task-done`, `ts-task-list`, `ts-task-reopen`
- `.claude/skills/ts-epic-list`
- All three `.claude/skills/ts-idea-*` skills

So the actual divergence is: 2 scripts + 5 skill SKILL.md files. Smaller than first thought, but TASK-PRE is still required to lock the relationship before the other sub-ideas land.

### Approach

**Step 1 — Bring the package up to date (one-shot).**

Copy live → package only for the diverged pairs, then verify with `diff -rq` that they are identical:

| Live | Package |
|---|---|
| `scripts/housekeep.py` | `awesome-task-system/scripts/housekeep.py` |
| `scripts/update_task_overview.py` | `awesome-task-system/scripts/update_task_overview.py` |
| `.claude/skills/ts-task-pause/SKILL.md` | `awesome-task-system/skills/ts-task-pause/SKILL.md` |
| `.claude/skills/ts-task-active/SKILL.md` | `awesome-task-system/skills/ts-task-active/SKILL.md` |
| `.claude/skills/ts-task-new/SKILL.md` | `awesome-task-system/skills/ts-task-new/SKILL.md` |
| `.claude/skills/ts-epic-new/SKILL.md` | `awesome-task-system/skills/ts-epic-new/SKILL.md` |
| `.claude/skills/tasks/SKILL.md` | (decide: package adds it, or skip — see below) |

Files already in sync (skip): `scripts/task_system_config.py`, `scripts/update_idea_overview.py`, all three `ts-idea-*` skills, plus `ts-task-done`, `ts-task-list`, `ts-task-reopen`, `ts-epic-list`. Final divergence-guard sweep should still verify all of these are byte-identical.

**Tests directory.** `scripts/tests/test_housekeep.py` exists live; `awesome-task-system/scripts/tests/` does not. Recommendation: **mirror tests into the package** (a carve-out package without its own tests is incomplete). Add `awesome-task-system/scripts/tests/` and extend the divergence guard's scope to cover it. Confirm before scaffolding (open question 6).

**Config file.** `awesome-task-system/config/task-system.yaml` and `docs/developers/task-system.yaml` are byte-identical today. The truth-table change in §3 lands in the package copy first, then syncs. The divergence guard must include this pair too.

Note: `tasks/SKILL.md` exists live but not in the package. Decide: include in the package (preferred — it's a task-system skill) or leave out (if it's project-specific). Leaning include.

Bump `awesome-task-system/VERSION` from `0.1.0` to `0.2.0`. Note the consolidation in `awesome-task-system/CHANGELOG.md` (create if absent).

**Step 2 — Replace the live copies with thin pointers.**

Two options. Pick one based on a 30-minute Windows test:

| Option | Pros | Cons |
|---|---|---|
| **Symlinks** | Zero sync cost. `git` supports them. Edits in either location update both (because they *are* the same file). | Windows + Git Bash needs `core.symlinks=true` and the user creating the symlinks needs Developer Mode or admin. Fragile if a checkout happens outside Git Bash (e.g. VS Code's bundled git on Windows). |
| **Sync script** (`scripts/sync_task_system.py`) | OS-independent. Explicit. | Two-step edit-and-sync flow. Risk of editing the live copy and forgetting to sync back. Mitigated by (a) divergence guard hook and (b) script that only ever copies package → live, never the other direction. |

**Recommendation: sync script.** This project explicitly runs on Windows 11 + Ubuntu (per CLAUDE.md), and "edit in one place, sync forwards" is easier to enforce than "symlinks must work in three different git clients on two OSes." The cost is one small Python script and an addition to the existing pre-commit hook.

`scripts/sync_task_system.py` (avoiding name-collision with the existing [scripts/sync_skills_config.py](../../../scripts/sync_skills_config.py), which has an unrelated purpose — it syncs `enabled_skills` in `.vibe/config.toml`):

- Walks `awesome-task-system/scripts/*.py` → copies to `scripts/`.
- Walks `awesome-task-system/scripts/tests/*.py` → copies to `scripts/tests/` (assuming open question 6 resolves "mirror").
- Walks `awesome-task-system/skills/*` → copies to `.claude/skills/`.
- Walks `awesome-task-system/config/task-system.yaml` → copies to `docs/developers/task-system.yaml`.
- Idempotent. Prints a one-line "synced N files" or "already in sync."
- Run it once after step 1 to bring the live copies back in lockstep.

**Step 3 — Divergence guard.**

Add a check to the existing [scripts/pre-commit](../../../scripts/pre-commit) shell script (do **not** replace it — it already runs clang-format, tests, and other checks). Call out to a small Python check that, for every mirrored pair, `diff`s them. If any pair differs, fail the commit with a message:

> `scripts/housekeep.py` differs from `awesome-task-system/scripts/housekeep.py`.
> Edit the package copy, then run `python scripts/sync_task_system.py`.

The guard's mirror set must include the scripts, the skill SKILL.md files, the `config/task-system.yaml` pair, and (per open question 6) the `scripts/tests/` directory. Files that exist only on one side count as divergence — except for the explicit allowlist of project-only files. Concretely, the guard's scope is the canonical task-system set: the scripts listed in the mirror table above, every `ts-*` skill, the `tasks/` skill (if open question 2 resolves "include"), `config/task-system.yaml`, and `scripts/tests/`. Everything else under `scripts/` and `.claude/skills/` is project-local and ignored by the guard.

Cheap insurance. Catches the "I edited the live copy out of habit" failure mode at commit time, not three weeks later.

**Step 4 — Update docs.**

- `awesome-task-system/LAYOUT.md` — note that the package is canonical and the project consumes it via `scripts/sync_task_system.py`.
- `awesome-task-system/TASK_SYSTEM.md` — same note.
- `CLAUDE.md` (project root) — add a short section: "When editing task-system scripts or skills, edit the copy under `awesome-task-system/` and run `python scripts/sync_task_system.py`. The pre-commit hook will catch you if you forget."

### Acceptance criteria

- [ ] All mirrored files are byte-identical between live and package (`diff -rq` clean).
- [ ] `scripts/sync_task_system.py` exists, is idempotent, and exits 0 when in sync.
- [ ] Divergence-guard step in `scripts/pre-commit` fails on a deliberate divergence (test: edit one live copy, try to commit, observe the failure).
- [ ] Mirror set includes scripts, `ts-*` skills, `tasks/SKILL.md` (if open question 2 resolves "include"), `config/task-system.yaml`, and `scripts/tests/` (if open question 6 resolves "mirror").
- [ ] `awesome-task-system/VERSION` bumped to `0.2.0`.
- [ ] Docs updated.

### Estimated effort

**L (8–24h)**, on the lower end. The copy is mechanical, but the divergence-guard plumbing (path handling, allowlist of project-only files, integration with the existing `scripts/pre-commit`, fail-mode test) plus docs across 3+ files is realistically a full day. Could land as M if the guard is kept minimal — but minimal-guard means accepting the "I forgot to sync" failure mode silently, which defeats the point.

### Risks

- **Sync script accidentally clobbers an in-flight live edit.** Mitigation: the script reads the package and writes the live; if the live is dirty in `git`, refuse to overwrite without `--force`.
- **Future skills get added to one side and not the other.** Mitigation: the divergence guard treats "file exists in live but not package" (or vice versa) as a divergence and fails.

---

## 3. paused-as-first-class-task-status — implementation notes

Full motivation and design: [IDEA-043-paused-as-first-class-task-status](idea-043.paused-as-first-class-task-status.md). Notes here are the additional refinements that came out of reviewing the three sub-ideas together.

### Premise

After TASK-PRE, every change here is made **once** in `awesome-task-system/` and then synced. The "every change must be applied twice" sentence in the original idea body disappears.

### Lifecycle (corrected)

```
open ──activate──> active ──pause──> paused
                      │                │
                      │                └── continue ──> active
                      │                │
                      │                └── done ──> closed
                      │
                      └── done ──> closed

closed ──reopen──> open  (only when explicitly demanded)

active/paused ──> open  (escape hatch — only on explicit user request,
                         e.g. "I activated this by mistake")
```

Verb mapping:

| Verb / skill | From | To |
|---|---|---|
| `ts-task-active` (activate) | `open` | `active` |
| `ts-task-pause` | `active` | `paused` |
| `ts-task-active` (continue) | `paused` | `active` |
| `ts-task-done` | `active` or `paused` | `closed` |
| `ts-task-reopen` | `closed` | `open` |
| (escape hatch) | `active` / `paused` → `open` | manual `git mv` + frontmatter edit |

The escape hatch is rare enough that it does not deserve its own skill. Document it in `TASK_SYSTEM.md` as the exception path; do not add `--to-open` to any existing skill.

### Concrete changes (all in `awesome-task-system/`, then synced)

**Filesystem.**

- New folder: `docs/developers/tasks/paused/`. Created on demand by housekeep when the first task transitions in.

**Scripts.**

| File | Change |
|---|---|
| `scripts/housekeep.py` | Add `"paused"` to `TASK_STATUS_FOLDERS`. Extend status→folder mapping. Pre-existing `STATUS_ICON["paused"] = "🟡"` and `_status_badge` already render the label, so most rendering is free. |
| `scripts/update_task_overview.py` | Biggest single diff. Add `PAUSED_DIR`, paused count, paused badge, paused section. Per-epic progress-bar denominator includes paused. |
| `scripts/task_system_config.py` | `DEFAULTS["tasks"]` gains `"paused": {"enabled": True}`. Add `paused_enabled(cfg)` helper returning `active.enabled AND paused.enabled` (paused depends on active per the truth table). |
| `scripts/tests/test_housekeep.py` | New cases: paused folder is scanned; paused → active transition moves the file; paused-disabled config collapses the status. |

**Skills.**

| Skill | Change |
|---|---|
| `ts-task-pause` | Set `status: paused`. `git mv active/ → paused/`. Keep the `## Paused` body section as documentation but make it optional — status is authoritative. |
| `ts-task-active` | Add the `paused → active` transition: flip status, `git mv paused/ → active/`. The existing `open → active` path is unchanged. |
| `ts-task-done` | Allow closing from `paused` in addition to `active`. |
| `ts-task-reopen` | No change. Reopen lands in `open` regardless of prior state, and only operates on `closed` tasks. |
| `ts-task-list` | Default-hide paused. Add `--paused` and `--all` flags. |
| `tasks` | Decide: include paused in the default "show all open tasks" view? Recommend **no** (matches the "what should I work on?" intent), with a flag for explicit inclusion. |
| `ts-epic-list` | Add paused count to the per-epic status breakdown. |
| `release` | No functional change — paused inherits open/active behavior (don't block, don't appear shipped, carry forward). Document this explicitly in the skill body. |
| `ts-task-new`, `ts-epic-new` | No change — new items are still created `open`. |

**Configuration.**

Edit the canonical `awesome-task-system/config/task-system.yaml` (synced to `docs/developers/task-system.yaml` by `scripts/sync_task_system.py`):

```yaml
tasks:
  enabled: true
  base_folder: docs/developers/tasks
  active:
    enabled: true
  paused:
    enabled: true  # Effective value is active.enabled AND paused.enabled.
  epics:
    enabled: true
  releases:
    enabled: true
```

Truth table:

| `active.enabled` | `paused.enabled` | Effective `paused` |
|---|---|---|
| `true`  | `true`  | enabled |
| `true`  | `false` | disabled |
| `false` | `true`  | **disabled** (forced off — active is gone) |
| `false` | `false` | disabled |

**Generated indexes.**

| File | Change |
|---|---|
| `OVERVIEW.md` | New "Paused Tasks" section between Active and Open. Counts header gains `Paused: N`. Per-epic progress-bar denominator includes paused. |
| `KANBAN.md` | New Paused lane (between Active and Closed). |
| `EPICS.md` | Per-epic paused count alongside active/open/closed. |

**Documentation.**

| File | Change |
|---|---|
| `awesome-task-system/TASK_SYSTEM.md` | Document `paused` as a status. Document the escape-hatch (`active`/`paused` → `open` is manual, rare). |
| `awesome-task-system/LAYOUT.md` | Add `paused/` to the folder tree. |
| `docs/developers/CONTRIBUTING.md` | Mention paused if it covers the task workflow. |

### Migration

**Skip.** Confirmed today: 4 tasks (`task-153`, `task-154`, `task-155`, `task-156`) are paused under the old model (`## Paused` body section, still in `active/`). They will continue to count as active until each is touched, at which point they get re-paused under the new mechanics or progress normally. Worst-case overcount: 4 tasks, decaying to zero over weeks.

If the overcount becomes annoying before it self-resolves: a one-shot script could grep `## Paused` from `active/*.md` and do the re-pause, but **don't bundle it into this task** — keep the task focused on the new mechanics.

### Acceptance criteria

- [ ] `paused/` folder exists and is scanned by housekeep.
- [ ] `/ts-task-pause` moves the file to `paused/` and sets `status: paused`.
- [ ] `/ts-task-active` resumes from `paused/` back to `active/`.
- [ ] `/ts-task-done` can close a paused task without intermediate transition.
- [ ] OVERVIEW / KANBAN / EPICS render the new state correctly.
- [ ] `task-system.yaml` opt-out works (paused-disabled collapses the status).
- [ ] Tests pass: `scripts/tests/test_housekeep.py` covers the new transitions and the disabled-config case.
- [ ] All changes synced from `awesome-task-system/` to live; divergence guard clean.

### Estimated effort

**L (8–24h)**. Realistic is closer to a full day, possibly more if `update_task_overview.py` turns out trickier than expected. With TASK-PRE done first, no doubling cost — the L estimate assumes single-source.

### Open questions to resolve before scaffolding

1. `tasks` skill — include paused in default view, or hide?
2. Per-epic paused state in `EPICS.md` — propagate (epic with all-paused subtasks shows as paused) or just count? Sub-idea body leans "just count, don't propagate." Probably correct.
3. Migration script for the 4 old-model tasks — yes, no, or "wait and see"? Recommend wait-and-see.

---

## 4. llm-effort-reassessment-on-close — implementation notes

Full motivation and design: [IDEA-043-llm-effort-reassessment-on-close](idea-043.llm-effort-reassessment-on-close.md). Notes here are the additional refinements.

### Premise

Lands after paused-as-first-class (so the canonical vocabulary from release-burnup-chart is settled — see below) but before release-burnup-chart (so the actuals line in the burn-up has data).

### Vocabulary reconciliation

The original `llm-effort-reassessment-on-close` body lists five sizes (`Trivial`, `Small (<2h)`, `Medium (2-8h)`, `Large (8-24h)`, `Extra Large (24-40h)`).
The `release-burnup-chart` body lists six canonical sizes (`XS`, `S`, `M`, `L`, `XL`, `XXL`).

**Resolution: the six-size set wins.** It's the dashboard's normalization point and explicitly intended as canonical going forward. The `llm-effort-reassessment-on-close` sub-idea should be updated to reference the canonical six by name, not redefine them.

Concretely, the canonical strings are:

| Size | Label |
|---|---|
| XS  | `XS (<30m)` |
| S   | `Small (<2h)` |
| M   | `Medium (2-8h)` |
| L   | `Large (8-24h)` |
| XL  | `Extra Large (24-40h)` |
| XXL | `XXL (>40h)` |

`/ts-task-new`, `/ts-task-done`, `/ts-epic-new`, and `/ts-task-active` (if it ever writes effort, which it doesn't today) all write only these strings. Legacy variants in already-closed tasks are read with normalization but never written.

### Concrete changes

**Skill: `ts-task-done`.**

Add a step between current steps 2 and 3:

> **2a.** Determine `effort_actual` and write it to frontmatter immediately after `effort:`.
> If the user passed `--effort-actual SIZE`, use that. Otherwise pick a t-shirt size from the work product: diff size, files touched, commits between `opened:` and today, pause/blocker history.
>
> **No-peek rule:** when judging from the work product, do **not** read the original `effort:` value first. Pick the actual, then reveal the original. The same agent estimating both ends has a regression-toward-"about right" bias; the chart only earns its keep if the gap is honest.

**Skill: `ts-task-new`.**

Update the `--effort` flag's accepted values to the canonical six. Update the prose to reference the canonical vocabulary table. Document that legacy values are tolerated on read but never written.

**Skill: `ts-epic-new`.**

Same vocabulary update as `ts-task-new` (epics carry effort estimates today even though the burn-up ignores them — they're aggregated from children).

**Frontmatter.**

```yaml
effort: Small (<2h)            # original estimate, never edited after creation
effort_actual: Medium (2-8h)   # post-hoc t-shirt size, written by ts-task-done
```

Both stay forever. The comparison is the point.

### No-peek enforcement

The rule is honor-system as written — the agent can read frontmatter at any time. Two options:

1. **Accept the honor system.** Document it clearly and trust the agent.
2. **Sequence the read in the skill.** Have `ts-task-done` instruct the agent to read only the body and diff first, write `effort_actual`, *then* read the rest of frontmatter. This is procedural, not enforced, but it nudges the right flow.

**Recommendation: option 2.** Costs nothing and shapes the behavior. Note in the skill that this is a behavioral nudge, not a guarantee.

### Acceptance criteria

- [ ] `ts-task-done` writes `effort_actual` to frontmatter immediately after `effort:`.
- [ ] `--effort-actual SIZE` flag accepted; sizes match the canonical six.
- [ ] No-peek sequencing is documented in the skill.
- [ ] `ts-task-new` and `ts-epic-new` write only the canonical six labels.
- [ ] Legacy labels in already-closed tasks remain readable (round-trip test in housekeep / overview generation).
- [ ] All changes synced from `awesome-task-system/` to live; divergence guard clean.

### Non-goals

- No retroactive backfill on already-closed tasks.
- No re-fill on later passes — if `effort_actual` stays blank, it stays blank forever.
- No replacing `effort:` with `effort_actual`.

### Estimated effort

**S (<2h)**. One skill edit (well-scoped), two minor skill updates for vocabulary, no script changes, no index changes.

---

## 5. release-burnup-chart — implementation notes

Full motivation and design: [IDEA-043-release-burnup-chart](idea-043.release-burnup-chart.md). Notes here cover the implementation specifics that emerged from the joint review.

### Premise

Lands last. By this point: paused is filtered out cleanly, `effort_actual` is being written on close, and the canonical vocabulary is settled.

### Concrete changes

**New script: `scripts/release_burnup.py` (in `awesome-task-system/scripts/`).**

Pseudocode:

```python
def main():
    tag = git("describe", "--tags", "--abbrev=0").strip()
    closed_files = parse_git_log_for_closures(tag)
    closures = []
    for path, date in closed_files:
        fm = parse_frontmatter(path)
        closures.append({
            "path": path,
            "date": date,
            "kind": "epic" if path.startswith("epic-") else "task",
            "effort_h": tshirt_to_hours(fm.get("effort")),
            "effort_actual_h": tshirt_to_hours(fm.get("effort_actual")),
        })
    closures = dedupe_by_basename_keep_latest(closures)
    series = aggregate_by_day(closures)
    block = render_burnup_block(series, tag)
    update_overview_section(block)  # byte-compare; only mutate if different

def tshirt_to_hours(label):
    return CANONICAL_MIDPOINTS.get(label) or LEGACY_MIDPOINTS.get(label)
```

`CANONICAL_MIDPOINTS`:

| Label | Hours |
|---|---|
| `XS (<30m)` | 0.25 |
| `Small (<2h)` | 1 |
| `Medium (2-8h)` | 5 |
| `Large (8-24h)` | 16 |
| `Extra Large (24-40h)` | 32 |
| `XXL (>40h)` | 56 (convention, not a true midpoint) |

`LEGACY_MIDPOINTS` (read-only, fades over time):

| Legacy label | Maps to |
|---|---|
| `Trivial (<30m)` | XS → 0.25 |
| `Small (1-2h)`, `Small (1-3h)`, `Small (2-4h)` | S → 1 |
| `Large (>8h)` | L → 16 |

**Git-log parser.**

```bash
git log <tag>..HEAD --diff-filter=A --no-renames --name-only \
  --date=short --pretty=format:'COMMIT %ad' \
  -- 'docs/developers/tasks/closed/task-*.md' \
     'docs/developers/tasks/closed/epic-*.md'
```

Stream parser: each `COMMIT <date>` sentinel sets the current commit date; every following non-empty path until the next sentinel is a closure on that date. Dedupe by basename, keeping the latest closure date per task (handles `open → active → closed → reopened → closed` round-trips).

**Pathspec note.** The pathspec restricts to `closed/task-*.md` and `closed/epic-*.md`. After paused-as-first-class lands, paused tasks live in `paused/` not `closed/`, so they correctly do not appear in the burn-up. No special-casing needed — exactly the property that motivated landing paused first.

**Integration with `housekeep.py`.**

`release_burnup.py` exposes a `render_block()` function. `housekeep.py` calls it during OVERVIEW.md regeneration, between the existing `<!-- GENERATED -->` block and the per-epic sections. The block is delimited:

```markdown
<!-- BURNUP:START -->
...generated content...
<!-- BURNUP:END -->
```

**Idempotency.** Build the new block in memory. Compare byte-for-byte against the current `BURNUP:START`/`BURNUP:END` slice in OVERVIEW.md. Only mutate the file if they differ. This keeps `housekeep --apply` a no-op on days with no closures.

**Rendered content.**

1. Summary table (date / tasks closed / cum. tasks / est. h / cum. est. h / actual h / cum. actual h / epics closed / cum. epics).
2. Two side-by-side `xychart-beta` blocks: cumulative tasks, cumulative epics. Two charts because `xychart-beta` has no secondary y-axis.
3. One `xychart-beta` block underneath: cumulative effort hours, two color-distinguished lines (estimate, actual). Markdown legend below the chart with the color codes.

**Side-by-side layout.** 2-column Markdown table with Mermaid blocks in cells. GitHub renders Mermaid in table cells. Fallback if it breaks (or in MkDocs Material per [IDEA-022](idea-022-mkdocs-documentation-site.md)): two stacked charts with their own headings.

**Defensive line-identity.** `themeVariables.xyChart.plotColorPalette` may not be honored everywhere. To make the chart degrade gracefully, also encode series identity in the chart title — e.g. "Cumulative effort hours — first line: estimate, second: actual." Ugly if colors render correctly, accurate if they don't.

### Acceptance criteria

- [ ] `scripts/release_burnup.py` exists in `awesome-task-system/scripts/` and is synced.
- [ ] Running `housekeep --apply` after a task close updates the BURNUP block in OVERVIEW.md.
- [ ] Running `housekeep --apply` with no new closures since the last run leaves OVERVIEW.md byte-identical.
- [ ] Legacy effort labels in already-closed tasks contribute correctly to the chart.
- [ ] Tasks with no `effort_actual` appear in the count chart but not the actual-hours line.
- [ ] Paused tasks do not appear in the burn-up.
- [ ] Manual smoke test: render the section against the current repo state, eyeball it for sanity.

### Estimated effort

**M (2–8h)**. The script itself is straightforward; most of the time goes into Mermaid rendering quirks and the housekeep integration.

### Risks

- **Mermaid table-cell rendering breaks on a future GitHub change.** Mitigation: stacked-chart fallback documented and ready to switch in.
- **`themeVariables.xyChart.plotColorPalette` not honored on some renderer.** Mitigation: title-based line identity (above).
- **Git-log parser edge cases** — multi-task archival commits, renamed files, tasks closed and reopened multiple times. Mitigation: dedupe-by-basename and `--no-renames` cover the common cases. Add unit tests for the parser specifically (synthetic git log fixtures).

---

## 6. Sequenced execution checklist

| # | Task | Depends on | Estimate | Notes |
|---|---|---|---|---|
| 1 | TASK-PRE — single source of truth for `awesome-task-system/` | — | L | Do this first. Unblocks everything. |
| 2 | paused-as-first-class status | TASK-PRE | L | Largest single piece. Lands the lifecycle change. |
| 3 | `effort_actual` on close | TASK-PRE | S | Can run in parallel with paused in principle; sequence after for tidy commits. |
| 4 | release burn-up chart | TASK-PRE, paused, effort | M | Lands last; both data sources are in place. |

**Total estimate: ~18–58 hours of work.** Spread across four discrete tasks, each independently mergeable.

---

## 7. Sub-idea body updates required before scaffolding tasks

Once this rollout is accepted:

- **`llm-effort-reassessment-on-close`** — replace the five-size vocabulary section with a pointer to the canonical six (§4 above). Soften the "XL already exists in the live vocabulary" claim if needed (verify with `grep` first).
- **`paused-as-first-class-task-status`** — replace the "big surprise: `awesome-task-system/` is a separate package" section with a pointer to TASK-PRE as a prerequisite. Drop the "every change must be applied twice" phrasing. Update the cost summary (no doubling cost). Add the explicit lifecycle / verb mapping table from §3 above (it's clearer than the current prose).
- **`release-burnup-chart`** — minor: soften the "vast majority will carry `effort_actual`" framing, since the example overstates v1 reality. Add the title-based line-identity fallback to the open questions.

---

## 8. Open questions

1. **TASK-PRE pointer mechanism — symlinks vs sync script?** Recommendation: sync script. Confirm before scaffolding.
2. **`tasks` skill — should it be added to the package?** Recommendation: yes (it's a task-system skill). Confirm.
3. **Migration of the 4 old-model paused tasks (153/154/155/156)?** Recommendation: skip, let it self-resolve. Confirm.
4. **Should TASK-PRE include consolidating `update_idea_overview.py` and the `ts-idea-*` skills too?** Recommendation: not strictly needed — verified byte-identical today, so they're already implicitly canonical via the package. But include them in the divergence-guard scope so they stay that way. Confirm.
5. **Order of effort and paused — strictly sequenced, or parallelizable?** Recommendation: sequence (paused first, then effort) for tidy commits and to settle the canonical vocabulary before effort references it. Confirm.
6. **Tests directory — does the package own its tests, or stay test-less?** Recommendation: mirror `scripts/tests/` into `awesome-task-system/scripts/tests/` and include in the divergence guard. A carve-out package without tests is incomplete, and the paused task body adds new test cases that need a canonical home. Confirm.

---

## 9. Minor follow-ups (bundle into the rollout)

Two small task-system improvements surfaced during planning. Both are minor and don't deserve their own ideas — bundle them into the rollout where they fit best.

### 9.1 EPICS.md — task table ordering

Per-epic task tables in `EPICS.md` should sort tasks deterministically:

1. **Primary: by status**, in the order `open` > `paused` > `active` > `closed` (open at the top).
2. **Secondary: by `order:` field**, ascending (small to large).

The current renderer's order is whatever the directory walk produces, which is unstable across runs. Locking the sort makes the table readable and stops cosmetic-only diffs in `EPICS.md` regenerations.

**Where it lands:** `awesome-task-system/scripts/update_task_overview.py` (or wherever EPICS.md per-epic tables are rendered — confirm during implementation). Bundle into the **paused-as-first-class** task (§3), since the status-ordering rule explicitly references `paused` and that task already touches the same renderer.

### 9.2 Housekeeping — validate `order:` field per epic

Housekeeping should detect and fail on broken `order:` values inside any single epic's task set:

- **No blanks** — every task with `epic: <name>` must have a numeric `order:`.
- **No duplicates** — two tasks in the same epic cannot share an `order:` value.
- **No `?` placeholders** — `order: ?` (occasionally written when the slot is unknown) is treated as missing.

Run as part of `EPICS.md` autogeneration (the natural moment when per-epic state is already grouped). Fail loudly, and offer an autofix mode that renumbers contiguously from 1 within each epic — gated behind an explicit flag (e.g. `--fix-order`) so the user opts in rather than silently accepting a renumbering. The existing `ts-task-new` skill already derives the next `order` from the max-across-folders (per recent commit `0641dfb`); this validator complements that by catching pre-existing damage.

**Where it lands:** `awesome-task-system/scripts/housekeep.py`. Independent of the major sub-ideas — could land alongside TASK-PRE, paused, or as its own follow-up. Recommendation: bundle into TASK-PRE since the package is already being touched there and the validator is a natural addition to the housekeeping surface that's getting consolidated.
