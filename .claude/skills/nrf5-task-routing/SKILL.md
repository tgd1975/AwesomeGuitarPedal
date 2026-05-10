---
name: nrf5-task-routing
description: Use BEFORE invoking /ts-task-new whenever the new task touches the nRF52840 hardware target — any change under src/nrf52840/, any new test under test/test_*_nrf52840/, any nRF52840 PlatformIO env or build target, any AC requiring on-device verification on the Adafruit Feather nRF52840 (BLE via Bluefruit, GPIO, ISRs), or any Test Plan / AC item proposing new nRF52840 on-device tests (even as "nice-to-have"). Routes nRF5-primary tasks to EPIC-025 (nrf52840-blocked); for ESP32-primary tasks with stray nRF52840 test-plan items, scrubs the items instead. Active until the device is available again — removed by TASK-359 at that point.
---

# nrf5-task-routing

A temporary routing rule. While this skill exists in
`.claude/skills/nrf5-task-routing/`, **every newly-scaffolded task that
touches the nRF52840 firmware target must be assigned to
`epic: nrf52840-blocked`** rather than to the feature epic that would
otherwise own it.

## Why

The Adafruit Feather nRF52840 hardware is not currently available, so
any new nRF5-touching task cannot be on-device verified. Routing all
such work into a single hardware-blocked epic mirrors
[EPIC-019 iphone-app](../../../docs/developers/tasks/active/epic-019-iphone-app.md):
the iPhone hardware is similarly out of reach, and that epic holds all
iPhone-dependent work in one place rather than scattering it across
feature epics where it would block per-epic progress.

Without this routing, nRF5 tasks would land in their natural feature
epics (e.g. `app-content-pages`, `ble-config`) and clutter those epics'
queues with paused-on-hardware items. With it, the feature epics stay
clean and the hardware-blocked queue is one focused list.

## When this applies

A task "touches nRF52840 hardware" if **any** of the following is true:

- It modifies, adds, or deletes any file under `src/nrf52840/`.
- It adds or extends a test under `test/test_*_nrf52840/`.
- It adds or modifies an nRF52840 PlatformIO env or build target
  (e.g. `feather-nrf52840`, `make build-nrf52840`,
  `make test-nrf52840-*`).
- Its acceptance criteria require on-device verification on the
  Adafruit Feather nRF52840 (BLE via Bluefruit, GPIO, ISR timing,
  filesystem on InternalFS, `Nrf52840PedalApp`).
- Its description mentions "nRF52840", "nRF5", "Bluefruit", or
  "Feather nRF52840" in a way that implies hardware-side work
  (not merely "this is also planned for nRF52840 later").
- Its **Test Plan or AC proposes** any new nRF52840 on-device test
  — `make test-nrf52840-*`, a new `test/test_*_nrf52840/` directory,
  or "verify equivalent behaviour on nRF52840". The proposal alone
  is enough to trigger the routing logic in the next section, even
  when the task's primary work is platform-agnostic or ESP32-only.

Pure ESP32 tasks are unaffected. Tasks that touch only platform-
agnostic code in `lib/PedalLogic/` are unaffected. Tasks that
*reference* nRF52840 in passing (e.g. cross-platform docs) are
unaffected — only tasks where the work itself targets nRF52840 or
proposes nRF52840 test work.

## Tests specifically — extra vigilance

Tests that target nRF52840 hardware but cannot be run today **bitrot
silently**: nobody flashes the device, nobody notices when the test
becomes stale, and the moment hardware is back the test reads as
"how did this ever pass?". The whole point of this skill is to
prevent that category of dead weight from accumulating in the repo
while the device is gone.

Three scenarios at task-scaffold time:

1. **Task is primarily nRF52840** (already trips the criteria above
   beyond the test bullet): route to EPIC-025 as usual; the test
   creation rides along with the implementation when hardware is
   back. Single bundled item, single point of activation.

2. **Task is primarily ESP32 / cross-platform**, but its Test Plan or
   AC proposes "also add an nRF52840 on-device test" / "also extend
   `test/test_*_nrf52840/`" / "verify equivalent behaviour on
   nRF52840". In this case **do NOT route the whole task** —
   instead, **scrub the nRF52840 test items** from the Test Plan
   and ACs before the task is scaffolded, and replace them with a
   one-liner reference to TASK-360 (the parity audit):

   > "nRF52840 parity verified by [TASK-360](../../../docs/developers/tasks/paused/task-360-nrf52840-esp32-parity-audit.md)
   > once hardware is available — no nRF52840 tests added in this
   > task."

   This applies even when the proposed test would compile clean
   today. Compiling clean is not the same as running. A test that
   builds into the firmware but never exercises the device is dead
   weight that has to be pruned later — better to never land it.

