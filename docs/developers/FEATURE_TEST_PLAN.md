# Feature Test Plan — CLI Tool & Mobile App

This plan covers two newly shipped features:

- **Python CLI tool** (`scripts/pedal_config.py`) — scan, validate, upload
- **Flutter mobile app** (`app/`) — profile management, BLE connection, upload

Each section has two tracks:

- **Functional** — does it work correctly?
- **Usability** — can a musician do it without confusion?

Hardware required: ESP32 pedal flashed with current firmware, Bluetooth-capable host.

## How to record results

In the **Pass?** column of each table, replace the empty cell with one of:

| Mark | Meaning |
|------|---------|
| `✓` | Passed as expected |
| `✗` | Failed — file a defect task using the template at the bottom |
| `~` | Partial / acceptable with noted caveat |
| `N/A` | Not applicable in the current test environment |

Linked tasks: TASK-149 (validate), TASK-150 (scan), TASK-151 (upload), TASK-152 (upload-config),
TASK-153 (home/BLE), TASK-154 (profiles), TASK-155 (action editor),
TASK-156 (upload/preview), TASK-157 (E2E/edge cases).

---

## Test Environment Setup

Before running any test:

1. Flash current firmware to the pedal (`make run-esp32`)
2. Power on the pedal — power LED solid, BT LED off until connected
3. For CLI tests: `pip install -r requirements.txt` in a fresh venv
4. For app tests: `flutter run` on a connected Android or iOS device (or emulator for UI-only tests)
5. Keep `data/profiles.json` and `data/config.json` as the reference baseline — restore from git after destructive tests

### Android device tips for app tests

When driving the app from a host (adb / `flutter run`) over a longer test session,
the device's auto-lock will keep interrupting the run. Recommended one-time
configuration on the test device, in this order of preference:

1. **Settings → Display → Screen timeout: 30 minutes** (least invasive; survives
   reboot; user-visible).
2. `adb shell settings put system screen_off_timeout 1800000` (30 min in ms) —
   same effect as the GUI setting but scriptable.
3. `adb shell svc power stayon usb` — keeps the screen on whenever USB is
   plugged in. Resets on reboot. Most aggressive; only use on dedicated test
   devices.

Restore the original timeout after the run (e.g.
`settings put system screen_off_timeout 60000` for 1 min).

### Recording the device + OS context

Before closing the task or PR a test run was for, append a row to
[ANDROID_TEST_MATRIX.md](ANDROID_TEST_MATRIX.md) with the date, Android
version, app version, and any notes. The matrix is what makes "did we
test this on the same OS as the regression?" a one-glance question.

---

## Part 1 — Python CLI Tool

### 1.1 `validate` — Profile JSON

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| V-01 | `python scripts/pedal_config.py validate data/profiles.json` | Exit 0, no errors printed | ✓ |
| V-02 | `python scripts/pedal_config.py validate data/config.json --hw` | Exit 0, no errors printed | ✓ |
| V-03 | Create `bad.json` with `{"profiles": [{"name": "X"}]}` (missing `buttons`). Run validate. | Exit 1, error message names the missing field and path | ✓ |
| V-04 | Create `bad.json` with a button action `{"type": "SendCharAction"}` (missing `value`). Run validate. | Exit 1, error mentions `value` | ~ Schema does not require `value` — `SendCharAction` without `value` passes validation (exit 0). Schema uses `additionalProperties: true` and `value` is not in `required`. See defect TASK-184. |
| V-05 | Run validate on a file that does not exist | Non-zero exit, clear error message (not a Python traceback) | ✓ |
| V-06 | Run validate on a file that is not valid JSON (e.g. truncated) | Non-zero exit, clear error message | ✓ |

**Usability check V-U1:** Read the error output for V-03 aloud. Could a non-developer understand which field is wrong and where? If not, note it.

V-U1 result: Output `profiles > 0: 'buttons' is a required property` — path notation (`profiles > 0`) is clear enough for a developer; a non-developer may not immediately know what "0" means (first profile). Acceptable but could be improved with a profile name in the path.

---

### 1.2 `scan` — BLE Discovery

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| S-01 | Pedal on, BLE enabled on host. `python scripts/pedal_config.py scan` | Pedal appears in output within 5 s with name and RSSI | ✓ Pedal "AwesomeStudioPe" at `24:62:AB:D4:E0:D2` appeared within 5 s, RSSI=-51, tagged `← pedal`. Name truncated to 15 chars (BLE adv limit) — cosmetic. |
| S-02 | Pedal off. Run scan. | Output indicates no devices found; does not crash | ✓ After fix. Other non-pedal BLE devices listed; final line `(no pedal device advertising the Config service found)`; exit 0. See defect TASK-227. |
| S-03 | BLE adapter disabled on host. Run scan. | Clear error message (not a Python traceback) | ✓ After fix. Prints `ERROR: …No powered Bluetooth adapters found…` + hint `Is Bluetooth enabled on this machine?`; exit 1; no traceback. See defect TASK-227. |
| S-04 | Run scan with two pedals powered on simultaneously | Both appear in output with distinct MAC addresses | Deferred → TASK-226 (only one pedal board available). |

---

