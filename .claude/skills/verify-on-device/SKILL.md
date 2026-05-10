---
name: verify-on-device
description: Drive the Flutter app on a connected Android device end-to-end via adb to verify a feature-test or defect-fix scenario without human intervention. Replaces "tap through it on Pixel 9" steps in TASK-156-style feature tests and defect ACs.
---

# verify-on-device

The user invokes this as `/verify-on-device <TASK-ID> <SCENARIO-ID>`,
e.g. `/verify-on-device TASK-261 UP-03` or
`/verify-on-device TASK-263 SC-01`.

The skill drives the installed app on a USB-connected Android device
via `adb` (taps, text input, uiautomator XML dumps, logcat, screencap),
asserts the scenario's success criterion, and reports PASS / FAIL with
evidence screenshots. On PASS for a defect-verification task, append a
one-line note to the task body and stop. On FAIL, print the captured
UI tree + logcat tail and stop — do not retry blindly, do not edit
the task.

The intent is: **assume the pedal is powered and advertising, and the
phone is plugged in with USB debugging on.** Run the recipe straight
through. Only stop and ask the user when a pre-flight check or scan
step proves the assumption wrong — never ask "is the pedal on?" up
front.

## Pre-flight

Default assumption: phone connected with USB debugging, Bluetooth on,
pedal powered and advertising. Run these checks in parallel and only
involve the user when one **fails**:

1. `adb devices` shows exactly one device in state `device`.
   Capture the serial as `$DEV`.
   - 0 devices → ask the user: "No Android device detected over adb.
     Plug in the phone and enable USB debugging, then say go."
   - >1 devices → ask the user: "Multiple devices visible: `<list>`.
     Which one should I drive? (or set `ANDROID_SERIAL=<serial>`)"
   - 1 device → proceed silently, do not ask for confirmation.
2. `adb -s $DEV shell getprop ro.product.model` and `ro.build.version.release`
   — record for the verification note. No user interaction.
3. `adb -s $DEV shell settings get global bluetooth_on`. If `1`, proceed.
   If `0`, attempt `adb -s $DEV shell svc bluetooth enable` and re-check
   after 2 s; only ask the user if that re-check still returns `0`
   ("Couldn't enable Bluetooth from adb — please toggle it on the phone").
4. `command -v flutter` resolves. If missing → invoke `/check-tool flutter`.
5. **Pedal state is assumed, not asked.** Do not prompt the user to
   confirm the pedal is powered or advertising. If a scenario's scan
   step (`wait_for_text "AwesomeStudio" …`) times out, *that* is when
   you surface the assumption failure: report
   "pedal not advertising / out of range — power-cycle the pedal and
   re-run" and stop. Same rule for any later step that proves the pedal
   isn't ready (e.g. connect failing): treat the failed step as the
   signal, not a precautionary up-front question.

## Build & install (cached)

```
APP=app
PKG=$(grep -m1 "applicationId" $APP/android/app/build.gradle | awk -F'"' '{print $2}')
APK=$APP/build/app/outputs/flutter-apk/app-release.apk
```

- If `$APK` exists and is newer than every file under `$APP/lib/`,
  skip the build (fast path).
- Otherwise: `cd $APP && flutter build apk --release` (5–8 min cold).
- Install: `adb -s $DEV install -r -t "$APK"`. The `-r` reinstalls,
  `-t` allows test packages.
- Grant runtime permissions up front so no system dialog interrupts
  the flow:

  ```
  for p in BLUETOOTH_CONNECT BLUETOOTH_SCAN ACCESS_FINE_LOCATION; do
    adb -s $DEV shell pm grant $PKG android.permission.$p || true
  done
  ```

- Force-stop any prior instance: `adb -s $DEV shell am force-stop $PKG`.
- Launch the main activity: `adb -s $DEV shell monkey -p $PKG -c android.intent.category.LAUNCHER 1`.
- Wait for the home screen to appear (see helpers below).

