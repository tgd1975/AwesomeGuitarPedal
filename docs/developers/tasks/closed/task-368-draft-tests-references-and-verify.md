---
id: TASK-368
title: Draft Tests + References sections of the BLE services guide; verify success criterion; close epic
status: closed
closed: 2026-05-11
opened: 2026-05-02
effort: Small (<2h)
effort_actual: Small (<2h)
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

## Acceptance Criteria — verification (2026-05-11)

- [x] **Tests section names the file paths and `make` targets per layer.** Host: [test/unit/test_ble_config_service.cpp](../../../test/unit/test_ble_config_service.cpp), `make test-host`. ESP32: [test/test_ble_config_esp32/](../../../test/test_ble_config_esp32/) → `make test-esp32-ble-config`, [test/test_ble_pairing_esp32/](../../../test/test_ble_pairing_esp32/) → `make test-esp32-ble-pairing`. App: [app/test/unit/ble_service_*.dart](../../../app/test/unit/) (4 files) and [app/test/widget/connected_pedal_screen_test.dart](../../../app/test/widget/connected_pedal_screen_test.dart) (+ peers), `make test-flutter`. nRF52840: noted as no-BLE-test-env today, comes with TASK-358. Plus the shimmable / not-shimmable decision rule with six worked yes/no examples.
- [x] **References section cites at least one entry per source class:** Bluetooth SIG (ATT/GATT/HID/DIS), library docs (NimBLE-Arduino, ESP32-BLE-Keyboard, Bluefruit, bleak, BlueZ), internal task history (TASK-235 / TASK-236 / TASK-240 / TASK-250 / TASK-353 / TASK-357 / TASK-358 / EPIC-026 cluster / IDEA-046), and internal docs (BLE_CONFIG_PROTOCOL.md, BLE_READBACK_IMPACT.md). Three+ source classes covered.
- [x] **Verification note recorded** — see below.
- [x] **EPIC-027 closes when this task closes** — derived by housekeep from child-task statuses (TASK-365 / TASK-366 / TASK-367 / TASK-368 all closed).

## Verification pass (2026-05-11) — success criterion

**Chosen success criterion (TASK-365 → user-confirmed):** *"The next BLE-touching task references the guide in its description and checks off its recipe items."*

**Verdict: conditional pass — deferred to confirmation by the next BLE task.**

The success criterion is forward-looking by construction: it asserts *adoption*, not *content*. Strict verification requires an actual next BLE task to land and reference the guide. The candidate next-BLE tasks are the EPIC-026 cluster — [TASK-354](../../../docs/developers/tasks/open/task-354-firmware-version-read-characteristic.md) (worked example in the guide), [TASK-355](../../../docs/developers/tasks/open/task-355-firmware-config-readback.md), [TASK-356](../../../docs/developers/tasks/open/task-356-firmware-active-profile-notify.md) — currently open under EPIC-026, blocked or unstarted.

Today's verification is **content readiness** — is the guide in a state where the next BLE task *can* reference it and check off concrete recipe items? End-to-end re-read result:

| Check | Verdict | Evidence |
|---|---|---|
| Catalog correctly inventories what is in firmware today | Pass | Cross-checked against [ble_config_service.cpp:18-22](../../../src/esp32/src/ble_config_service.cpp#L18) — four chars on ESP32 (`5c1`/`5c2`/`5c3`/`5c4`). nRF52840 = HID-only, matches code. |
| Catalog names planned EPIC-026 chars with task IDs | Pass | `FIRMWARE_VERSION` (`5c5`, TASK-354), `CONFIG_READBACK` (TASK-355), `ACTIVE_PROFILE` (`5c6`, TASK-356) — all in the *Planned* subtable. |
| Recipe is a numbered checklist with concrete file paths | Pass | 12 steps; every step names at least one file or `make` target; no step requires the implementer to grep. |
| Worked example walks one real characteristic end-to-end | Pass | TASK-354 walk-through covers firmware (`ble_config_service.cpp`, `version.h`), app constant (`ble_constants.dart`), app service (`ble_service.dart` field/discovery/method), app screen (`connected_pedal_screen.dart`), widget test + mocks, protocol doc, on-device runner. Sequencing explicit. |
| Cross-cutting invariants table covers `MAX_CONFIG_BYTES` | Pass | First row, with full file set (header / protocol doc / app constant / boundary test) and the "edit every file in the row in the same commit" rule. |
| Gotchas cite ≥ 3 closed-task references | Pass | TASK-235, TASK-236, TASK-250, TASK-357, IDEA-046 — five total. |
| Tests section names paths + `make` targets per layer | Pass | Host / ESP32 on-device / nRF52840 / app unit / app widget all enumerated with `make` targets and shimmability rule. |
| References cover SIG + library + internal | Pass | Four library entries, ten internal task / doc references. |
| All section anchors resolve from the table of contents | Pass | TOC entries map to actual `##` headings with matching slugs. |

**Identified gaps and follow-ups:**

1. **No live "next BLE task" amendment yet.** The success criterion cannot be strictly closed until at least one BLE task (most plausibly TASK-354 when it unblocks under EPIC-026) explicitly references the guide in its description / AC and checks off the Recipe items in its closing comment. This is an artefact of the criterion's forward-looking nature — the guide is ready, adoption is the next step.
2. **`BLE_CONFIG_IMPLEMENTATION_NOTES.md` retained as a redirect stub** rather than `git rm`-ed (TASK-365's plan). Rationale: the source-code references in [src/esp32/include/ble_keyboard_adapter.h](../../../src/esp32/include/ble_keyboard_adapter.h) would break, and updating those comments was deemed out of scope for EPIC-027's strict docs-only constraint. The stub is short, redirects to specific sections, and does not duplicate content.
3. **nRF52840 catalog rows are necessarily sparse** because the platform has no custom BLE service today. Will flesh out when [TASK-358](../../../docs/developers/tasks/paused/task-358-nrf52840-ble-readback-surfaces.md) lands.

**Recommendation for the next BLE task:** when activating TASK-354 (or whichever BLE task lands next), amend its description with a one-liner — *"Follows the recipe in BLE_SERVICES_GUIDE.md (worked example for this characteristic in § Worked example)."* — and tick the relevant Recipe items in its closing comment. That closes EPIC-027's success criterion strictly.

## Output

- Sections 7 (Tests), 8 (References) of [docs/developers/BLE_SERVICES_GUIDE.md](../../BLE_SERVICES_GUIDE.md) populated, replacing the placeholders left by TASK-366. TOC cleaned of "populated by TASK-XYZ" notes.
- Verification verdict recorded above. Conditional pass on content readiness; final pass deferred to the next BLE task referencing the guide.
- `/doc-check` verdict applied manually: persona `developer` (path) matches `developer` (content). High confidence, correctly placed.