3. **A new `test/test_*_nrf52840/` directory** is *never* scaffolded
   by a feature-epic task while this skill is active. The first such
   directory lands inside [TASK-358](../../../docs/developers/tasks/paused/task-358-nrf52840-ble-readback-surfaces.md)
   (BLE readback) along with the implementation. Any later test-
   directory additions also live in tasks routed to EPIC-025.

The principle: **no nRF52840 test files land in the repo until they
can be run on hardware**. TASK-360 is the single place where
nRF52840 test-coverage gaps are catalogued and resolved post-
hardware-arrival.

## How to apply

1. **Decide which path applies** using the criteria above and the
   "Tests specifically" scenarios:

   - **Scenario 1 (nRF5-primary)**: continue with steps 2–5 below to
     route the task into EPIC-025.
   - **Scenario 2 (ESP32-primary with stray nRF52840 test items)**:
     skip the routing — instead, scrub the nRF52840 test-plan / AC
     items from the proposed task body and replace them with the
     TASK-360 reference one-liner. Then invoke `/ts-task-new`
     normally with the cleaned-up scope. **Do not** route the task
     itself; the routing is for the *work*, not for residual prose.
   - **Scenario 3 (new `test/test_*_nrf52840/` directory)**: stop —
     such a directory only lands inside an existing EPIC-025 task
     (e.g. TASK-358). Do not scaffold a new feature task that
     creates one. If the proposed work cannot fit into an existing
     EPIC-025 task, scaffold it as a new EPIC-025 task instead.
   - **None apply**: invoke `/ts-task-new` normally.

2. **Determine the next `order:` within EPIC-025.** Scan tasks with
   `epic: nrf52840-blocked` across `open/`, `active/`, `paused/`, and
   `closed/`. Take the maximum `order` value, add 1.

   ```bash
   grep -l "^epic: nrf52840-blocked$" docs/developers/tasks/*/*.md \
     | xargs grep -h "^order:" | awk '{print $2}' | sort -n | tail -1
   ```

3. **Invoke `/ts-task-new`** with `--epic nrf52840-blocked` and
   `--order <next>`. Pick `--effort` and `--complexity` based on the
   actual work as usual. Default `human-in-loop: Clarification`
   unless the task body says otherwise.

4. **Set `status: paused`** on the resulting file (the default from
   `/ts-task-new` is `open` — manually flip to `paused` and run
   `/housekeep` so the file lands in `paused/`). Hardware blockers do
   not use a task-system `prerequisites:` field; instead, add a
   `## Paused` section in the body explaining the blocker, following
   the pattern of [task-158](../../../docs/developers/tasks/paused/task-158-feature-test-ios-build-deploy.md).

   Example:

   ```markdown
   ## Paused

   - YYYY-MM-DD: Paused — no Adafruit Feather nRF52840 device is
     currently on hand. <one-line summary of which AC item requires
     hardware>. Resume with `/ts-task-active TASK-NNN` once a device
     is available.
   ```

5. **Record the original feature epic in the Notes section** so the
   re-homing performed by TASK-359 (when hardware returns) is
   mechanical:

   ```markdown
   ## Notes

   - Originally belongs to feature epic `<feature-epic>` by scope;
     routed to EPIC-025 nrf52840-blocked while nRF52840 hardware is
     unavailable. Once TASK-359 closes, this task can be re-homed to
     `<feature-epic>`.
   ```

   The `<feature-epic>` value is whatever epic the task would have
   belonged to without this routing — i.e. the natural feature epic
   for its scope. If the task does not have a natural feature epic
   (genuinely cross-cutting infra), say so in the Notes line and
   omit the re-homing step from the AC of the future cleanup.

## When this skill is removed

[TASK-359](../../../docs/developers/tasks/open/task-359-remove-nrf5-task-routing-skill.md)
removes this skill once hardware is back. At that point this directory
is deleted and `nrf5-task-routing` comes off the `enabled_skills` list
in [.vibe/config.toml](../../../.vibe/config.toml). Until then, the
skill is active and the routing rule above is mandatory for every
nRF5-touching task.