## ADB driving primitives (use these — do NOT `sleep` and tap blindly)

```bash
# Dump the current UI tree to a temp XML on the host.
ui_dump() {
  adb -s $DEV shell uiautomator dump /sdcard/ui.xml >/dev/null
  adb -s $DEV pull /sdcard/ui.xml /tmp/ui.xml >/dev/null
  cat /tmp/ui.xml
}

# Wait until $1 (a substring) appears in the UI tree, up to ${2:-15}s.
# Returns 0 on found, 1 on timeout.
wait_for_text() {
  local needle="$1" max="${2:-15}" i=0
  while [ $i -lt $max ]; do
    if ui_dump | grep -F -q "$needle"; then return 0; fi
    sleep 1; i=$((i+1))
  done
  return 1
}

# Tap the centre of the first node whose `text` or `content-desc` matches $1.
tap_text() {
  local needle="$1"
  local bounds=$(ui_dump | python3 -c "
import sys, re, xml.etree.ElementTree as ET
needle = sys.argv[1]
tree = ET.fromstring(sys.stdin.read())
for n in tree.iter('node'):
    t, d = n.get('text',''), n.get('content-desc','')
    if needle in t or needle in d:
        print(n.get('bounds')); break
" "$needle")
  [ -z "$bounds" ] && { echo "tap_text: '$needle' not found"; return 1; }
  # bounds = "[x1,y1][x2,y2]"
  read x y <<<"$(python3 -c "
import sys, re
m = re.match(r'\[(\d+),(\d+)\]\[(\d+),(\d+)\]', sys.argv[1])
x1,y1,x2,y2 = map(int, m.groups())
print((x1+x2)//2, (y1+y2)//2)
" "$bounds")"
  adb -s $DEV shell input tap "$x" "$y"
}

# Capture a screenshot for evidence and emit its path.
# Uses scripts/screenshot.py (allowlisted as `Bash(python3 scripts/screenshot.py *)`)
# instead of `adb exec-out screencap -p > $out`, because the redirect makes the
# raw adb form fall outside the matcher and trigger a prompt every time.
shot() {
  local name="$1"
  local out="/tmp/verify_${TASK_ID}_${name}.png"
  python3 scripts/screenshot.py "$out" --serial "$DEV" >/dev/null
  echo "$out"
}

# Recent flutter / app logs since the start of the run.
logcat_since() { adb -s $DEV logcat -d -t "${1:-200}" | grep -E "flutter|$PKG" || true; }

# Replace the contents of the currently-focused EditText. Unlike `input text`,
# this preserves characters that `input text` mangles ('+' becomes space,
# spaces require '%s', etc.) — important for AE-06 ("ctrl+z") and AE-03
# (display name "Next Page").
set_text() {
  local s="$1"
  # Escape backslashes and double-quotes for the shell.
  local esc=$(printf '%s' "$s" | sed 's/\\/\\\\/g; s/"/\\"/g')
  adb -s $DEV shell input text "$(printf '%s' "$esc" | sed 's/ /%s/g')"
}

# Tap the EditText whose current text matches $1 to focus it, then clear and
# replace it with $2. The Action Editor has multiple EditTexts (value, name,
# pin number); the matcher uses the visible text to disambiguate.
focus_and_set() {
  local match="$1" replacement="$2"
  local bounds=$(ui_dump | python3 -c "
import sys, xml.etree.ElementTree as ET
needle = sys.argv[1]
tree = ET.fromstring(sys.stdin.read())
for n in tree.iter('node'):
    if n.get('class') == 'android.widget.EditText' and needle in n.get('text',''):
        print(n.get('bounds')); break
" "$match")
  [ -z "$bounds" ] && { echo "focus_and_set: EditText with '$match' not found"; return 1; }
  local x y
  read x y <<<"$(python3 -c "
import re,sys
m = re.match(r'\[(\d+),(\d+)\]\[(\d+),(\d+)\]', sys.argv[1])
x1,y1,x2,y2 = map(int, m.groups()); print((x1+x2)//2, (y1+y2)//2)
" "$bounds")"
  adb -s $DEV shell input tap "$x" "$y"
  sleep 0.3
  # Select all + delete, then type the replacement.
  adb -s $DEV shell input keyevent KEYCODE_MOVE_END
  for _ in $(seq 1 100); do adb -s $DEV shell input keyevent KEYCODE_DEL; done
  set_text "$replacement"
}

# Read the full Profile List → JSON Preview text. Must be invoked while on
# the Profile List screen (i.e. after `nav_to_profile_list`). Returns the
# JSON as stdout. Side-effect: leaves the app on the JSON Preview screen.
read_json_preview() {
  tap_text "JSON Preview" || return 1
  sleep 1
  ui_dump | python3 -c "
import sys, xml.etree.ElementTree as ET
tree = ET.fromstring(sys.stdin.read())
for n in tree.iter('node'):
    t = n.get('text','')
    if t.lstrip().startswith('{') and '\"profiles\"' in t:
        print(t); break
"
}

# Navigate to the Profile List from anywhere by pressing BACK until we see
# the "JSON Preview" toolbar button (max 5 presses).
nav_to_profile_list() {
  for _ in 1 2 3 4 5; do
    if ui_dump | grep -F -q 'content-desc="JSON Preview"'; then return 0; fi
    adb -s $DEV shell input keyevent KEYCODE_BACK
    sleep 0.5
  done
  return 1
}

# Dismiss notification heads-up popups that overlay the app and steal taps.
# WhatsApp, Signal, calendar reminders, etc. drop a Material card at the top
# of the screen and absorb input until they auto-hide (~5 s) or are swiped.
# Call this before any tap sequence that runs while the device is in real-
# world use. Idempotent — safe to call when no notification is showing.
dismiss_notifications() {
  adb -s $DEV shell cmd statusbar collapse >/dev/null 2>&1 || true
}

# Force-stop the app, clear its data + permissions, then re-grant the
# Bluetooth permissions so the next launch starts from a clean state
# WITHOUT the runtime permission dialog. Use this when you need a known
# baseline — especially when a previous test denied a permission, because
# the in-memory permission cache survives a `pm grant` until the process
# is killed.
reset_app_state() {
  adb -s $DEV shell am force-stop "$PKG"
  adb -s $DEV shell pm clear "$PKG"
  adb -s $DEV shell pm grant "$PKG" android.permission.BLUETOOTH_SCAN
  adb -s $DEV shell pm grant "$PKG" android.permission.BLUETOOTH_CONNECT
}
```

