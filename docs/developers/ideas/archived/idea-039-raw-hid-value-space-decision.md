---
id: IDEA-039
title: Action Editor "Key (raw HID)" — pick a value space (USB Usage IDs vs firmware-internal codes) and align app, firmware, and docs
category: firmware
description: Decide whether the "Key (raw HID)" numeric value is the standard USB HID Usage ID (e.g. 0x28 = Enter) or the firmware-internal byte the BleKeyboard library expects (e.g. 0xB0 = Enter), then align the app field, firmware parser, schema, and docs to a single answer.
---

## Archive Reason

2026-05-01 — Solution looking for a problem — no real user demand.

## Background

Surfaced while closing TASK-257 (2026-04-26) and updating TASK-155's
AE-07 row.

The Action Editor's "Key (raw HID)" option lets the user enter a hex
value (e.g. `0x28`) which becomes a `SendKeyAction.value` in the
profile JSON. As of TASK-257, the firmware parser correctly converts
hex/decimal strings to `uint8_t` HID codes — but the **value space** is
ambiguous:

| Source | What "0x28" means |
|---|---|
| USB HID specification (Usage Page 0x07, Usage IDs) | Keyboard `Return (Enter)` |
| This firmware's `i_ble_keyboard.h` constants | An unmapped value (between `0x00` and the modifier range that starts at `0x80`) |

The firmware's `i_ble_keyboard.h` defines its own byte layout:
modifiers at `0x80`+, function/special keys at `0xB0`+/`0xC0`+/etc.
So `KEY_RETURN = 0xB0` in this codebase, **not** `0x28`. That
internally-shifted byte is what gets fed to
`bleKeyboard->write(uint8_t)` in `SendKeyAction::send()`. (The byte
layout matches what the upstream BleKeyboard Arduino library expects —
that's why it exists, not because we picked it.)

Concretely, today:

- A user who knows USB HID and enters `0x28` expecting Enter will get
  silence (or a wrong key, depending on the value).
- A user who reads the firmware headers and enters `0xB0` will get
  Enter.
- The "Key (named)" path (`KEY_RETURN`) hides this entirely — it just
  works.

So "raw HID" today means "the byte the firmware will write to the BLE
HID report after its internal mapping," **not** the canonical USB HID
Usage ID. That is surprising to anyone with HID background, and it is
also a footgun for AE-07 in TASK-155 (where the test plan originally
said `0x28`).

## Options

### Option A — Document the current scheme and rename the field

Lowest-effort, highest-honesty option. The field is labeled "Key (raw
HID)" today but it is really "raw firmware-internal code." Rename it
in the app to something like "Key (raw byte)" or "Key (advanced)",
and link to a reference table in the app's How-To or info screen
listing the firmware-internal codes. The firmware stays as-is.

- **Pro:** No firmware changes, no schema migration, no breakage of
  existing user profiles.
- **Con:** Makes the project's "raw HID" semantics divergent from the
  rest of the HID universe. Anyone copying values from a USB HID
  reference table or from another keyboard project will get the wrong
  result on first try.

### Option B — Switch the value space to canonical USB HID Usage IDs

The field accepts USB HID Usage IDs (`0x28` for Enter, `0x3A` for F1,
etc.). The firmware translates these into its internal byte before
calling `bleKeyboard->write()`. The named-key path (`KEY_RETURN`)
keeps producing the same wire-level result as today — only the numeric
input convention changes.

- **Pro:** "Raw HID" finally means raw HID. Documentation, training,
  and HID references all line up.
- **Con:** Existing user profiles that already store values like
  `"0xB0"` would break on next firmware load. Needs a migration path
  (heuristic: if value ≥ 0x80 and not in the small-known USB-Usage-ID
  range, treat as legacy firmware-internal and remap; otherwise treat
  as new USB-Usage-ID). Or just version-bump the schema.
- **Note:** The translation table is non-trivial (modifier handling,
  the `0xE0–0xE7` modifier byte vs the firmware's `0x80–0x87` block,
  etc.) and needs careful test coverage.

### Option C — Hide the field entirely

The "Key (raw HID)" option is rare-by-design — almost everyone uses
"Key (named)". If telemetry/usage shows nobody picks "raw HID," the
cheapest fix is to remove it from the app's action-type picker and
keep it only as a JSON-edit-mode escape hatch.

- **Pro:** Zero confusion for end users. Smallest surface area.
- **Con:** Removes a power-user feature without replacement. May make
  it harder to express keys that don't have a friendly name yet.

## Decision criteria

- Who actually uses "Key (raw HID)" today? (Probably: the project
  owner during testing. Worth a quick `grep` of any sample profiles
  and a moment of honesty about real demand.)
- Are existing user profiles in the wild already using hex values?
  (TASK-257 was just merged; the answer is likely "no, because it
  didn't work until now.")
- How much do we care about the "principle of least surprise" for
  HID-literate users? On a hobby pedal, probably less than on a
  general-purpose keyboard firmware.

A reasonable lean given those: **Option A** (rename + document) is
the cheapest and probably right. **Option B** is right "if we ever
ship to people who already know HID and would notice the divergence."
**Option C** is fine if "raw HID" turns out to be a vestige nobody
uses.

## Proposed next step

Pick a sitting to decide A/B/C. If A: file a small follow-up to rename
the field and add a reference table to the app's info/about page (see
idea-037). If B: file a TASK with the migration strategy and the
USB-Usage-ID → firmware-internal translation table. If C: file a
TASK to drop the option from the picker.

## Out of scope

- Re-deriving the firmware-internal mapping table — it already exists
  in `i_ble_keyboard.h`; if Option B is picked, the USB-Usage-ID →
  firmware-internal translation can be sourced from the BleKeyboard
  library headers or USB HID Usage Tables 1.21 directly.
- Whether "Key (named)" and "Key (raw HID)" should ever be merged
  into one input — that was deferred in TASK-257 already and is a
  separate UX question.

## References

- TASK-257 (closed 2026-04-26) — added hex/decimal parsing.
- TASK-155 AE-07 — uses `0xB0` for the test value with a Notes-section
  pointer to this idea.
- `lib/PedalLogic/include/i_ble_keyboard.h` — the firmware-internal
  code table.
- USB HID Usage Tables 1.21 §10 ("Keyboard/Keypad Page (0x07)") for
  the canonical Usage IDs.
