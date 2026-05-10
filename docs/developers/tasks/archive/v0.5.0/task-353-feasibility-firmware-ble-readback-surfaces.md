---
id: TASK-353
title: Feasibility & impact analysis — firmware BLE readback surfaces
status: closed
closed: 2026-05-01
opened: 2026-05-01
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Medium
human-in-loop: Main
epic: app-content-pages
order: 11
---

## Description

Before any firmware code lands for the EPIC-023 BLE readback work
(originally TASK-337), produce a written feasibility & impact
analysis covering the three deliverables, on both target platforms
(ESP32 and nRF52840). The output gates TASK-354 / TASK-355 /
TASK-356 — those tasks reference this analysis to scope themselves.

The three deliverables under analysis:

1. **Firmware-version read characteristic** + Device Information
   Service (DIS, 0x180A) evaluation. Cheap, read-only.
2. **Config readback** — option (a) make existing config char
   readable, retaining the canonical JSON in RAM, vs option (b) a
   separate "current config" read char with chunked reassembly,
   no resident copy.
3. **Active-profile-index notify characteristic.**

## What the analysis must cover

For each deliverable, on each of ESP32 and nRF52840:

- **Per-platform implementation sketch** — which library/API is
  involved (NimBLE / Bluedroid on ESP32; Bluefruit nRF52 on
  nRF52840), where in the existing GATT table the new char/service
  fits, what the registration call looks like.
- **Memory cost** — flash + RAM, per platform. For config-readback,
  measure or estimate the resident-copy size cost of option (a) vs
  the chunked-reassembler cost of option (b). nRF52840 has the
  tighter budget; the task body of the original TASK-337 explicitly
  flagged this.
- **Test strategy** — host-testable behind `HOST_TEST_BUILD` or
  on-device-only? Which `test/test_*_*` directory does each
  characteristic's test extend?
- **Risk** — handle exhaustion, bond/security implications, MTU
  pressure, race conditions with notifies.
- **Recommended go / no-go / defer**.

For config-readback specifically:

- **Pick option (a) or option (b)** and write the rationale here.
  Subsequent firmware tasks must follow the chosen path.
- **Document the nRF52840 RAM headroom number** measured today, so
  TASK-355 has a baseline to verify against.

For DIS (0x180A):

- Decide whether to bundle DIS with the firmware-version char in
  TASK-354, or skip it entirely. DIS is "free" if the version
  string is already constructed; the only cost is the second
  service registration.

## Acceptance Criteria

- [ ] Document committed at `docs/developers/BLE_READBACK_IMPACT.md`
      (or appended to `BLE_CONFIG_IMPLEMENTATION_NOTES.md` if that's
      the convention) covering the three sections above.
- [ ] Config-readback option (a vs b) decision is explicit, with a
      one-paragraph rationale.
- [ ] DIS (0x180A) decision is explicit (bundle / skip / defer).
- [ ] nRF52840 RAM headroom number is captured — even if it's an
      estimate from the latest .map file rather than a fresh
      measurement.
- [ ] TASK-354 / TASK-355 / TASK-356 frontmatter reference this
      doc and pick up the option-a/b decision.

## Test Plan

No automated tests — this is an analysis/decision task.

## Notes

- This was carved out of the original TASK-337 because that task
  bundled three independently-scopable firmware deliverables and
  a real design decision (config-readback option a vs b). Doing
  the analysis first means TASK-354/355/356 each scope themselves
  honestly rather than the original "Large/Senior" hand-wave.
- Human-in-loop is **Main** — the user is the decision-maker for
  option (a vs b) and the DIS bundle/skip choice; the agent's job
  is to surface the trade-offs and write up the chosen path.
- The agent must not start TASK-354 / TASK-355 / TASK-356 until
  this task is closed.