## Pitfalls — things that have wasted time

These are not theoretical; each cost a real chunk of a session. Treat them
as automatic checks before debugging "the tap didn't work."

- **A notification heads-up overlay was eating the tap.** When typing into
  a dialog or tapping a card, an incoming WhatsApp / Signal / calendar
  popup will drop a Material card at the top of the screen and absorb
  every tap until it auto-hides. The screenshot shows the app, but the
  tap target is wrong. Call `dismiss_notifications` before any sequence
  that runs while the device is in real-world use.
- **Permission state survives `pm grant` until the process restarts.**
  After `pm revoke … BLUETOOTH_SCAN` and tapping "Don't allow" in the
  permission dialog, the scanner caches the denied state. A subsequent
  `pm grant` does not flip it back — the next launch still shows
  "Bluetooth permission denied." Use `reset_app_state` (which `pm clear`s
  the runtime cache) when you need a clean baseline.
- **uiautomator emits two flavors of `<node>`.** Container rows (a card
  with a child Delete button) write `<node …>…</node>`; leaf widgets write
  `<node … />`. A regex like `<node[^>]+/>` matches only leaves and silently
  skips the row you actually want to tap, so `tap_text "01 Score Navigator"`
  finds the trash icon instead of the row. The `tap_text` helper above
  uses `ElementTree`, which handles both — do not reach for an ad-hoc
  regex parser when the helper exists.

