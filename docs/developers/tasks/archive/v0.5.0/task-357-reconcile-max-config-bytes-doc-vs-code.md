---
id: TASK-357
title: Reconcile MAX_CONFIG_BYTES between BLE protocol doc and reassembler code
status: closed
closed: 2026-05-01
opened: 2026-05-01
effort: XS (<30m)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: Clarification
epic: app-content-pages
order: 15
---

## Description

The BLE config-write reassembly buffer limit is documented in two places and they disagree:

- [docs/developers/BLE_CONFIG_PROTOCOL.md §Constants](../../BLE_CONFIG_PROTOCOL.md#constants) — `MAX_CONFIG_BYTES = 32768` ("Maximum reassembled JSON size (bytes)")
- [lib/PedalLogic/include/ble_config_reassembler.h:30](../../../lib/PedalLogic/include/ble_config_reassembler.h#L30) — `#define MAX_CONFIG_BYTES 16384`

The code value is what actually gets enforced — uploads larger than 16 KB return `ERROR:too_large` regardless of what the doc claims. Clients that read the protocol spec and budget against 32 KB will hit a truncation cliff that the spec does not document.

This was flagged in [BLE_READBACK_IMPACT.md §6](../../BLE_READBACK_IMPACT.md#6-open-follow-ups-not-in-scope-here) during the TASK-353 analysis. TASK-355's resident-copy sizing assumes 16 KB (the code value); a future "raise the limit" decision would invalidate that sizing, so resolving this *before* TASK-355 lands is preferred.

**Recommended canonical value: 16 KB** — the code enforces it, TASK-355 sizes against it, and no current consumer requires larger configs (largest example in `profiles/` is well under 4 KB). Bumping the code up to 32 KB is the alternative, but doubles the resident-copy footprint TASK-355 commits to with no current need.

## Acceptance Criteria

- [x] One canonical value chosen and documented as the single source of truth (recommend the `#define` in `ble_config_reassembler.h`).
- [x] [docs/developers/BLE_CONFIG_PROTOCOL.md](../../BLE_CONFIG_PROTOCOL.md) updated so its Constants table matches the code value, with a one-line note pointing the doc reader at the header file as authoritative.
- [x] If the canonical value is to be raised (32 KB), update the code `#define` and surface the resident-copy budget impact in [BLE_READBACK_IMPACT.md §3.2](../../BLE_READBACK_IMPACT.md#32-config-readback--task-355) so TASK-355 picks up the new threshold. *(N/A — kept at 16 KB.)*
- [x] No new test work — existing `test/unit/test_ble_config_*` already exercise the boundary; verify they still pass with `make test-host`. *(300 tests pass.)*

## Test Plan

**Host tests** (`make test-host`):

- Re-run the existing reassembler unit tests; the `too_large` boundary case in `test/unit/test_ble_config_*` already covers `MAX_CONFIG_BYTES + 1`. If the canonical value changes, the test's expected boundary updates with it.

No new tests required — change is configuration-and-doc, not behavioural.

## Documentation

- `docs/developers/BLE_CONFIG_PROTOCOL.md` — Constants table value + one-line authority note pointing at `ble_config_reassembler.h`.

## Notes

- Carved out of TASK-353's "Open follow-ups" list. Small but load-bearing for TASK-355's resident-copy memory budget — the 16 KB number is referenced explicitly in the impact doc and in TASK-355's AC.
- The protocol doc was likely updated to "32768" speculatively at some point and the code never followed (or the code was tightened and the doc never updated). `git blame` on both sides will tell the story; either way, the fix is to make them agree.
- Pure doc/header change, no on-device or BLE work — can land independently of any other task in the epic.
