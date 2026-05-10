# Key Reference

Complete listing of every valid `value` string per action type.

Use this file alongside `data/profiles.json` when writing button mappings.

---

## SendStringAction

Sends a literal text string — every character is typed in sequence.

```json
{ "type": "SendStringAction", "name": "Undo Take", "value": "ctrl+z" }
```

`value` is any printable string. Special sequences like `ctrl+z` are sent
character-by-character; the host OS handles modifier interpretation.

---

## SendCharAction

Sends a single key by name, or a single printable ASCII character.

```json
{ "type": "SendCharAction", "name": "Next Page", "value": "KEY_PAGE_DOWN" }
{ "type": "SendCharAction", "name": "Speed Up",  "value": "]" }
```

### Arrow keys

| Value | Key |
|-------|-----|
| `KEY_LEFT_ARROW` / `LEFT_ARROW` | ← |
| `KEY_RIGHT_ARROW` / `RIGHT_ARROW` | → |
| `KEY_UP_ARROW` / `UP_ARROW` | ↑ |
| `KEY_DOWN_ARROW` / `DOWN_ARROW` | ↓ |

### Navigation

| Value | Key |
|-------|-----|
| `KEY_PAGE_UP` | Page Up |
| `KEY_PAGE_DOWN` | Page Down |
| `KEY_HOME` | Home |
| `KEY_END` | End |
| `KEY_INSERT` | Insert |
| `KEY_DELETE` | Delete |

### Control

| Value | Key |
|-------|-----|
| `KEY_BACKSPACE` | Backspace |
| `KEY_TAB` | Tab |
| `KEY_RETURN` / `KEY_ENTER` | Enter/Return |
| `KEY_ESC` | Escape |
| `KEY_CAPS_LOCK` | Caps Lock |
| `KEY_PRINTSCREEN` | Print Screen |

### Modifiers

| Value | Key |
|-------|-----|
| `KEY_LEFT_CTRL` | Left Control |
| `KEY_LEFT_SHIFT` | Left Shift |
| `KEY_LEFT_ALT` | Left Alt |
| `KEY_LEFT_GUI` | Left GUI (Win/Cmd) |
| `KEY_RIGHT_CTRL` | Right Control |
| `KEY_RIGHT_SHIFT` | Right Shift |
| `KEY_RIGHT_ALT` | Right Alt |
| `KEY_RIGHT_GUI` | Right GUI |

### Function keys

| Value | Alias (no KEY_ prefix) | Key |
|-------|------------------------|-----|
| `KEY_F1` | `F1` | F1 |
| `KEY_F2` | `F2` | F2 |
| `KEY_F3` | `F3` | F3 |
| `KEY_F4` | `F4` | F4 |
| `KEY_F5` | `F5` | F5 |
| `KEY_F6` | `F6` | F6 |
| `KEY_F7` | `F7` | F7 |
| `KEY_F8` | `F8` | F8 |
| `KEY_F9` | `F9` | F9 |
| `KEY_F10` | `F10` | F10 |
| `KEY_F11` | `F11` | F11 |
| `KEY_F12` | `F12` | F12 |
| `KEY_F13` | `F13` | F13 |
| `KEY_F14` | `F14` | F14 |
| `KEY_F15` | `F15` | F15 |
| `KEY_F16` | `F16` | F16 |
| `KEY_F17` | `F17` | F17 |
| `KEY_F18` | `F18` | F18 |
| `KEY_F19` | `F19` | F19 |
| `KEY_F20` | `F20` | F20 |
| `KEY_F21` | `F21` | F21 |
| `KEY_F22` | `F22` | F22 |
| `KEY_F23` | `F23` | F23 |
| `KEY_F24` | `F24` | F24 |

F13–F24 are useful for DAW shortcuts (OBS, Ableton, etc.) that would otherwise
conflict with standard keyboard input.

### Numpad

| Value | Key |
|-------|-----|
| `KEY_NUM_0` | Numpad 0 |
| `KEY_NUM_1` | Numpad 1 |
| `KEY_NUM_2` | Numpad 2 |
| `KEY_NUM_3` | Numpad 3 |
| `KEY_NUM_4` | Numpad 4 |
| `KEY_NUM_5` | Numpad 5 |
| `KEY_NUM_6` | Numpad 6 |
| `KEY_NUM_7` | Numpad 7 |
| `KEY_NUM_8` | Numpad 8 |
| `KEY_NUM_9` | Numpad 9 |
| `KEY_NUM_SLASH` | Numpad / |
| `KEY_NUM_ASTERISK` | Numpad * |
| `KEY_NUM_MINUS` | Numpad − |
| `KEY_NUM_PLUS` | Numpad + |
| `KEY_NUM_ENTER` | Numpad Enter |
| `KEY_NUM_PERIOD` | Numpad . |