## Growing the primitive set — extend, don't bypass

If you are about to drive the app, the phone, or the pedal via `adb`
(or `flutter`, `bluetoothctl`, `pio`, etc.) and the operation you
need is **not** already a helper above, **add it as a helper before
using it** — do not inline the raw command into a scenario, and do
not run the raw command outside the skill "just this once."

The point of this skill is to be the single source of truth for
"how do we drive the app on a real device." Every ad-hoc one-off
adb invocation that lives outside it is friction the next person
(or the next session) will have to re-derive. Examples of additions
that belong here as helpers, not as inline commands:

- Reading the Android clipboard (`adb shell cmd clipboard …`).
- Pushing/pulling fixture files (`adb push fixtures/…`).
- Toggling Bluetooth, Wi-Fi, or airplane mode.
- Reading focused-window / current-activity for navigation assertions.
- Triggering or listening for a BLE pairing PIN dialog.
- `bluetoothctl` flows on the host for non-Android pairing tests.
- `pio device monitor` snapshots for asserting firmware serial output
  during an end-to-end test.
- Anything that uses `adb forward` or a screen-recording capture.

Rule of thumb: if you wrote the same `adb …` command in two scenarios,
or you're writing one whose shape is non-obvious (escaping, polling,
parsing), promote it to a function in the primitives section above
with a one-line comment on what it does and what surprises it hides.

This applies to the pedal too. The skill currently only drives the
phone, but pedal-side primitives (e.g. `monitor_serial_until`,
`assert_pedal_connected`) belong here as soon as we need them — not
in a sibling skill or a one-off bash script.

## Permission allowlist — keep commands atomic, not chained

Claude Code's Bash permission rules match the **whole literal command
string** against a single allowlist entry. Pipelines and chained
statements break that match and fall back to interactive approval —
not because the operations are unsafe, but because the matcher cannot
prove from one rule that the entire chain is covered.

Concretely, all four of these will prompt **even if every individual
command is allowlisted**:

```bash
adb -s $DEV shell input tap 540 863 && sleep 2 && adb -s $DEV exec-out screencap -p > /tmp/x.png
sleep 1; adb -s $DEV shell uiautomator dump /sdcard/ui.xml
for i in 1 2 3; do adb -s $DEV shell input tap 100 200; sleep 1; done
adb -s $DEV shell uiautomator dump /sdcard/ui.xml | grep -q "Connect"
```

The allowlist sees `Bash(adb -s $DEV shell input tap 540 863 && sleep 2 && ...)` and finds no rule that starts that way, so it asks.

**Rules of thumb when writing scenarios:**

- **One operation per Bash call.** Tap, then dump, then pull, then
  inspect — each in its own tool invocation. Yes, it is more verbose;
  yes, the per-call overhead is real. It is the only shape that runs
  unattended.
