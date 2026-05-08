---
id: IDEA-024
title: Profile-Independent Actions
category: firmware
description: An optional always-active pseudo-profile whose actions fire in parallel with the active profile's actions on every button event — regardless of which profile is selected. If the block is absent, behaviour is identical to today.
---

## Archive Reason

2026-04-29 — Converted to EPIC-020 (TASK-306, TASK-307, TASK-308, TASK-309).

## Context

Today, every button action belongs to a switchable profile. If you want an LED to light
while a button is held, you must add that pin action to every profile separately. When you
add a new profile, you must remember to add it there too.

The idea is a top-level `"independentActions"` block in `profiles.json` — same button-map
syntax as a profile's `"buttons"` block — that is never selectable and never switchable.
On every button event (press, long-press, double-press) the matching independent action
fires alongside whatever the current profile does. A missing entry for a button is a no-op.

**Example:** `"independentActions": { "A": { "type": "PinHighWhilePressed", "pin": 5 } }`
— pin 5 goes high whenever button A is held, no matter which profile sends what BLE key.

---

## Scope

- All action types supported (SendChar, PinHighWhilePressed, Macro, …)
- All event types supported (press, long-press, double-press)
- `independentActions` does **not** occupy a profile slot in `ProfileManager`
- `executeRelease()` must be forwarded for "while pressed" pin actions
- `ConfigLoader::saveToFile` and `mergeConfig` must round-trip the block correctly
- All downstream tooling needs updating: `profiles.schema.json`,
  `docs/builders/KEY_REFERENCE.md`, web simulator, web config builder, Flutter app

---

## Notes

- Dispatch site (likely `EventDispatcher` or `main.cpp`) runs both actions without one
  blocking the other.
- Switching profiles must leave the independent action set untouched.
- The block is **optional** — the user adds it only when they need it. Existing
  `profiles.json` files without the key behave exactly as today.
