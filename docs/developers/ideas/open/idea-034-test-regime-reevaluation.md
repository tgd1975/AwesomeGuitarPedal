---
id: IDEA-034
title: Test Regime Reevaluation — minimize human-in-the-loop after IDEA-028
category: tooling
description: After the IDEA-028 HAL refactoring lands, do a thorough reevaluation of the entire test regime — close gaps, drop deprecated/duplicate tests, and minimize human-in-the-loop coverage (e.g. BLE is now testable headlessly).
---

## Motivation

The Hardware Abstraction Layer refactoring in [IDEA-028](idea-028-hardware-abstraction-layer.md)
will reshape how platform code is structured and what is shimmable for host tests.
Once it lands, the existing test suite will no longer reflect the new boundaries:

- Some tests will be **deprecated** (covering code paths that no longer exist or
  are now covered at a different layer).
- Some coverage will be **duplicated** across host/on-device layers in ways that
  no longer make sense after the HAL split.
- Some areas will have **gaps** that the old structure made hard to reach but
  the HAL boundary now makes trivial.
- Some tests still require a **human in the loop** even though the underlying
  capability could be tested headlessly. Concrete recent example: we discovered
  the BLE stack can be exercised without a human present (no phone needed,
  no manual pairing dance) — meaning BLE tests that used to require a human
  no longer have to.

The point of this idea is not to add tests reactively as IDEA-028 lands, but
to sit down once the dust has settled and **redesign the test regime as a
whole** with clear principles.

## Goal

A refactored testing system that has:

- **As little human-in-the-loop as possible** — every test that *can* be
  automated *is* automated, on host or on-device.
- **As much human-in-the-loop as necessary** — we don't pretend things are
  covered when they actually need a human (e.g. perceptual checks, ergonomics,
  real-world stage use). Those tests stay, but are clearly labelled and
  separated from the automated suite.

## Scope of the reevaluation

Cover at least:

1. **Inventory** — what host tests, on-device tests, and manual checklists
   exist today, and what each one actually covers.
