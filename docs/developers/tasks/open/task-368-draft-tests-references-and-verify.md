---
id: TASK-368
title: Draft Tests + References sections of the BLE services guide; verify success criterion; close epic
status: open
opened: 2026-05-02
effort: Small (<2h)
complexity: Medium
human-in-loop: No
epic: ble-services-developer-guide
order: 4
prerequisites: [TASK-366, TASK-367]
---

## Description

Finish the BLE services developer guide and validate it against the chosen success criterion:

1. **Tests section** — pointers to the relevant tests on each layer: host fakes (`test/unit/test_*.cpp`), on-device runner (`test/test_*_esp32/runner.py`), app integration (`app/test/...`). Include the decision rule "is this characteristic shimmable for a host test?" (per CLAUDE.md testing policy) with worked yes/no examples drawn from existing tests.
2. **References section** — Bluetooth SIG conventions, NimBLE-Arduino docs, Bluefruit docs, relevant project closed-task IDs. Cite, do not copy.
3. **Verification pass** — re-read the assembled guide end-to-end. Confirm against EPIC-027's chosen success criterion (default from IDEA-056: "the recipe + worked example is sufficient to add the characteristic the example walks through, without grepping or asking"). Record the result in this task's closing comment — pass/fail/gap.

## Acceptance Criteria

- [ ] Tests section names the file paths and `make` targets per layer (host, ESP32 on-device, app integration).
- [ ] References section cites at least one entry per source class (Bluetooth SIG, library docs, internal task history).
- [ ] Verification note recorded — either confirms the success criterion is met, or names the specific gap.
- [ ] EPIC-027 closes when this task closes (housekeep derives epic status from child task statuses).

## Test Plan

No automated tests required — change is non-functional.

## Prerequisites

- **TASK-366** — provides the catalog + recipe + worked example that the verification pass exercises.
- **TASK-367** — provides the conventions + invariants + gotchas that the Tests section cross-references.

## Notes

- This is the close-out task for EPIC-027. The success-criterion check lives here so the epic does not drift past "done" without verification.
- After writing, invoke `/doc-check` since the file lives under `docs/developers/`.