> Note: `KEY_NUM_LOCK`, `KEY_SCROLL_LOCK`, and `KEY_PAUSE` are **not** defined
> in the BleKeyboard library and cannot be used.

---

## SendKeyAction

Sends a USB HID key code. Accepts the same key names as `SendCharAction`.

```json
{ "type": "SendKeyAction", "name": "Escape", "value": "KEY_ESC" }
```

Use `SendKeyAction` when you need a raw HID key event rather than a typed character.

---

## SendMediaKeyAction

Sends a media/consumer control key.

```json
{ "type": "SendMediaKeyAction", "name": "Play/Pause", "value": "MEDIA_PLAY_PAUSE" }
```

### Media keys

| Value | Description |
|-------|-------------|
| `MEDIA_NEXT_TRACK` / `KEY_MEDIA_NEXT_TRACK` | Next track |
| `MEDIA_PREVIOUS_TRACK` / `KEY_MEDIA_PREVIOUS_TRACK` | Previous track |
| `MEDIA_STOP` / `KEY_MEDIA_STOP` | Stop |
| `MEDIA_PLAY_PAUSE` / `KEY_MEDIA_PLAY_PAUSE` | Play / Pause |
| `MEDIA_MUTE` / `KEY_MEDIA_MUTE` | Mute |
| `MEDIA_VOLUME_UP` / `KEY_MEDIA_VOLUME_UP` / `KEY_VOLUME_UP` | Volume up |
| `MEDIA_VOLUME_DOWN` / `KEY_MEDIA_VOLUME_DOWN` / `KEY_VOLUME_DOWN` | Volume down |
| `KEY_MEDIA_WWW_HOME` | Browser home |
| `KEY_MEDIA_WWW_BACK` | Browser back |
| `KEY_MEDIA_WWW_STOP` | Browser stop |
| `KEY_MEDIA_WWW_SEARCH` | Browser search |
| `KEY_MEDIA_WWW_BOOKMARKS` | Browser bookmarks |
| `KEY_MEDIA_CALCULATOR` | Calculator |
| `KEY_MEDIA_EMAIL_READER` | Email client |
| `KEY_MEDIA_LOCAL_MACHINE_BROWSER` | File browser |
| `KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION` | Media settings |

---

## SerialOutputAction

Writes a string to the Serial Monitor. Does not send any key to the host device.
Useful for testing and debugging.

```json
{ "type": "SerialOutputAction", "name": "Ping Pedal", "value": "Pedal alive." }
```

---

## DelayedAction

Waits, then executes any other action. The power LED blinks while the countdown runs.

```json
{
  "type": "DelayedAction",
  "name": "Delayed Shutter",
  "delayMs": 3000,
  "action": { "type": "SendMediaKeyAction", "value": "KEY_VOLUME_UP" }
}
```

`delayMs` is the wait time in milliseconds. The nested `action` is any other action type.

---

## Long Press and Double Press

Any button action can optionally include a `longPress` and/or `doublePress` sub-action.
These are triggered by different physical gestures on the same button:

| Gesture | Threshold | JSON key |
|---------|-----------|----------|
| Short tap | < 500 ms | *(primary action)* |
| Hold | ≥ 500 ms | `longPress` |
| Two taps within 300 ms | — | `doublePress` |

Priority: double press is detected first. If confirmed, the single-press action does **not** also fire.

```json
"A": {
  "type": "SendCharAction", "value": "KEY_F1", "name": "Play",
  "longPress":   { "type": "SendMediaKeyAction", "value": "MEDIA_STOP",  "name": "Stop" },
  "doublePress": { "type": "SendCharAction",     "value": "KEY_F2",      "name": "Record" }
}
```

Both keys are optional. The nested action object uses the same format as any primary action.
The long-press threshold (500 ms) and double-press window (300 ms) are fixed in firmware.

---

## Profile-Independent Actions

`profiles.json` may contain an optional top-level `independentActions` block.
Its actions fire on every button event **alongside** the active profile's actions —
regardless of which profile is selected. Switching profiles never affects this block.

The block uses the same shape as a profile's `buttons` block. A missing entry for a
button is a no-op; an entirely missing block leaves behaviour identical to today.

When both an independent action and a profile action exist for the same button event,
the independent action fires first.

```json
{
  "profiles": [
    {
      "name": "Studio",
      "buttons": {
        "A": { "type": "SendCharAction", "value": "KEY_F1", "name": "Play" }
      }
    }
  ],
  "independentActions": {
    "A": { "type": "PinHighWhilePressedAction", "pin": 5, "name": "Tally Light" }
  }
}
```

In this example, button A always drives pin 5 high while held — even if you switch to
a different profile that maps A to something else. The profile's per-button action
still fires on top.