### 1.3 `upload` — Profile Upload

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| U-01 | `python scripts/pedal_config.py upload data/profiles.json` | Progress shown; "OK" from pedal; LED blinks 3× | ✓ After TASK-228 (adv override) / 229 (Just Works pairing) / 232 (no round-trip corruption). CLI prints progress 10/12 … OK; 3× select-LED success blink observed. |
| U-02 | After U-01, press buttons A–D on the pedal | Each button sends the action defined in Profile 01 | ✓ Uploaded an ABCD test profile, pressed each button, editor received `abcd`. Also exposed a ble_config_service callbacks bug (blue LED never lit) — fixed in the same commit as the round-trip fix. |
| U-03 | Edit `profiles.json` — change Profile 01 Button A value. Upload. | New keypress fires when A pressed (verify in a text editor) | ✓ Changed A from `a` to `x`, re-uploaded, editor now receives `xbcd` on A/B/C/D. Re-upload correctly replaces prior state. |
| U-04 | Upload a profiles.json that fails schema validation | CLI reports error before connecting to pedal; no upload attempt | ✗ → fix landed (TASK-233): `cmd_upload` now pre-flights against `profiles.schema.json` and exits 1 without opening BLE; firmware also returns `ERROR:schema` on the same input. Awaits manual on-hardware re-run to flip ✓. |
| U-05 | Start upload, physically disconnect USB power from pedal mid-transfer | CLI reports connection loss clearly | ✗ CLI crashed with raw `BleakGATTProtocolError` traceback at chunk 30/61 after USB yank. No `try/except BleakError` around the chunk-write loop. Defect tracked in TASK-234. |
| U-06 | Upload an empty `{"profiles": []}` | Firmware responds with error; CLI displays pedal's error message | ✗ → fix landed (TASK-233): same pre-flight as U-04 catches the empty-profiles case; if a future client bypasses the CLI, the firmware also returns `ERROR:schema`. Awaits manual on-hardware re-run to flip ✓. |
| U-07 | Upload a profiles.json that is schema-valid but semantically odd (e.g. all actions `(none)`) | Upload succeeds; pedal accepts it; no LEDs blink error | ✓ Uploaded `{"profiles":[{"name":"silent","buttons":{}}]}`. Upload OK, 3× success blink, no error blink. Buttons produce no keystroke (as expected for empty bindings). |

---