2. **Classification** — for each existing test, after IDEA-028:
   - keep as-is
   - migrate (e.g. on-device → host because it's now shimmable)
   - merge (duplicated coverage across layers)
   - drop (deprecated by the refactoring)
3. **Gap analysis** — what is *not* covered today that the HAL boundary now
   makes practical to test (BLE without a human is the prototype example;
   look for others).
4. **Human-in-the-loop audit** — for every test that still needs a human,
   ask: *can this be automated now?* If yes, plan the conversion. If no,
   document *why* it must stay manual.
5. **Test-policy update** — refresh the testing policy in `CLAUDE.md` so the
   "host vs. on-device" decision rule reflects the new reality (HAL fakes,
   headless BLE, etc.).

## Concrete sub-thread: adb-driven Flutter app feature tests

Today, every functional test of the Flutter app (the `FT-`/`PL-`/`PE-`/`AE-`/`UP-`/`JP-`
families in [`FEATURE_TEST_PLAN.md`](../../FEATURE_TEST_PLAN.md)) is **manual**.
A human runs the app on a real phone, taps through it, and records pass/fail.
TASK-153/154/155/156 made this concrete: the Pixel 9 was driven via `adb` and
screenshots were captured semi-manually. The shape of those runs strongly
suggests they can be **fully scripted** as part of the regular host test suite.

### Goal

Promote the manual app feature tests into automated, adb-driven tests that
run from a developer machine against a phone in USB-debug mode, with the
same status as host unit tests — i.e. the pre-commit / CI gate refuses to
ship if any of them break.

### Operational requirement: lock-screen timeout management

A real phone running these tests will sleep mid-run if the screen-off
timeout is at its normal user value (often 30 s – 5 min). The test
runner must therefore:

1. **Before the run:** read the current
   `settings get system screen_off_timeout` value via adb and save it.
2. **During the run:** set the timeout to ~2 h (e.g. 7_200_000 ms) so the
   phone does not lock or dim during long suites.
3. **After the run (always — success, failure, or interrupt):** restore
   the saved value with `settings put system screen_off_timeout <orig>`.

This must be wrapped in a context manager / `try/finally` so the user's
normal lock policy is never left disabled. The same pattern applies to
any other phone state we mutate (BT on/off, brightness, etc.) — capture
old value, mutate, restore.

### Drafted framework proposal

A pragmatic stack that fits the existing Python tooling without pulling
in a heavy mobile-test dependency:

| Layer | Choice | Why |
|------|--------|-----|
| Driver | Direct `adb` shell + `uiautomator dump` | We already use it (`scripts/screenshot.py`, the manual runs). No daemon to install on device. Resolves real bounds, not approximated taps from screenshots. |
| Element lookup | XML parse of `uiautomator dump` (filter by `content-desc` / `text` / `resource-id`, then locate enclosing `clickable="true"` parent) | Same approach the TASK-154 run already uses ad-hoc; codify it as a small helper. |
| Test framework | `pytest` | Same Python toolchain already in `scripts/`; integrates with `make test-host` cleanly via a marker (`@pytest.mark.adb_app`). |
| Fixtures | `pytest` fixtures | One fixture for the screen-off-timeout save/restore. One for app reset (`pm clear`). One for pushing test JSON fixtures to `/sdcard/Download/`. One for screenshot-on-failure. |
| Skip rule | Auto-skip the marked tests if `adb devices` returns no device | Keeps the suite green on dev machines without a phone, while still gating CI / release runners that have one attached. |
| Evidence | PNG screenshot per failed assertion, attached to pytest output | Mirrors what the manual runs already produce; useful for triage. |

Concrete first artifacts to ship under this idea:

- `test/app/conftest.py` — fixtures for `adb_device`, `screen_timeout`,
  `app_state_reset`, `pushed_fixtures`.
- `test/app/util/ui.py` — helpers `dump_ui()`, `find_clickable(text=…|content_desc=…|res_id=…)`,
  `tap(node)`, `type(text)`, `wait_for(text=…)`.
- `test/app/test_profile_list.py` — first ported test file, covering the
  `PL-` family from the feature-test plan that is reliably automatable.
- A `make test-app` target that wraps the screen-timeout fixture and
  invokes `pytest -m adb_app`.

Open questions for this sub-thread (decide during the wider IDEA-034 design pass):

- Drag-to-reorder (PL-05 in TASK-154) cannot be driven reliably with
  `adb input swipe` against Flutter's `ReorderableListView`. Does it
  stay manual, or do we adopt `flutter_driver` / Patrol *just* for
  gesture-heavy tests and keep adb for the rest?
- Where do screenshots live? `_attachments/` is fine for evidence
  attached to a task, but for an automated suite producing per-run
  artefacts we probably want `/tmp/<run-id>/` and only persist on
  failure.
- Fixture management: keep generating invalid-JSON / invalid-schema
  fixtures inline (as TASK-154 did), or commit a versioned set under
  `test/app/fixtures/`?

## Open questions

- Do we want a third tier between pure host tests and full on-device tests
  — e.g. "headless integration" tests that run real components (BLE stack,
  HAL fakes) without hardware?
- Should the manual checklists be promoted into a versioned document
  (like `docs/developers/manual-tests.md`) so they're not lost between
  releases?
- Is there an automated hardware test rig story here that overlaps with
  [IDEA-014](idea-014-automated-hardware-testing-rig.md)? Likely yes —
  this idea should explicitly cross-reference it.

## Dependencies

- **Blocked by IDEA-028** — the HAL refactoring must be substantially
  complete before this reevaluation is worth doing; otherwise the analysis
  targets a moving structure.
- **Related to [IDEA-014](idea-014-automated-hardware-testing-rig.md)** —
  the test-rig idea is one possible answer to the "remove the human"
  question for hardware-bound tests.
- **Related to [IDEA-004](idea-004-nrf-hardware-testing.md)** — nRF
  hardware test coverage should be folded into the reevaluation rather
  than tracked as a separate effort.
