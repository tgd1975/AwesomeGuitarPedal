---
id: TASK-370
title: Make Schemdraw-generated SVGs deterministic so the Docs CI staleness guard can pass
status: open
opened: 2026-05-08
effort: Small (<2h)
complexity: Medium
human-in-loop: Clarification
prerequisites: [TASK-363]
---

## Description

The `Docs / Schemdraw staleness guard` job in `.github/workflows/docs.yml`
(added by [TASK-204](../closed/task-204-update-ci-staleness-guard-schemdraw.md))
fails on every push to `main`. Diagnosed in
[TASK-363](task-363-investigate-recurring-ci-failures.md): the underlying
matplotlib backend that schemdraw uses to write SVG embeds a non-deterministic
`<dc:date>` element into the file's RDF metadata block, e.g.

```xml
<dc:date>2026-04-24T23:45:42.501318</dc:date>
```

CI regenerates the two SVGs (`docs/builders/wiring/esp32/main-circuit.svg`
and `docs/builders/wiring/nrf52840/main-circuit.svg`) and then runs
`git diff --exit-code` against the committed copies. Because the embedded
timestamp is "now" inside the runner, the diff is always non-empty and the
job exits 1 — the actual schematic geometry is unchanged. The check is
flaky-by-design and cannot ever go green without a code change.

This task makes the regenerated SVGs byte-stable so the staleness guard
catches **real** drift between `data/config.json` and the committed SVGs,
which is what TASK-204 intended.

Two viable approaches (pick whichever is cleaner — both are in scope):

1. **Suppress matplotlib metadata at save time.** matplotlib's SVG backend
   accepts `metadata={}` (or specific keys set to `None`) on `savefig`,
   which controls what ends up in the RDF block. If schemdraw exposes
   the underlying figure, pass an empty/curated metadata dict to remove
   `Date` (and ideally also `Creator` if it varies by matplotlib version).
2. **Post-process the SVG.** After `schemdraw` writes the file, strip
   `<dc:date>…</dc:date>` (and any other non-deterministic elements like
   `<dc:format>` versioning) with a small regex pass before the file is
   considered "final". This is robust to schemdraw / matplotlib version
   drift but slightly uglier.

Approach 1 is preferred if it works cleanly; fall back to approach 2 if
schemdraw does not surface a way to pass `metadata` through.

## Acceptance Criteria

- [ ] Running `python scripts/generate-schematic.py --target esp32`
      twice in a row produces byte-identical output (same for `nrf52840`).
      Verify with `sha256sum` before/after.
- [ ] The committed SVGs (`docs/builders/wiring/esp32/main-circuit.svg`,
      `docs/builders/wiring/nrf52840/main-circuit.svg`) are regenerated
      with the deterministic generator and committed in this task — so
      the diff against a fresh CI regeneration is empty.
- [ ] `Docs / Schemdraw staleness guard` is green on this branch's push.
- [ ] Regression check: deliberately edit one SVG (e.g. shift a label
      attribute) without regenerating, push, and confirm the staleness
      guard fails as expected. Revert before merging.

## Test Plan

No automated tests required — change is non-functional (CI / dev tooling).
Manual verification:

- Run the generator twice locally and `sha256sum` the outputs — they
  must match.
- Push and watch `Docs / Schemdraw staleness guard` go green.
- One-shot regression test: temporarily mutate an SVG, push, watch the
  guard fail, revert.

## Prerequisites

- **TASK-363** — diagnosis of the recurring CI failures, including the
  identification of `<dc:date>` as the staleness-guard root cause.

## Notes

- **Reference run.** Failing run on `main` showing the diff signature:
  <https://github.com/tgd1975/AwesomeStudioPedal/actions/runs/25080496759>
  — search the log for `dc:date` to see the timestamp diff.
- **Schemdraw / matplotlib API.** `schemdraw.Drawing.save(filename)`
  delegates to matplotlib's `Figure.savefig`. matplotlib accepts
  `metadata=` on SVG save; if schemdraw does not pass it through,
  consider monkey-patching at script level or grabbing the underlying
  figure via `drawing.fig` / similar before saving.
- **Other non-deterministic fields to watch.** `<dc:date>` is the
  one we've seen, but matplotlib may also embed `<dc:creator>` with a
  matplotlib version. The acceptance criterion of "byte-identical
  across two runs" catches all of these in one check.
- **Out of scope.** Switching away from schemdraw, refactoring the
  schematic-generation script's structure, fixing unrelated lint
  warnings in the script. This task is *only* about determinism.
- **Cross-link.** TASK-363's closure note documents the diagnosis and
  points here as the follow-up. After this task closes, TASK-362
  (require CI pass before merge to main) becomes safer to flip on.
