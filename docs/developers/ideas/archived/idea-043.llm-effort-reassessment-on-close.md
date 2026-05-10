# Re-assess effort on task close (estimate vs. actual)

> Sub-note of [IDEA-043](idea-043-coordinated-task-system-rollout.md)

## Archive Reason

2026-04-28 — Implemented as part of IDEA-043 rollout. `effort_actual:` frontmatter field is written by `/ts-task-done` and present on closed tasks (TASK-281, 282, 284, 285, 286, 289, 293, 294, 297…). The actual-hours line in the burn-up chart consumes it.

## Motivation

[IDEA-043-release-burnup-chart](idea-043.release-burnup-chart.md) plots cumulative "estimated effort" since the last release tag — but those numbers are the *initial* guess, recorded before the work was understood and never corrected. Tobias is curious how those first estimates held up. So am I.

## Approach

`/ts-task-done` gains a hindsight-effort step that mirrors how `/ts-task-new` already handles `effort:` today: the user can pass a flag (`--effort-actual S|M|L|XL|Trivial`), and if no flag is given the agent picks a value from the work product. One new frontmatter field:

```yaml
effort: Small (<2h)            # original estimate, never edited
effort_actual: Medium (2-8h)   # post-hoc t-shirt size, same vocabulary as effort
```

The original `effort:` keeps its value untouched so historical data stays readable. IDEA-043-release-burnup-chart's burn-up script gets a second cumulative line — estimate-hours vs. actual-hours — using the same midpoint table for both. The gap between the lines is the interesting part.

### Concrete change to `/ts-task-done`

Today the skill is purely mechanical ([SKILL.md](../../../../.claude/skills/ts-task-done/SKILL.md)). Add one step between current steps 2 and 3:

> **2a.** Determine `effort_actual` and write it to frontmatter immediately after `effort:`.
> If the user passed `--effort-actual`, use that. Otherwise pick a t-shirt size from the work product: diff size, files touched, commits between `opened:` and today, and any pause/blocker history visible in the task body.
>
> **No-peek rule:** when judging from the work product, do not look at the original `effort:` value first. Pick the actual, *then* reveal the original. This avoids regression toward "I was about right" — the same agent estimating both ends has an obvious bias risk and the chart only earns its keep if the gap is honest.

The vocabulary is the same as `effort:` and identical to the `--effort` flag in [`ts-task-new`](../../../../.claude/skills/ts-task-new/SKILL.md): `Trivial (<30m)`, `Small (<2h)`, `Medium (2-8h)`, `Large (8-24h)`, `Extra Large (24-40h)`. XL already exists in the live vocabulary and is in use on open tasks — no new size is needed.

### Vocabulary normalization — depends on IDEA-043-release-burnup-chart

IDEA-043-release-burnup-chart's open questions flag that closed tasks currently use four "Small" variants (`<2h`, `1-2h`, `1-3h`, `2-4h`). `effort_actual` writes only the canonical post-normalization vocabulary, whichever IDEA-043-release-burnup-chart settles on. Without that, hindsight values like `Small (1-2h)` vs. originals like `Small (<2h)` would record meaningless drift driven by vocabulary, not estimation.

If IDEA-043-release-burnup-chart lands first → use its canonical set. If this idea lands first → freeze on the five sizes above and let IDEA-043-release-burnup-chart normalize the historical `effort:` field around the same set.

## Non-goals

- No retroactive backfill on already-closed tasks. The chart starts useful from the first close after this lands.
- No re-fill on later passes. If a task is closed by hand without the agent and `effort_actual` stays blank, it stays blank forever — don't let a future session synthesize a value from old diffs.
- No replacing `effort:` with `effort_actual`. Both stay; the comparison is the point.
- No new t-shirt size. XL (`Extra Large (24-40h)`) already exists.
