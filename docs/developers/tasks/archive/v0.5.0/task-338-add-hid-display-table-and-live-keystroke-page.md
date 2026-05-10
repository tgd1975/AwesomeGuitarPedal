---
id: TASK-338
title: Add HID-usage display lookup table (generated) and Live-keystroke page
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Medium (2-8h)
effort_actual: Medium (2-8h)
complexity: Medium
human-in-loop: Clarification
epic: app-content-pages
order: 9
---

## Description

Two coupled deliverables — the lookup table is only useful with at
least one consumer, and the live-keystroke page is the most
illustrative consumer:

### A. Generated HID usage display table

Single source of truth mapping HID usage ID → display descriptor,
owned by the app and **generated** rather than hand-typed:

```dart
class KeyDisplay {
  final String label;        // "Page Up", "Volume Up"
  final String? iconName;    // Material Symbols name
  final String? glyph;       // "⏎", "⇧"
  final KeyCategory category;
}

const Map<int, KeyDisplay> kHidUsageDisplay = { /* generated */ };
```

A small build-time script ingests the public HID Usage Tables
(Section 10, Keyboard/Keypad Page 0x07; Section 15, Consumer Page
0x0C) and emits the Dart map, with a hand-maintained overlay for
icons/glyphs the spec doesn't carry. `KeyCategory` lets future UIs
group/colour and filter.

Why this shape — same table will eventually drive: the Live-keystroke
page, the profile editor's button-action picker, the Connected-Pedal
"current config" readback display, and any future "preview what this
profile does" view.

### B. Live-keystroke diagnostic page

Scrolling tape of "key chips" newest-first. Each chip applies the
three rendering tiers from IDEA-037: printable glyph → named-special
icon+label from the table → unmapped-HID hex fallback. Modifier+key
combos collapse into one chip (`Ctrl + Shift + S`). Repeated events
collapse with a `×N` counter. "Clear" and "Copy as text" actions on
the tape.

**Event source — the host OS, not BLE.** When the pedal is paired
over BLE, both Android and iOS route its HID keystrokes to the
focused widget exactly as they would for any external Bluetooth
keyboard. The page hosts a focused `KeyboardListener` /
`HardwareKeyboard` widget that consumes Flutter `KeyEvent`s. No
firmware change, no new BLE characteristic, no special permission
beyond keyboard input focus.

Each `KeyEvent` carries `logicalKey` and `physicalKey` whose USB HID
usage codes are the same vocabulary as the pedal's HID descriptors,
so the same `kHidUsageDisplay` table covers chip rendering for both
"normal" keyboards and pedal output.

## Acceptance Criteria

- [x] Build-time generator script lives under `scripts/` and re-runs
      idempotently (output diff = 0 on second run with no spec
      changes). *(`scripts/generate_hid_display_table.py`. Idempotency
      enforced by `scripts/tests/test_generate_hid_display_table.py`
      which runs the generator twice and asserts byte-equal output +
      "already up to date" on second run.)*
- [x] Generated `kHidUsageDisplay` covers Keyboard/Keypad page (0x07)
      and Consumer page (0x0C) at minimum. *(138 entries — full a-z,
      0-9, F1-F24, edit/nav/locking/modifier rows from 0x07 plus
      media + browser entries from 0x0C.)*
- [x] Hand-maintained icon/glyph overlay is a separate file the
      generator merges in, not embedded in the generated output.
      *(`scripts/hid_display_data/overlay.yaml` — keyed by
      `<page>:<usage_id>`, can override label and add glyph/icon.)*
- [x] Live-keystroke page renders the three rendering tiers correctly
      (test fixtures: `a`, `Enter`, an unmapped usage). *(Tier 1:
      printable character via `KeyEvent.character`. Tier 2: table
      lookup with glyph/icon (verified via Enter + ⏎ glyph). Tier 3:
      hex fallback `0xPP:0xUUUU`.)*
- [x] Modifier+key collapse and repeated-event `×N` collapse both
      verified in tests. *(Down ×3 → "×3" rendered. Ctrl+Shift+S →
      `Ctrl`/`Shift` mod chips prefix the `s` key chip.)*
- [x] "Copy as text" emits the documented plain-text form
      (`Ctrl+Shift+S, Enter, Vol+ ×3`). *(Plain-text round-trip
      verified by the clipboard mock — `Ctrl+Shift+s, Enter, Down ×3`
      lands in the clipboard.)*

## Implementation notes

- **Event source: the host OS, not BLE.** Originally specced as a
  new BLE characteristic (which would have required TASK-337
  firmware work); corrected at activation — Android and iOS already
  route the pedal's HID keystrokes to a focused widget, exactly as
  for any external Bluetooth keyboard. Same vocabulary: Flutter's
  `KeyEvent.physicalKey.usbHidUsage` is the same packed
  `(page << 16) | usage_id` form as our table key.
- **Modifier handling:** standalone modifier presses are dropped
  (only `_chipForEvent` returns null for usage 0xE0–0xE7 on page
  0x07). When a real key follows, the active modifiers from
  `HardwareKeyboard.instance.logicalKeysPressed` are folded into the
  next chip's `modifiers:` list.
- **Repeated-event collapse:** if the next event has the same
  `usbHidUsage` *and* the same `modifiers`, increment `count` on
  the head chip instead of inserting a new one. Different modifiers
  break the streak (so `S` after `Ctrl+S` opens a new chip).
- **No build-time codegen integration in CI yet** — running the
  generator is a manual `python scripts/generate_hid_display_table.py`.
  Could be wired into the pre-commit hook later if YAML edits
  start drifting from the generated file; for now the Python test
  catches stale output by failing on first commit.
- All 159 app tests pass; both Python tests pass; `flutter analyze`
  clean.

## Prerequisites

None — host-side only. Originally listed TASK-337 as a prerequisite
on the assumption that the live-keystroke event source would be a
new BLE characteristic; corrected at activation: the OS already
routes pedal-emitted HID keystrokes to a focused widget, so no
firmware change is needed.

## Test Plan

**Host tests** (Flutter widget + unit tests):

- `app/test/unit/hid_display_table_test.dart` — generated table is
  non-empty for pages 0x07 and 0x0C; overlay entries override
  generated defaults.
- `app/test/widget/live_keystroke_screen_test.dart` — three rendering
  tiers; modifier+key collapse; repeated-event `×N`; copy-as-text
  format. Drive `KeyEvent`s into the page via
  `tester.sendKeyEvent` / `WidgetTester.sendKeyDownEvent`.

No on-device or firmware-side tests required.

## Documentation

- `docs/developers/ARCHITECTURE.md` — note the generated HID display
  table and its overlay contract. *(Optional; the table is
  self-documenting and the generator script's docstring is the
  authoritative description.)*

## Notes

- Bundling the table with the live-keystroke page is intentional: the
  table without a consumer is dead weight, and the page without the
  table degenerates into a string of hex codes.
- Localizable: `label` is the only translated string; icons/glyphs
  are language-neutral.
