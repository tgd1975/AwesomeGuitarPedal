---
id: TASK-320
title: Pre-commit hook — remove BLE_CONFIG_TEST_BUILD check (orphan from TASK-236)
status: closed
closed: 2026-04-30
opened: 2026-04-29
effort: XS (<30m)
effort_actual: XS (<30m)
complexity: Junior
human-in-loop: No
---

## Description

[TASK-236](../archive/v0.4.0/task-236-retire-ble-config-test-build.md)
retired the `BLE_CONFIG_TEST_BUILD` build flag in v0.4.0. Its AC bullet
*"removed from … the pre-commit hook"* was left unchecked when the
task was closed. The check still lives in `.git/hooks/pre-commit` —
it never made it into the canonical `scripts/pre-commit`, which is
part of the same hook-divergence problem this cleanup addresses.

`grep -rn BLE_CONFIG_TEST_BUILD` against active C++/header/ini source
returns no hits today, so the check is guarding a flag that no longer
exists. Remove it. Also sweep the other unchecked AC items from
TASK-236 (`release.yml`, `RELEASE_CHECKLIST.md`,
`BLE_CONFIG_IMPLEMENTATION_NOTES.md`) since they were left in the
same half-done state.

## Acceptance Criteria

- [ ] BLE security flags block removed from `.git/hooks/pre-commit`
      (and not re-introduced into `scripts/pre-commit`)
- [ ] `grep -rn BLE_CONFIG_TEST_BUILD` in active source returns no
      hits — only archived task docs may still mention it
- [ ] No mentions remain in `release.yml`, `RELEASE_CHECKLIST.md`,
      `BLE_CONFIG_IMPLEMENTATION_NOTES.md`, or `BLE_CONFIG_PROTOCOL.md`
      (BLE_CONFIG_PROTOCOL.md:134 still describes the flag as current
      behavior — confirmed stale during 2026-04-29 hook reconciliation)

## Test Plan

No automated tests required — change is non-functional.

Manual: stage an unrelated change in `platformio.ini` and run
`git commit --dry-run`. Confirm the hook no longer prints
"Checking BLE security flags…".

## Notes

The check was originally belt-and-suspenders for two production-leak
defects:

- [TASK-228](../archive/v0.4.0/task-228-defect-adv-override-leaks-to-production.md)
  — adv-override leaked to production
- [TASK-229](../archive/v0.4.0/task-229-defect-ble-pairing-policy-mitm.md)
  — BLE pairing policy MITM

Safe to remove now that the underlying flag is gone — there is nothing
left to leak.