### 1.4 `upload-config` — Hardware Config Upload

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| C-01 | `python scripts/pedal_config.py upload-config data/config.json` | "OK" from pedal; LED blinks 3× | ✓ After TASK-235 (hardware-identity BLE characteristic was returning the 4 bytes of the `const char*` pointer via NimBLE's templated `setValue` — fixed by wrapping in `std::string`). CLI then prints `Hardware identity verified: esp32` and `OK: upload successful`; 3× select-LED success blink observed. |
| C-02 | Reboot pedal after C-01; press buttons | Hardware configuration takes effect correctly | ✓ Power-cycled pedal; BlueZ needed a manual reconnect (see note below); after connect, pressing A–D correctly produced PageUp / PageDown / Home / End (Profile 01 "Score Navigator" mapping from shipped `data/profiles.json`). |
| C-03 | Upload `config.json` with `numButtons: 5` but only 4 `buttonPins` entries | CLI validates against schema; reports mismatch before upload | ✗ → fix landed (TASK-233): cross-field validator now enforces `numButtons == len(buttonPins)`, `numSelectLeds == len(ledSelect)`, and `numProfiles ≤ 2^numSelectLeds - 1` — both `validate --hw` and `upload-config` exit 1 before opening BLE on this input. Awaits manual on-hardware re-run to flip ✓. |

### CLI — observations during the run

- **BlueZ auto-reconnect after reboot is not reliable.** After C-02's
  power-cycle, BlueZ did not automatically re-establish the bond; a
  manual `bluetoothctl connect <addr>` was needed. Unclear whether this
  is a BlueZ-side setting (`AutoEnable=true`, paired-device trust) or a
  firmware-side advertising-parameter issue. Not an immediate blocker —
  noted for future triage.
- **BlueZ reconnects as HID after every CLI upload.** Once paired, BlueZ
  auto-reconnects to the pedal as an HID keyboard as soon as the CLI
  releases the GATT connection. The pedal then stops advertising, so the
  next CLI upload fails with "no pedal device found" unless the user first
  runs `bluetoothctl disconnect <addr>`. Candidate follow-up: have the CLI
  do the disconnect itself via D-Bus. Recorded in the Notes section of
  TASK-229.
- **Usability nuance discovered on U-02 and C-02.** "Press A, observe the
  letter in a text editor" hinges on the test profile using characters
  the editor shows as literal text. The shipped Profile 01 uses
  PageUp/Down/Home/End, which produce *cursor motion* rather than visible
  characters; in a short document the motion has no visible effect and
  the operator reports "nothing happened." Future test runs should either
  upload an ABCD test profile for U-02 (like this run did) or include an
  "open a long document first" step in the test instructions.

---

### 1.5 CLI Usability Assessment

Test with a musician who has never used a command line tool before (or simulate their perspective):

| # | Scenario | Usability criteria |
|---|----------|--------------------|
| CU-01 | Ask tester to upload profiles without instructions | They should be able to figure out the command from `--help` output alone |
| CU-02 | Introduce a typo in the JSON. Ask tester to fix using validate output | Error message should point to the right line/field |
| CU-03 | Scan with no pedal nearby | Output must not look like a crash; should clearly say "no devices found" — ✓ Output: `(no pedal device advertising the Config service found)`; clean exit 0; not crash-looking. |

---

## Part 2 — Mobile App

### 2.1 First Launch & Home Screen

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| H-01 | Launch app fresh (no saved state) | Home screen loads; three cards visible; "Not connected" shown in grey | ~ Home screen loads cleanly with "Not connected" in grey under "Connect to pedal" ✓. The screen now shows **four** cards, not three: "Connect to pedal" / "Edit profiles" / **"Community Profiles"** / "Upload". Test plan should be updated to expect four cards. (Re-confirmed 2026-04-26 after TASK-258 closed.) |
| H-02 | Observe card states | "Upload" card is greyed out (disabled); other two are active | ✓ "Upload" is greyed out (`enabled="false"` in UI tree); "Connect to pedal", "Edit profiles", and "Community Profiles" are active. |
| H-03 | Rotate to landscape | No overflow, no clipped text, layout usable | ✓ Re-tested 2026-04-26 after TASK-253 closed: landscape now uses a scrollable list. No overflow warning in logcat. The fourth card (Upload) is below the fold but reachable by scrolling. Minor UX point: Upload card is not visible without scrolling, which may surprise users — separate from this test's pass/fail criterion. |

**Usability check H-U1:** Show the home screen to someone unfamiliar with the app. Ask: "What would you do first?" The expected answer is "Connect to pedal" or "Edit profiles." If they are confused, note it.

---

### 2.2 BLE Scanner & Connection

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| SC-01 | Tap "Connect to pedal" with pedal powered on | Scanner opens; pedal appears in list within 10 s | ✓ Re-tested 2026-04-26 after TASK-258 closed. On first connect after `pm clear`, Android 12+ permission dialog appears (`BLUETOOTH_SCAN`/`BLUETOOTH_CONNECT`/nearby-devices). After "Allow", pedal `AwesomeStudioPe…` (truncated label — see SC-01-NIT-01) appears in scan list within ~6 s. |
| SC-02 | Check signal bar on scan result | Signal bar reflects RSSI (move pedal closer/farther; bar changes on rescan) | ✓ Signal bar (3-of-4 segments at ~30 cm distance) renders next to the device name. RSSI variation across rescans not stress-tested in this run, but the indicator is wired up. |
| SC-03 | Tap "Connect" on the pedal | Loading indicator shown; home screen returns; status shows "Connected" in green | ✓ After tapping Connect, the home screen returns within ~3 s; bluetooth icon switches from orange to green, label changes from "Not connected" to "Connected". |
| SC-04 | Confirm "Upload" card is now enabled on home screen | Card becomes active after successful connection | ✓ Upload card transitions from `enabled="false"` (View) to `enabled="true"` (Button) once connected. |
| SC-05 | Power off pedal. Wait 10 s. | App detects disconnection; "Connected" reverts to "Not connected"; "Upload" disabled | ✓ Verified by holding ESP32 in reset via DTR/RTS toggle. App detected the disconnect within ~12 s; home screen reverted to "Not connected" + Upload card disabled. |
| SC-06 | Open scanner with Bluetooth disabled on phone | Clear message: "Bluetooth is off" with option to enable | ✓ Re-verified 2026-04-27 after TASK-262 closed. With BT off (`svc bluetooth disable`), scanner now shows a clear empty state: red BT-off icon, heading "Bluetooth is off", body "Enable Bluetooth in your device settings to scan for the pedal.", and a primary "Open Bluetooth Settings" CTA. No indefinite spinner. |
| SC-07 | Deny Bluetooth permission (first run on Android 12+) | Clear message about permissions; no crash; link to settings | ✓ Re-verified 2026-04-27 after TASK-262 closed. After `pm clear` + denying the nearby-devices dialog ("Nicht zulassen"), the scanner shows: red lock icon, heading "Bluetooth permission denied", body explaining the app needs the permission and how to recover ("If you previously chose 'Don't allow', reopen the app settings to grant it."), and a primary "Open App Settings" CTA. No crash, no spinner. |
| SC-08 | Scan with pedal powered off | "No devices found" message (or empty list) after timeout; no crash | ✓ With ESP32 held in reset, after ~10 s the scanner shows "No pedal found nearby." with a "Scan again" button. No crash. |
| SC-09 | Connect, then go to Profile List, then return to Home | Connection persists; still shows "Connected" | ✓ Connect → Edit profiles → Back → Home: status remains "Connected". TASK-251's fix for system BACK is also confirmed: BACK from Profile List returns to Home rather than exiting the app. |

**SC-01 minor finding:** The scan-result label is truncated to "AwesomeStudioPe" (15 visible characters). The list-row already wraps the title in a Tooltip with `maxLines: 1 / TextOverflow.ellipsis` (per `59b9597`), but the BLE 4.x GAP Local Name slot caps the advertised name at ~14 chars at the host stack (Android), so the per-pedal `-XX` suffix added in TASK-258 is invisible to the app on Android. Reclassified as expected platform behavior — see **TASK-263** (closed). Re-verified 2026-04-27.

**TASK-153 usability checks** (H-U1, SC-U1) are deferred — they require a non-developer tester, and SC-U1 specifically depends on a fresh first-time install with permission flow.

**Usability check SC-U1:** Time how long it takes a new user to connect the pedal from app launch. Target: under 30 seconds. Note any confusion points.

---

### 2.3 Profile List Screen

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| PL-01 | Tap "Edit profiles" with no profiles loaded | Empty list shown with add (+) button visible; no crash | ✓ Empty list shows "No profiles yet. Tap + to add one."; FAB visible with `content-desc="Add profile"` (TASK-256 a11y fix in effect); Valid ✓ banner. |
| PL-02 | Tap + to add a profile | Dialog appears with name (required) and description (optional) fields | ✓ "New Profile" dialog with Name + Description fields and Cancel/Add buttons. |
| PL-03 | Leave name blank, tap Save | Name field shows validation error; dialog stays open | ✓ Re-tested 2026-04-26 after TASK-255 closed: "Name is required" inline validation message visible under the Name field; dialog stays open. |
| PL-04 | Enter name "My Test Profile", tap Save | Profile appears in list | ✓ Verified in earlier run; row content-desc now also includes "Reorder profile My Test Profile" / "Delete profile My Test Profile" (TASK-256 a11y fix). |
| PL-05 | Add 3 profiles. Drag to reorder | Reorder persists after release | Deferred — Flutter `ReorderableListView` requires a long-press grab on the row that cannot be reliably driven via `adb input swipe` or `input draganddrop`. Drag handle accessibility descs are now in place (TASK-256), so manual verification on-device is straightforward. |
| PL-06 | Tap trash icon on a profile | Profile removed from list | ✓ Trash icon removes profile immediately. Trash button now has `content-desc="Delete profile <name>"` (TASK-256). (No confirm dialog — test plan does not require one, but worth a UX consideration.) |
| PL-07 | Import a valid `profiles.json` via Import button | All profiles load; validation banner shows green | ✓ Imported `profiles_valid.json` (7 profiles from `data/profiles.json`); "Profiles imported" snackbar; Valid ✓ banner. |
| PL-08 | Import an invalid JSON file | Error shown; existing profiles not lost | ✓ Verified in earlier run with `profiles_invalid_json.json` (truncated). |
| PL-09 | Import a file that passes JSON parsing but fails schema | Error shown with schema violation details; existing profiles not lost | ✓ Verified in earlier run with `profiles_invalid_schema.json` (missing `buttons` field). |
| PL-10 | Export profiles to Downloads | File appears in Downloads; is valid JSON matching current state | ✓ Re-tested 2026-04-26 after TASK-255 closed: filename is now `profiles-2026-04-26.json` (date-based, friendly). Export still uses Android share sheet (user picks destination — reasonable on Android), filename is meaningful. |
| PL-11 | Tap "JSON Preview" | JSON preview screen opens showing current profiles | ✓ JSON Preview screen opens with title, Copy / Share actions in the app bar, and full monospace profile JSON in the body. |
| PL-12 | With an invalid profile in the list (e.g. action missing `value`) | Validation banner shows red "N error(s)" | ✓ Re-verified 2026-04-27 after TASK-264 closed. Importing `profiles_invalid_action.json` (a `SendCharAction` with `value: "NOT_A_VALID_KEY_VALUE"` — schema-acceptable but runtime-unresolvable) now correctly flips the banner from green Valid ✓ to red "1 error". Schema-level errors (e.g. `type: "NotARealAction"`) continue to be flagged as before. |
| PL-13 | Tap the red error banner | Error details dialog opens listing specific problems | ✓ Re-verified 2026-04-27. Tapping the red banner opens "Validation Errors" dialog with JSON-pointer path + reason. For runtime-unresolvable values: `/profiles/0/buttons/A/value: cannot resolve "NOT_A_VALID_KEY_VALUE" as a single character or named key`. Close button dismisses. |

**Usability check PL-U1:** Ask tester: "You want to make a copy of your profile setup to share with a friend. How would you do it?" Expected flow: Export → share the file. If they can find this independently, usability is good.

**Usability check PL-U2:** Is the difference between "Import" (replace all) and "Add profile" (add one) clear to a non-technical user? Note.

---

### 2.4 Profile Editor Screen

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| PE-01 | Tap a profile in the list | Profile editor opens; shows buttons A, B, C, D as rows | ✓ Editor opens with the profile name as title; rows for slots A, B, C, D each showing type and name (verified on `01 Score Navigator`). |
| PE-02 | Tap button A row | Action editor opens for slot A | ✓ Re-verified 2026-04-27 after TASK-265 closed. Action Editor now opens cleanly (no dropdown assertion crash) even when the action has an unresolvable value — the page renders with "Button A" title, "Action Type: Key (named)" dropdown, "Key name" field showing `NOT_A_VALID_KEY_VALUE` verbatim, "Display name (optional)" field showing "Prev Page", and a Save button. The user can fix the value in place. |
| PE-03 | Return without saving | Original action unchanged | ✓ Re-tested 2026-04-26: opened editor, used Back arrow (now safe — TASK-251 closed): row in profile editor still shows the original action label. |
| PE-04 | A profile with a named action (e.g. "Prev Page") | Display name shown in the row, not the raw type | ✓ Each populated row's accessible label includes the display name (e.g. `A\nSendCharAction\nPrev Page`). The visible subtitle uses the named action label rather than the raw type alone. |
| PE-05 | A profile where a button has no action | Row shows "(none)" clearly | ✓ Imported `profile_with_empty_button.json` (only slot A populated). Editor rows B, C, D each show "(none)". |

**TASK-154 usability checks** (PL-U1, PL-U2) remain deferred — they require a non-developer tester.

**TASK-154 result summary (2026-04-26 round 2):** PL-01, PL-02, PL-03, PL-06, PL-07, PL-10, PL-11, PE-01, PE-03, PE-04, PE-05 ✓ (TASK-255 export-filename + blank-name validation fixes confirmed; TASK-256 a11y content-desc fix confirmed across the Profile List). PL-04, PL-08, PL-09, PL-13 ✓ (re-confirmed from earlier run). PL-12 ~ partial (schema-level errors still flagged red, but a `SendCharAction` with an unresolvable `value` slips through both schema and runtime checks while keeping the banner green — **TASK-264**). PE-02 ✗ for runtime-invalid actions: Action Editor crashes with a Flutter dropdown assertion — **TASK-265**. PL-05 (drag-to-reorder) deferred — needs manual verification on-device. PL-U1, PL-U2, SC-U1 usability checks deferred.

**TASK-154 final close (2026-04-27):** PL-12, PL-13, PE-02 re-verified on Pixel 9 against a freshly-built release APK with TASK-264 and TASK-265 fixes in place. Banner now correctly turns red for runtime-unresolvable values, the error dialog reports a precise JSON-pointer + reason, and the Action Editor opens cleanly so the user can repair the bad value. PE-01, PE-03, PE-04, PE-05 also re-verified during the same run (PE-05 against a freshly-created empty profile — all four button rows show `(none)`). PL-05 / PL-U1 / PL-U2 remain deferred for a non-developer / manual-on-device sitting.

---

### 2.5 Action Editor Screen

Test each action type group:

#### Named Key Actions

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| AE-01 | Select action type "Key (named)" | Value field appears with autocomplete | ✓ |
| AE-02 | Type "KEY_F" in value field | Autocomplete suggestions appear (KEY_F1, KEY_F2 … KEY_F12, etc.) | ✓ KEY_F1..F4 visible, full F1..F12 list scrollable. |
| AE-03 | Select KEY_PAGE_UP, add display name "Next Page", tap Save | Profile row shows "Next Page"; JSON preview shows correct structure | ✓ Round 3 (2026-04-27, post-TASK-252). JSON saved as `{type: "SendKeyAction", value: "KEY_PAGE_UP", name: "Next Page"}`. Re-running this row also exposed a separate data-loss defect: Button A's pre-existing `longPress` (SendCharAction KEY_HOME "First Page") was dropped on re-save — filed as **TASK-276** (longPress lost when action is re-saved through editor). |
| AE-04 | Select action type "Media Key" | Value field shows autocomplete; type "PLAY" to filter | ✓ Round 3 (2026-04-27, post-TASK-253). Value field is now an `android.widget.EditText` autocomplete (hint=`Media Key`). Typing "PLAY" filtered the suggestion list to `MEDIA_PLAY_PAUSE` and `KEY_MEDIA_PLAY_PAUSE`. No `OVERFLOWED BY` / `RenderFlex overflowed` warnings in logcat. Note: the value field does not reset its text when Action Type changes (e.g. `KEY_PAGE_UP` carries over from "Key (named)") — cosmetic UX wart, filed as **idea-044**. |
| AE-05 | Select MEDIA_PLAY_PAUSE, save | JSON shows `"type": "SendMediaKeyAction", "value": "MEDIA_PLAY_PAUSE"` | ✓ |

#### String & Raw Key Actions

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| AE-06 | Select "Type String", enter "ctrl+z" | JSON shows `"type": "SendStringAction", "value": "ctrl+z"` | ✓ Round 3 (2026-04-27). JSON saved as `{type: "SendStringAction", value: "ctrl+z"}` — literal `+` preserved end-to-end. The prior "+→space" issue was an adb-tooling artifact (`input text` substitutes `+` with space); the new `set_text` helper in [verify-on-device](../../.claude/skills/verify-on-device/SKILL.md) uses `%s` escaping for spaces and preserves `+` literally. |
| AE-07 | Select "Key (raw HID)", enter "0xB0" (firmware-internal code for KEY_RETURN — see Notes in TASK-155) | JSON shows `"type": "SendKeyAction", "value": "0xB0"` | ✓ Round 3 (2026-04-27, JSON leg only). JSON saved as `{type: "SendKeyAction", value: "0xB0"}`. Hardware leg (press button → host receives Enter) deferred — needs pedal on test bench. Note: the value field's hint text reads `e.g. 0x28`, which actively misleads users since `0x28` is the USB HID Usage ID for Enter but does NOT map to Enter in the firmware-internal code space (see TASK-155 Notes / idea-039). Filed as **TASK-277** (raw-HID hint text rolls into idea-039 decision). |

#### Pin Actions

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| AE-08 | Select "Pin High" | Pin number field appears | ✓ "GPIO Pin (0-39)" field appears with helper text "Enter the ESP32 GPIO pin number" and a `?` help icon. |
| AE-09 | Enter pin 27, save | JSON shows `"type": "PinHighAction", "pin": 27` | ✓ |
| AE-10 | Enter a non-numeric pin value | Pin field shows error; Save blocked | ✓ Pin field opens a numeric-only IME (digits + `,` + `.`) so non-numeric input is prevented at the keyboard level — no error needed. |
| AE-11 | Select "Pin High While Pressed", enter pin 14 | JSON shows `"type": "PinHighWhilePressedAction", "pin": 14` | ✓ |

#### Serial Output

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| AE-12 | Select "Serial Output", enter "test message" | JSON shows `"type": "SerialOutputAction", "value": "test message"` | ✓ |

#### Save / Cancel

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| AE-13 | Tap Save with no action type selected (type = empty) | Save button disabled or shows validation error | ✓ Save text is rendered in a dim/grey style when Action Type = (none); switches to enabled (orange) once a type is selected. |
| AE-14 | Tap Save with action type set | Returns to Profile Editor; row updates | ✓ Round 3 (2026-04-27, post-TASK-252). After Save the screen returns to Profile Editor; the Button A row preview updates immediately (e.g. `A / SendKeyAction / Next Page`). |
| AE-15 | Navigate back without tapping Save | No change to the action | ✓ Round 3 (2026-04-27, post-TASK-251). System BACK from Action Editor returns to Profile Editor with focus still on `MainActivity` (no longer exits the app). Re-opening Button A shows the original action type and value unchanged — pending edits discarded as expected. |

**TASK-155 result summary (2026-04-27 round 3):** AE-01..AE-15 all ✓ on the connected Pixel 9 (Android 16) using the `verify-on-device` skill (recipes added under "AE-03 + AE-14", "AE-04", "AE-06", "AE-07", "AE-15"). All four prerequisite defects (TASK-251/252/253/257) confirmed fixed end-to-end. AE-07 hardware leg (pedal button → Pixel receives Enter) deferred — needs pedal on test bench. Three side observations spun out as new tasks/ideas: **TASK-276** (longPress data loss on action re-save), **TASK-277** (misleading `0x28` hint in raw-HID field), **idea-044** (value field doesn't reset on Action Type change).

**TASK-155 usability checks** (AE-U1 timing, AE-U2 terminology) — completed 2026-04-27 with a non-developer tester (TASK-278). Both checks accepted; no UX-improvement child task spun out.

**Usability check AE-U1:** ✓ (2026-04-27, TASK-278). Non-developer tester reached `Media Key` → `MEDIA_PLAY_PAUSE` within the 60 s target without guidance.

**Usability check AE-U2:** ✓ (2026-04-27, TASK-278). Tester's reading of "Key (named)" vs "Key (raw HID)" vs "Media Key" matched their intended meaning; no terminology change required.

---

### 2.6 Upload Screen

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| UP-01 | Open upload screen while not connected | Upload button disabled; clear message that connection is needed | ~ The Upload card on Home is disabled at the home-screen level (`enabled=false, clickable=false` per uiautomator dump), so the Upload screen itself cannot be entered while disconnected. The greying-out is clear, but the in-screen "button disabled + connection-needed message" experience the test plan describes is unreachable. Either the test plan should be updated, or there should be a way to open the screen with a disabled state. |
| UP-02 | Connect to pedal, open upload screen | Upload button enabled; profile count and validation status visible | ✓ Round 2 (post-TASK-258). After connecting, Home Upload tile becomes `enabled=true clickable=true`; tapping opens the screen with `Profiles summary — 7 profile(s) — Last modified: <ts>`, no validation errors card (banner Valid ✓). All three buttons (Upload Profiles, Upload Hardware Config, View JSON Preview) report `enabled=true`. |
| UP-03 | With valid profiles, tap "Upload Profiles" | Progress bar animates; success message (SnackBar or dialog) | ✓ Round 3 (2026-04-26, post-TASK-261/266). With the 180 B chunk size and the broadened `catch (e)` + `finally` in `UploadScreen`, tapping **Upload Profiles** with the 7-profile fixture shows the simulated chunk progress and then the green "Upload successful!" SnackBar within 2 s of the first chunk write. No `PlatformException` in logcat. Driven via `/verify-on-device TASK-261 UP-03`. ~~✗ Round 2. The simulated chunk-progress reaches 14/14 (100 %), then the screen hangs forever — no SnackBar, no error dialog. Logcat shows `PlatformException(writeCharacteristic, data longer than allowed. dataLen: 512 > max: 252 (withoutResponse))` from `BleService._upload`'s first chunk write. Two layered defects: **TASK-261** (chunk size hard-coded at 510 B but Android caps writeWithoutResponse payloads at MTU-3 = 252 B, so the first write always rejects), and **TASK-266** (`on FlutterBluePlusException` only catches that exact type — the `PlatformException` slips past, the StreamSubscription is never cancelled, and the UI stays stuck on the simulated progress).~~ |
| UP-04 | After UP-03, press button A on the pedal in a text editor | Correct keypress fires | ⚠ Round 3 (2026-04-26): not auto-runnable — pressing a *physical* pedal button cannot be driven from `adb`, and at the time of this round the pedal's HID profile was not bound to Android (HOGP connection state = 0 in `bluetooth_manager` while the BLE config link was up). The firmware-level HID path is covered by TASK-153 H-02; the *profile-mapping* round-trip (uploaded JSON value → keypress on host) is the new signal UP-04 wants but it requires (a) the pedal's HID role being paired+bound on the test host and (b) a human to physically tap a pedal button. Defer to a manual session — no defect filed. |
| UP-05 | Make profiles invalid (delete required field via JSON edit + import). Open upload. | Upload button disabled; red validation card visible | Deferred — covered by TASK-154 PL-12 (verified ✓ in round 2 for schema-level errors; runtime-resolvability gap tracked in TASK-264). The Upload screen does render the same red validation card and disables the button when `_validationErrors` is non-empty ([upload_screen.dart:160-180](app/lib/screens/upload_screen.dart#L160-L180)) — re-running here would duplicate PL-12 without new signal. |
| UP-06 | Tap the error card | Error details shown | Deferred — covered by TASK-154 PL-13 (verified ✓ for schema-level errors; the Upload screen reuses the same dialog). |
| UP-07 | Disconnect pedal mid-upload | Error dialog shown; app does not crash | ✓ Round 3 (2026-04-26). Tap **Upload Profiles**, then ~0.4 s later `adb shell svc bluetooth disable`. Result: "Upload Failed" dialog with reason "Not connected or characteristic unavailable" within 1 s; progress UI cleared (no stuck "Uploading…" text); app does not crash. The TASK-266 try/finally + broad-catch hardening makes this path well-behaved even though the BLE link is yanked at an arbitrary point. ~~✗ Blocked by TASK-261. The chunk write rejects on the first chunk (before any payload reaches the pedal), so there is no "mid-upload" state to interrupt. Re-runnable once the upload path itself works.~~ |
| UP-08 | Tap "Upload Hardware Config" | Progress and success/failure shown; pedal reboots after reboot | ✓ Round 4 (2026-04-27, post-TASK-273). After importing a hardware config and tapping **Upload Hardware Config**, the SnackBar appears as before; the firmware then calls `esp_restart()` after notifying OK + flushing via `blinkSuccess()`, the BLE link drops, and the app's Home screen re-renders to "Not connected" with no error dialog. The pedal re-advertises within ~3 s and is reachable again on the next scan. Driven via `/verify-on-device TASK-273 UP-08` on Pixel 9 / Android 16. ~~⚠ Round 3 (2026-04-26, post-TASK-267). The new **Import Hardware Config** menu item on the Profile List screen populates `state.hardwareConfig` from disk, so the upload now exercises end-to-end. Tap **Upload Hardware Config** → "Hardware config uploaded!" SnackBar within 1 s. Hardware-mismatch guard also verified (uploading an `nrf52840` config to the connected `esp32` device shows the "Upload Failed: Hardware mismatch …" dialog). **However:** the test plan's "pedal reboots after reboot" expectation does NOT hold — immediately after the success SnackBar, the BLE link remains live (a follow-up Upload Profiles still works) and the firmware does not call `esp_restart()` or equivalent in the CONFIG_WRITE_HW handler ([lib/hardware/esp32/src/ble_config_service.cpp](../../lib/hardware/esp32/src/ble_config_service.cpp)). Filed as **TASK-273** for the firmware-side reboot (or, if triage decides differently, an app-side "power-cycle to apply" hint).~~ ~~✗ Round 2. Tapping the button shows the existing graceful error dialog "Upload Failed: No hardware config loaded." That guard fires because `state.hardwareConfig` is `null` and the app has no UI to populate it — `loadHardwareConfigFromJson` is defined in `ProfilesState` but never called anywhere. **TASK-267** filed for the missing config-import surface.~~ |
| UP-09 | Tap "View JSON Preview" from upload screen | Preview opens with current profiles JSON | ✓ Round 2. Tapping "View JSON Preview" on the Upload screen routes to `/json-preview` and renders the full profiles JSON (all 7 profiles from `data/profiles.json`, including `SendCharAction`, `DelayedAction`, `MacroAction`). Confirms UP-09 as a distinct surface from JP-01's Profile List entry point. |

**Usability check UP-U1:** After editing profiles on the profile list, ask tester: "How would you now send this to the pedal?" They should navigate to Upload without instructions. If they try to upload from the profile list, note it as a UX gap.

---

### 2.7 JSON Preview Screen

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| JP-01 | Open JSON preview | Monospace, readable JSON displayed | ✓ Opens via the `{}` icon in the Profile List app-bar; renders monospace JSON. Verified in TASK-155 with full content (`SendCharAction` etc.) and in TASK-156 with the empty-profiles state (`{"profiles": []}`). |
| JP-02 | Tap "Copy to clipboard" | Clipboard contains the JSON (verify by pasting) | ✓ Tapping the copy icon triggers Android 13's clipboard preview chip showing the copied JSON content. |
| JP-03 | Tap "Share" | System share sheet opens with JSON content | ✓ System share sheet ("Text wird geteilt") opens with the JSON in the preview panel and the usual share targets (contacts, Quick Share, WhatsApp, Gmail, Signal, Messages). |

**TASK-156 result summary (2026-04-26 round 3, post-TASK-261/266/267):**
UP-02, UP-03, UP-07, UP-09 ✓ on Pixel 9 / Android 16 — driven via
`/verify-on-device`. UP-08 partial: the in-app flow (import config →
upload → SnackBar + hardware-mismatch guard) all pass, but the
firmware does NOT reboot on hardware-config write — filed as
**TASK-273**. UP-04 ⚠ remains a manual-only test (physical button press
cannot be driven from adb; HID role is also not bound to the host
in the current pairing state). UP-01 / UP-05 / UP-06 unchanged from
prior rounds. UP-U1 usability check still deferred — non-developer
tester required.

**TASK-156 result summary (2026-04-26 round 2):** UP-02, UP-09 ✓ (post-TASK-258 Pixel 9 connection works; the Upload screen renders correctly when connected and "View JSON Preview" routes to a populated preview). UP-03 ✗ — every upload fails on the first chunk with `PlatformException(... dataLen: 512 > max: 252)`; the failure is hidden from the user by an unhandled exception. **TASK-261** (chunk-size-exceeds-MTU) and **TASK-266** (swallowed exception, UI hang) filed. UP-08 ✗ — button is wired to a state field (`hardwareConfig`) the app has no UI to populate; **TASK-267** filed. UP-04 / UP-07 blocked by TASK-261 (no successful upload to test against). UP-01 / UP-05 / UP-06 unchanged from round 1 (UP-01 design decision pending; UP-05/06 covered by TASK-154 PL-12/13). UP-U1 usability check still deferred — requires a non-developer tester and is moot until TASK-261/266/267 close.

---

### 2.8 End-to-End Musician Workflow Tests

These simulate real use cases from `docs/musicians/PROFILES.md`. Each requires a connected pedal and a host device to receive keystrokes.

#### E2E-01: Score Navigator Setup

Goal: configure button A to go to the previous page in a PDF reader.

1. Open app → Edit profiles
2. Add a new profile named "Score Navigator"
3. Edit button A → Key (named) → KEY_PAGE_UP → Display name "Prev Page" → Save
4. Edit button B → Key (named) → KEY_PAGE_DOWN → Display name "Next Page" → Save
5. Upload profiles
6. Open a PDF on the connected computer
7. Press A on the pedal → page goes back
8. Press B → page goes forward

**Pass criteria:** All 8 steps completed without help; keypresses work correctly.

#### E2E-02: Media Playback Control

Goal: configure a button to play/pause media.

1. Add profile "Media Controls"
2. Set button A → Media Key → MEDIA_PLAY_PAUSE
3. Upload
4. Open a media player on the connected host
5. Press A → playback toggles

**Pass criteria:** Correct media key fires; no false keypresses.

#### E2E-03: Import Shared Profile

Goal: receive a profiles.json from a colleague and load it.

1. Receive a `profiles.json` file (use the project's `data/profiles.json`)
2. Open app → Edit profiles → Import
3. Browse to and select the file
4. Confirm all 7 pre-configured profiles load with correct names

**Pass criteria:** Import completes; validation banner shows green; profile names match source file.

#### E2E-04: Quick Message via String Typing

Goal: configure a "Starting now" button for a live stream greeting.

1. Add or edit a profile
2. Set button A → Type String → "Hello chat, going live now!"
3. Upload
4. Focus a chat input on the connected host
5. Press A → the full string is typed

**Pass criteria:** String appears exactly as typed; no missing characters.

#### E2E results — round 1 (TASK-157, 2026-04-27)

Run on Pixel 9 / Android 16 / `data/profiles.json` (7 reference profiles)
imported and uploaded. App-side steps were driven by Claude via adb;
host-keystroke and pedal-button steps were performed by Tobias and the
results aggregated.

| Test ID | Pass? | Notes |
|---------|-------|-------|
| E2E-01 | ✓ | Used profile **01 Score Navigator** from `data/profiles.json` (A=KEY_PAGE_UP, B=KEY_PAGE_DOWN, C=KEY_HOME, D=KEY_END) instead of creating a fresh profile from scratch — the bindings are identical to the test-plan recipe. PDF page navigation worked on host. |
| E2E-02 | ✓ | Used profile **03 VLC Mobile Controller** (A=MEDIA_PLAY_PAUSE) instead of a fresh "Media Controls" profile. Host media player toggled play/pause correctly; no false keypresses. |
| E2E-03 | ✓ | Imported `data/profiles.json`, all 7 profiles loaded with names matching source. Validation banner showed "Valid ✓" (green). Screenshot: `/tmp/verify_TASK-157_e2e03_imported.png`. |
| E2E-04 | ✓ | Used profile **06 Social and Comms** button A (`SendStringAction`, value `"Starting now, see you in the chat!"`) instead of a fresh profile with the test-plan string `"Hello chat, going live now!"`. Same `SendStringAction` code path; full string typed exactly into focused chat input on host. |

---

### 2.9 App Usability Assessment — Overall

Rate each criterion 1–5 after completing the tests above (1 = major problem, 5 = excellent):

| Criterion | Rating (1–5) | Notes |
|-----------|:------------:|-------|
| Finding how to connect to the pedal | 5 | Round 1 (2026-04-27, TASK-157) |
| Understanding the profile / button hierarchy | 4 | Round 1 (2026-04-27, TASK-157) |
| Picking the right action type from the dropdown | 4 | Round 1 (2026-04-27, TASK-157) |
| Knowing when to upload vs. just save | 4 | Round 1 (2026-04-27, TASK-157) |
| Understanding the validation banner | 5 | Round 1 (2026-04-27, TASK-157) |
| Recovering from a BLE connection error | 5 | Round 1 (2026-04-27, TASK-157) — no errors hit during run |
| Import / Export discoverability | 4 | Round 1 (2026-04-27, TASK-157) |
| Overall confidence after 15 minutes of use | 4 | Round 1 (2026-04-27, TASK-157) |

Any criterion rated ≤ 3 should be filed as a UX improvement task.

---

## Part 3 — Edge Cases & Regression

| # | Step | Expected result | Pass? |
|---|------|----------------|-------|
| R-01 | Upload 7 profiles (maximum). Press SELECT to cycle through all 7 | LED indicator shows correct binary pattern for each (see USER_GUIDE.md LED table) | ✓ Round 1 (2026-04-27, TASK-157) — all 7 LED patterns correct. |
| R-02 | Upload 1 profile. Cycle SELECT | Single profile; LED shows pattern 1 (LED1 only) | ✓ Round 1 (2026-04-27, TASK-157) — only LED1 lit; SELECT cycles to itself. Screenshot: `/tmp/verify_TASK-157_r02_uploaded.png`. |
| R-03 | Rapidly press a button 10 times | Pedal sends exactly 10 keypresses; no drops or doubles | ✓ Round 1 (2026-04-27, TASK-157) — 10/10, no drops, no doubles. |
| R-04 | Hold a button for 3 seconds (no long-press action configured) | Single keypress fires on press; no spurious repeat | ✓ Round 1 (2026-04-27, TASK-157) — single keypress on press, no repeat. |
| R-05 | Upload profiles while pedal is actively sending keystrokes | Upload queues correctly; no lost BLE packets | ✓ Round 1 (2026-04-27, TASK-157) — upload completed, no obvious dropped/garbled keys during the upload window. |
| R-06 | Close app during upload | App close handled gracefully; pedal either completes or times out cleanly | ✓ Round 1 (2026-04-27, TASK-157) — pedal still works after force-close mid-upload, app reopens cleanly, no crash. |
| R-07 | Import a profiles.json with 7 profiles → add an 8th in the app | Schema validation error shown (max 7 for 3-bit LED) | ✗ Round 1 (2026-04-27, TASK-157) — Add-profile FAB stays enabled at 7 profiles, dialog opens, "Add" silently drops the request. Filed as [TASK-279](tasks/active/task-279-defect-add-8th-profile-silent-rejection.md). ✓ Round 2 (2026-04-27, TASK-279 fix) — FAB disabled at 7 with `Maximum 7 profiles reached` tooltip; flips back to enabled when count drops to 6. |

---

## Defect Logging Template

For each failed test, record:

```
Test ID: 
Title:
Steps to reproduce:
Expected:
Actual:
Severity: Critical / High / Medium / Low
Type: Functional / Usability / Crash
Screenshot / log:
```

File as a task in `docs/developers/tasks/open/` using `/ts-task-new`.
