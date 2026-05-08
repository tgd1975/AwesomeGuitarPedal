---
id: IDEA-044
title: Action Editor value field clears (or contextualises) when Action Type changes
category: apps
description: When the user changes Action Type in the Action Editor, the value field currently keeps its previous text — so switching from "Key (named)" / KEY_PAGE_UP to "Media Key" leaves the field reading `KEY_PAGE_UP`, which is meaningless under the new type. Make the field reset, or at least show a type-appropriate placeholder.
---

## Archive Reason

2026-04-28 — Converted to TASK-280.

## The Idea in One Sentence

When the user switches Action Type in the Action Editor, the value
field's current text should either clear automatically, or surface a
visible "this value isn't valid for the new type" affordance so the
user is not silently left with a stale entry that happens to look
syntactically fine.

---

## Why This Matters

Surfaced in TASK-155 round 3 (2026-04-27, AE-04, AE-06, AE-07
recipes). When the recipe walked through

1. Action Type = "Key (named)", value = `KEY_PAGE_UP`,
2. switch Action Type → "Media Key" — value field still reads `KEY_PAGE_UP`,
3. switch Action Type → "Type String" — still `KEY_PAGE_UP`,
4. switch Action Type → "Key (raw HID)" — still `KEY_PAGE_UP`,

the field's *placeholder hint* updates correctly per type ("Media Key",
"String to type", "HID code (e.g. 0x28)" — the latter is its own
defect, see TASK-277), but the **typed value** does not. A user who
overlooks the carry-over can save a "Media Key" action whose value is
`KEY_PAGE_UP` (or, conversely, a string action whose value happens
to look like a key name).

This is cosmetic — the Save path treats the field as the source of
truth, and most of these values fail validation downstream — but it
is a small intuitive-feel cost that compounds with TASK-277 and the
AE-U2 terminology session (TASK-278).

---

## Possible Approaches

1. **Hard clear**: changing Action Type wipes the value field.
   Simple, predictable. Loses the user's last-typed value if they
   accidentally tap the dropdown, which is annoying for power users.
2. **Soft clear with undo**: clear, but show a small "Restore
   previous value" affordance for ~3 s. More forgiving; more code.
3. **Validate-and-warn**: keep the value but show an inline error
   when it doesn't match the new type's value space. Most surgical;
   requires a per-type validator.
4. **Rebuild the field per type**: each type has a purpose-built
   widget (autocomplete-from-fixed-set, free-text, hex code, GPIO
   number); the field always starts empty when the type changes
   because it is a different widget instance. Cleanest UX-wise;
   may be what the codebase is already half-doing for Pin types
   today.

Tradeoffs: option 4 + option 1 together is probably the right
combination — clear when the new type uses a different widget, keep
when the user toggles between two named-key-style types.

---

## Out of Scope

- The TASK-277 hint-text fix for "Key (raw HID)" — that is a separate,
  narrower defect.
- The broader idea-039 question of whether "Key (raw HID)" should
  accept USB HID Usage IDs.
- Any AE-U1/AE-U2 usability findings — those are TASK-278.

---

## References

- TASK-155 round 3 (2026-04-27), AE-04 / AE-06 / AE-07 recipe
  observations.
- TASK-277 (misleading raw-HID hint).
- TASK-278 (Action Editor usability session).