- **Push polling loops into helper scripts.** If a scenario needs a
  for-loop or while-loop (e.g. "poll the UI every 0.5 s for up to
  5 s"), put the loop in a shell script under
  `.claude/skills/verify-on-device/helpers/` and add a single
  allowlist entry for invoking that script (e.g.
  `Bash(.claude/skills/verify-on-device/helpers/poll_for_text.sh *)`).
  The whole loop is then one allowlisted command from the matcher's
  point of view.
- **Do not bargain with the matcher** by adding broad shell-builtin
  rules like `Bash(for *)`, `Bash(while *)`, `Bash(awk *)`,
  `Bash(sed *)`, or `Bash(python3 -c *)`. Those rules turn the
  allowlist into a permission-bypass: any prompt-injected payload
  inside a tool result that starts with `for` or `python3 -c` would
  match. Keep the allowlist **scoped to specific tools and specific
  argument shapes**, even if it costs an extra prompt or two.
- **Redirects (`>`, `>>`, `|`) are part of the literal command.** A
  rule like `Bash(adb -s * exec-out screencap*)` does **not** match
  `adb -s $DEV exec-out screencap -p > /tmp/foo.png`. Either include
  the redirect target shape in the rule
  (`Bash(adb -s * exec-out screencap -p > /tmp/*)`) or write the
  output via a tool that takes a filename argument and avoids
  redirection.

If a scenario keeps prompting on a particular shape, the right fix
is almost always "extract a helper script and allowlist that script,"
not "broaden the allowlist."

## Scenario catalog

The skill knows a fixed set of scenarios. Each is a sub-routine: a
sequence of `tap_text` / `wait_for_text` / `shot` calls, ending with
an explicit pass/fail assertion. To add a new scenario, add a section
below — keep recipes inline; do not parse FEATURE_TEST_PLAN.md.

If the requested `<scenario-id>` is not in the catalog, list the
known ones and stop.

### UP-03 — Upload Profiles, happy path (Upload screen)

Verifies: a valid profile JSON uploads to a connected pedal and the
"Upload successful!" SnackBar appears. Underlying defect: TASK-261
chunk size cap. Cross-ref TASK-266 (swallowed exception).

Steps:

1. Wait for the home screen: `wait_for_text "Connect to pedal" 10`.
2. `tap_text "Connect to pedal"` → scanner opens.
3. `wait_for_text "AwesomeStudio" 20` (pedal name prefix is
   `AwesomeStudio` — see [app/lib/constants/ble_constants.dart](app/lib/constants/ble_constants.dart)).
   If timeout: PASS-blocked-by-environment, report "pedal not
   advertising / out of range" and stop.
4. `tap_text "Connect"` (the row's button). Wait for scanner to pop:
   `wait_for_text "Edit profiles" 15` (back on home, now connected).
5. `tap_text "Edit profiles"` → ProfileListScreen.
6. `tap_text "Import"` (or whatever the import-JSON entry point is —
   see ProfileListScreen). Pick `data/profiles.json` via the file
   picker — **TODO**: this step needs an Android file-picker driver,
   which is fragile. Workaround: push the JSON onto the device and
   use an in-app "load default" if one exists, else accept that this
   step requires a one-time recipe extension.
7. Back to home: system back, then `wait_for_text "Upload" 5`.
8. `tap_text "Upload"` → Upload screen.
9. `tap_text "Upload Profiles"`.
10. `wait_for_text "Upload successful" 15` → if found, PASS with
    `shot success`. If timeout: capture `shot fail`, dump
    `logcat_since 500`, dump `ui_dump`, FAIL.

### SC-01 — Scanner shows full pedal name (or clear ellipsis)

Verifies: the in-app scanner row is not silently truncated to
`AwesomeStudioPe` without an ellipsis. Underlying defect: TASK-263.

Steps:

1. Wait for home: `wait_for_text "Connect to pedal" 10`.
2. `tap_text "Connect to pedal"` → scanner.
3. `wait_for_text "AwesomeStudio" 20`.
4. Inspect the matched node from `ui_dump`: the `text` attribute on
   the device-name `TextView` must either contain the full pedal
   name (`AwesomeStudioPedal-XX` style) **or** end in `…` (Unicode
   ellipsis, U+2026). If it equals `AwesomeStudioPe` exactly with
   no ellipsis: FAIL. Otherwise PASS.
5. `shot scanner_row` regardless.

### AE-03 + AE-14 — Action Editor: KEY_PAGE_UP saves as SendKeyAction and Save navigates back

Verifies two TASK-252 fixes at once: (a) Saving a "Key (named)" /
KEY_PAGE_UP action writes JSON `type: "SendKeyAction"` (not the old
`SendCharAction`); (b) Tapping Save returns to Profile Editor.

Pre-condition: profile "01 Score Navigator" exists (loaded from
`data/profiles.json`). The recipe modifies profile 01 button A —
acceptable for an automated re-run; the device's profile state is
not preserved.

Steps:

1. `wait_for_text "Edit profiles" 10` then `tap_text "Edit profiles"`.
2. `wait_for_text "01 Score Navigator" 5` then `tap_text "01 Score Navigator"`.
3. `wait_for_text "SendCharAction" 5` (Profile Editor showing button rows).
4. Tap Button A row. There is no unique text — tap the centre of the
   first `Button` whose `content-desc` starts with `"A\n"`. Use the
   same Python-bounds extraction as `tap_text` but with a regex match.
5. `wait_for_text "Button A" 5` — confirms Action Editor.
6. The default loaded action is "Key (named)" / "KEY_PAGE_UP" / "Prev Page".
   We want the action type to be Key (named) (already is). Use
   `focus_and_set "KEY_PAGE_UP" "KEY_PAGE_UP"` to confirm/refresh the
   value field, and `focus_and_set "Prev Page" "Next Page"` to set
   the display name.
7. `tap_text "Save"`.
8. **Assert (AE-14):** `wait_for_text "01 Score Navigator" 5` returns 0
   AND `ui_dump | grep -F -q 'content-desc="Button A"'` returns 1
   (we are NOT still on Action Editor). On failure → AE-14 FAIL.
9. `nav_to_profile_list` → `read_json_preview`.
10. **Assert (AE-03):** the JSON output contains, for profile 01 button A,
    a substring matching all of:
    - `"type": "SendKeyAction"`
    - `"value": "KEY_PAGE_UP"`
    - `"name": "Next Page"`
    Use Python to parse the JSON and read `profiles[0].buttons.A`. On
    failure → AE-03 FAIL with the actual block printed.
11. `shot ae_03_json_preview` regardless.

### AE-04 — Action Editor: Media Key value field is filterable and not overflowing

Verifies TASK-253 fixes: (a) the Media Key value field is now a
type-filterable autocomplete (it was a read-only dropdown before);
(b) no Flutter "OVERFLOWED BY 79 PIXELS" render warning.

Steps:

1. From Profile List, `tap_text "01 Score Navigator"`, then tap the
   Button A row (as in AE-03 step 4).
2. `wait_for_text "Button A" 5`.
3. `tap_text "Action Type"` — opens the dropdown overlay.
4. `tap_text "Media Key"` — selects it; overlay closes.
5. `wait_for_text "Media Key" 3` (Action Type now shows "Media Key").
6. **Assert overflow gone:** start a fresh `adb logcat -c` before step 3
   and capture `logcat_since 200` after step 5; grep for
   `OVERFLOWED BY` and `RenderFlex overflowed`. If found → AE-04
   FAIL (overflow regression).
7. **Assert filterable:** find the value field below Action Type. It
   should be an `EditText` (filterable autocomplete), not a `View`/
   `Spinner`. `ui_dump` and assert there is an `android.widget.EditText`
   in the bounds y > Action Type's y2 and y < the next labeled field.
   If the only matching widget is non-editable → AE-04 FAIL.
8. `focus_and_set "" "PLAY"` on the value field (use the bounds-by-position
   match from step 7 since the field starts empty). The autocomplete
   should drop a suggestion list.
9. `ui_dump` and assert at least one Button/View has content-desc
   matching `MEDIA_PLAY` (e.g. `MEDIA_PLAY_PAUSE`). If none → AE-04 FAIL.
10. `shot ae_04_filtered`. Do not save — leave Action Type so AE-06/07
    can re-use the same Action Editor session if invoked next.

### AE-06 — Type String: literal `ctrl+z` round-trips to JSON

Verifies that the `+` character is preserved through SET_TEXT input
(the prior round used `adb input text` which substitutes `+` with
space, an adb-tooling artifact rather than an app bug — this recipe
removes the artifact by using the `set_text` helper which uses `%s`
escaping for spaces but preserves `+`).

Steps:

1. From Profile List, navigate to profile 01 → Button A Action Editor.
2. `tap_text "Action Type"` → `tap_text "Type String"`.
3. `wait_for_text "Type String" 3`.
4. `focus_and_set "" "ctrl+z"` on the value field.
5. `tap_text "Save"`.
6. `nav_to_profile_list` → `read_json_preview`.
7. **Assert:** profile 01 button A has `"type": "SendStringAction"` and
   `"value": "ctrl+z"` (the `+` literal). Parse JSON in Python and
   compare exactly. On failure → AE-06 FAIL with actual value printed.
8. `shot ae_06_json_preview`.

### AE-07 — Key (raw HID) `0xB0` saves as SendKeyAction (JSON leg only)

Verifies the JSON output of TASK-257's hex-value support. The hardware
leg (pedal button → host receives Enter) is **out of scope** for this
skill since it requires the pedal to be powered, BLE-paired, and
HID-bonded — none of which the skill can drive from adb. Run the
hardware leg manually on the test bench.

Steps:

1. From Profile List, navigate to profile 01 → Button A Action Editor.
2. `tap_text "Action Type"` → `tap_text "Key (raw HID)"`.
3. `wait_for_text "Key (raw HID)" 3`.
4. `focus_and_set "" "0xB0"` on the value field.
5. `tap_text "Save"`.
6. `nav_to_profile_list` → `read_json_preview`.
7. **Assert:** profile 01 button A has `"type": "SendKeyAction"` and
   `"value": "0xB0"`. On failure → AE-07 FAIL with actual block printed.
8. `shot ae_07_json_preview`. Print: "Hardware leg deferred — verify
   manually on test bench (press button → host receives Enter)."

### AE-15 — System BACK from Action Editor returns to Profile Editor (not exit app)

Verifies TASK-251 fix: the system BACK gesture from Action Editor
must navigate back to Profile Editor, not exit the app. Also
re-confirms the data-unchanged half (re-open the action, value
matches the original).

Steps:

1. From Profile List, `tap_text "01 Score Navigator"`, then tap
   Button A row.
2. `wait_for_text "Button A" 5`.
3. Capture the value field's current text via `ui_dump` (call this
   `$ORIGINAL`). It will be whatever a prior recipe left there
   (`KEY_PAGE_UP`, `ctrl+z`, `0xB0`, etc.) — this recipe is
   deliberately ordering-agnostic.
4. `tap_text "Action Type"` → `tap_text "Pin Low"`. Action Type now
   shows "Pin Low" with a numeric Pin field instead of the value
   EditText. Do **not** tap Save.
5. `adb -s $DEV shell input keyevent KEYCODE_BACK`.
6. **Assert (AE-15 nav):** `ui_dump` after BACK shows the Profile Editor
   (button rows visible, profile name in title), NOT the Home screen
   (no "Connect to pedal" card) and NOT the Android launcher
   (current_focus belongs to `$PKG`). Check focus with
   `adb -s $DEV shell dumpsys window | grep mCurrentFocus`. On failure
   → AE-15 FAIL.
7. **Assert (AE-15 data):** re-tap Button A, `wait_for_text "Button A" 5`,
   read the value field — it must equal `$ORIGINAL` (action type and
   value were not persisted). On failure → AE-15 FAIL.
8. `shot ae_15_profile_editor`.

### UP-08 — Upload Hardware Config triggers pedal reboot

Verifies TASK-273 fix: after `Upload Hardware Config`, the firmware
must reboot so the freshly written `/config.json` is reapplied at
boot. The visible signal from the host side is that the BLE link
drops within ~1–2 s of the "Hardware config uploaded!" SnackBar.

Pre-condition: a valid `config.json` has been pushed to
`/sdcard/Download/config.json` (the recipe pushes it from
`data/config.json`).

Steps:

1. `adb -s $DEV push data/config.json /sdcard/Download/config.json`.
2. Wait for home: `wait_for_text "Connect to pedal" 10`.
3. `tap_text "Connect to pedal"` → scanner.
4. `wait_for_text "AwesomeStudio" 20`. If timeout: PASS-blocked-by-
   environment, "pedal not advertising / out of range".
5. `tap_text "Connect"`. Wait connected:
   `wait_for_text "Edit profiles" 15`.
6. `tap_text "Edit profiles"` → ProfileListScreen.
7. Open the More-actions / overflow menu in the toolbar (TASK-267
   landed this entry point). Look for `content-desc="More options"`
   or "More actions"; if not present, fall back to tapping the
   3-dot icon by bounds.
8. `tap_text "Import Hardware Config"`.
9. The Android file picker opens. Drive it: navigate to
   `/sdcard/Download/`, tap `config.json`. (File-picker driving is
   fragile; if the recipe cannot select the file in 5 attempts,
   FAIL with `import_picker_unreachable`.)
10. After import, navigate back to home: BACK until
    `wait_for_text "Upload" 5` returns 0.
11. `tap_text "Upload"` → Upload screen.
12. Capture the BLE-connected state immediately before the upload:
    `ui_dump | grep -F "Connected"` — record as `$WAS_CONNECTED`.
13. `tap_text "Upload Hardware Config"`.
14. `wait_for_text "Hardware config uploaded" 10` — SnackBar must
    appear. On timeout: FAIL `snackbar_missing`, dump logcat.
15. **Assert (TASK-273 reboot):** within 5 s of the SnackBar, the
    BLE link must drop. Poll `ui_dump` every 500 ms for up to 5 s
    looking for either:
    - "Not connected" appearing on Home, OR
    - the disconnect listener in
      [app/lib/services/ble_service.dart](../../../../app/lib/services/ble_service.dart#L70)
      having fired (visible as the connection-status indicator
      flipping to disconnected).
    If the link is still up after 5 s → FAIL `no_reboot_detected`
    (this was the pre-fix behaviour: SnackBar shown, link stays
    live, reboot never happens).
16. `shot up_08_disconnected` after the disconnect.
17. **Assert recovery:** `tap_text "Connect to pedal"` and
    `wait_for_text "AwesomeStudio" 30` — the pedal must come back
    advertising after rebooting. Timeout → FAIL
    `pedal_did_not_re_advertise`.
18. `shot up_08_re_advertising`. PASS.

### Adding more scenarios

Put new recipes here. Keep them small (≤ 15 steps), explicit (no
"figure out how to navigate"), and end with a clear pass/fail
assertion. If a scenario depends on a fixture (e.g. an invalid
profiles.json), push the fixture with `adb push` at the start.

## Result handling

On **PASS** for a defect-fix verification task (id starts with TASK-2xx
and the title contains "Defect" or the body's AC mentions "on-device
verification"), append a single line under `## Notes` (or create
that section if missing) of the form:

```
- Verified on-device on YYYY-MM-DD via /verify-on-device <SCENARIO>
  on <model> / Android <version>. Screenshot: /tmp/verify_<TASK>_<step>.png.
```

For a feature-test task (TASK-153..157), do **not** edit the task
file — feature-test results live in `FEATURE_TEST_PLAN.md` and are
appended by the human-in-the-loop reviewer.

On **FAIL**, print:

- The pass/fail line for each step that ran.
- `ui_dump` of the failing screen.
- The last 200 lines of `logcat_since`.
- The screenshot path.
Do not retry. Do not auto-close anything. The user diagnoses.

## When NOT to invoke

- No device is connected (use a host-only Flutter widget test instead).
- The scenario is purely usability / aesthetic ("does this feel right
  to a non-developer"). Those need a human; the skill cannot answer
  them.
- The scenario depends on a feature that does not yet have a UI entry
  point (e.g. UP-08 before TASK-267 lands).

## Skill registration

This skill is registered in [.vibe/config.toml](.vibe/config.toml)'s
`enabled_skills` list per the project's CLAUDE.md skill-registration
rule.
