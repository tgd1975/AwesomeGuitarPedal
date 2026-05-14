---
id: IDEA-067
title: MIDI interface support — protocol, hardware, and configuration
description: Investigate MIDI support so the pedal can interoperate with MIDI-capable pedalboards — protocol variants, hardware options, configuration model, and whether real demand exists.
category: 🔧 hardware
---

## Motivation

Some guitarists already run a **pedalboard with a MIDI interface** — a central MIDI controller / switcher that talks to amps, effects, and looper hardware over MIDI. If the AwesomeStudioPedal can speak MIDI, it could plug into that ecosystem instead of being a closed BLE-only device.

**Open question that gates the whole idea:** does this audience exist in meaningful numbers? Before any implementation work, we need to verify:

- How common are MIDI-capable pedalboards in the hobbyist / semi-pro segment we target?
- Which protocol variant do those boards actually use (DIN-5 MIDI, USB-MIDI, BLE-MIDI, MIDI 2.0)?
- What MIDI messages would users actually want the pedal to send / receive — program changes, control changes, notes, system-exclusive?

If the answer to "are there real users" is "no", this idea archives without further work.

## Protocol questions

MIDI is not one protocol — it's a family. Each variant changes the hardware and firmware story dramatically.

| Variant | Transport | Pedal-side hardware needed | Notes |
|---|---|---|---|
| Classic MIDI (DIN-5) | Current loop, 31.25 kbaud | DIN-5 jack(s), optocoupler on input, driver on output | The "real" pedalboard interface; messy on a 3D-printed enclosure |
| TRS MIDI | 1/4" or 3.5 mm TRS | TRS jack, plus the same optocoupler/driver | Modern pedalboards moved to TRS — easier mechanically |
| USB-MIDI | USB | USB device controller (ESP32 has it, nRF52840 has it) | Pedalboard would need to be USB host — uncommon |
| BLE-MIDI | Bluetooth LE, Apple's profile | None extra — we already have BLE | Cheapest path; not all pedalboards support it |
| RTP-MIDI | UDP over IP | Wi-Fi + RTP-MIDI stack | Way out of scope for a pedal |

**Sub-questions:**

- Is **BLE-MIDI** enough to claim "MIDI support"? It's free hardware-wise, but real pedalboard ecosystems still lean on DIN/TRS.
- If we do DIN/TRS, which side(s) — input only, output only, or both? Output-only ("the pedal sends program changes to the rest of the rig") is probably 80% of the value at 50% of the hardware cost.
- How does this interact with [IDEA-013 (Bus System)](idea-013-bus-system.md)? A MIDI-out is *already* a bus — do we still need our own?
- How does this interact with [IDEA-008 (Hybrid Tool with DSP)](idea-008-hybrid-tool-with-dsp.md)? That idea already pushes toward more powerful hardware with audio jacks — MIDI ports fit naturally there.

## Hardware challenges

If we go beyond BLE-MIDI:

- **Mechanical:** DIN-5 is bulky and doesn't fit nicely on the current 3D-printed enclosure. TRS is more realistic.
- **Electrical:** the MIDI spec is a 5 V current loop. Our pedal runs at 3.3 V GPIO. Input needs optoisolation (6N138 or modern equivalent); output needs a current-limited driver. Not hard, but not zero either.
- **Galvanic isolation:** MIDI mandates input-side optoisolation to prevent ground loops. Skipping it works "most of the time" and then mysteriously breaks in one user's rig.
- **Pin budget:** every MIDI pair consumes a UART (or bit-banged equivalent). On the ESP32 that's manageable; on the nRF52840 with BLE active we'd need to be careful.

## Button identification on the wire

Once a transport variant is chosen, the next protocol question is **how a MIDI message identifies which button (and which event type) was triggered**. MIDI gives us three orthogonal axes plus a value byte:

| Axis | Range | Role |
|---|---|---|
| Channel | 1–16 | Groups messages by source (per pedal, or per pedal region) |
| Message type | PC / CC / Note On/Off / SysEx | Semantic class |
| Identifier byte | 0–127 (PC#, CC#, Note#) | The specific button or action within that channel + type |
| Value byte | 0–127 (where applicable) | Press / release state, velocity, or numeric payload |

### Message-type options

| Message | Identifier space (per channel) | Carries a value? | Natural fit |
|---|---|---|---|
| Program Change (PC) | 128 | No (PC# *is* the message) | Profile switching — aligns with the pedal's existing profile concept |
| Control Change (CC) | 128 × value 0–127 | Yes | Buttons with press/release/long/double semantics |
| Note On / Note Off | 128 × velocity 0–127 | Yes | Buttons with natural on/off semantics; widely understood by off-the-shelf gear |
| SysEx | Vendor-defined | Arbitrary | Custom multi-byte messages — overkill for v1, useful later for SysEx device-ID |

### Encoding approaches — pressing MIDI into the pedal's event model

The pedal already has a model of "button → press / release / long-press / double-press". MIDI mapping has to encode that somehow. Four candidate approaches, in increasing distance from "vanilla MIDI gear understands it":

1. **One CC per (button, event-type).** Button A press = CC#20 value 127, button A release = CC#20 value 0, button A long-press = CC#21 value 127, etc. Identifier is the CC#, value is just press/release. Uses 1–4 CCs per button. **Most compatible with off-the-shelf gear** because CC values are still interpreted as magnitudes (127 = "fully on").
2. **One CC per button, value encodes the event type.** Button A = CC#20; value 1 = press, 2 = release, 3 = long, 4 = double. Cleaner namespace, but **breaks general MIDI rendering** — receivers that expect a CC value as a magnitude won't understand it.
3. **Note On / Note Off for press/release, CC for long / double-press.** Note On = press, Note Off = release — already what most rigs expect. CC handles the "special" events.
4. **PC for profile-switch actions, CC for everything else.** PC#7 = "switch the receiving device to program 7". Aligns naturally with the pedal's existing profile-switching action; CCs handle non-profile buttons.

Approaches 3 and 4 are not mutually exclusive — most likely the v1 default would be **PC for profile changes + Note On/Off (or CC-with-magnitude) for everything else**.

### Channel allocation

- **Single shared channel per pedal** — simplest; user picks the channel in config. Default.
- **One channel per button** — wasteful (only 16 channels exist total) and breaks multi-pedal setups.
- **Multi-pedal scenario** — two AwesomeStudioPedals on the same MIDI bus need either:
  - Separate channels (caps at 16 pedals per bus — fine in practice), or
  - A SysEx-based unique **device ID** (unlimited, but adds complexity).

### Direction — output only vs. bidirectional

- **Output only** — pedal sends, rig listens. Covers ~80 % of user value at ~50 % of the cost. Probably v1.
- **Bidirectional** — pedal also receives, so it can mirror rig state (e.g. external program change → pedal lights up the matching profile LED). Doubles the firmware work and the protocol surface. Justified only if users specifically want rig→pedal sync.

### Open questions

- [ ] Pick the encoding approach (1 / 2 / 3 / 4, or a hybrid) — every later mapping decision depends on this.
- [ ] Default MIDI channel — fixed, or user-configurable per pedal?
- [ ] Multi-pedal identity — separate channels for v1, SysEx device-ID later?
- [ ] Output only for v1, or bidirectional from day one?

## Worked example — one button, end to end

To make the encoding choices concrete, here is what each approach actually looks like on the wire and in firmware for **a single button (Button 1 on MIDI channel 1)**.

### MIDI byte-level refresher

MIDI is a byte stream. Each message starts with a **status byte** (high bit = 1, so 0x80–0xFF, encoding both message type and channel 0–15) followed by 0, 1, or 2 **data bytes** (high bit = 0, so 0x00–0x7F).

| Status byte | Message | Data bytes |
|---|---|---|
| `0x80–0x8F` | Note Off, channel 1–16 | note (0–127), velocity (0–127) |
| `0x90–0x9F` | Note On, channel 1–16 | note (0–127), velocity (0–127) |
| `0xB0–0xBF` | Control Change, channel 1–16 | CC# (0–127), value (0–127) |
| `0xC0–0xCF` | Program Change, channel 1–16 | program (0–127) |

DIN/TRS transport is UART at **31 250 baud, 8N1, no flow control**.

### Approach 1 — one CC per (button, event-type), value = press/release

**Mapping:** Button 1 → CC #20 on channel 1; value 127 = press, value 0 = release.

Wire bytes for a press → release sequence:

```text
0xB0 0x14 0x7F     // CC, ch 1, CC#20, value 127  (press)
0xB0 0x14 0x00     // CC, ch 1, CC#20, value   0  (release)
```

ESP32 / nRF52840 firmware (Arduino-style, hardware UART):

```cpp
constexpr uint8_t MIDI_CHANNEL = 0;          // 0–15 = "channel 1"–"channel 16"
constexpr uint8_t BUTTON_1_CC  = 20;

void setup() {
    Serial1.begin(31250);                    // MIDI baud rate
}

void on_button_1_press()   { send_cc(BUTTON_1_CC, 127); }
void on_button_1_release() { send_cc(BUTTON_1_CC,   0); }

static void send_cc(uint8_t cc, uint8_t value) {
    Serial1.write(0xB0 | MIDI_CHANNEL);
    Serial1.write(cc);
    Serial1.write(value);
}
```

### Approach 3 — Note On / Note Off

**Mapping:** Button 1 → Note 60 (middle C), channel 1.

Wire bytes:

```text
0x90 0x3C 0x7F     // Note On,  ch 1, note 60, velocity 127  (press)
0x80 0x3C 0x00     // Note Off, ch 1, note 60, velocity   0  (release)
```

A widely used shortcut: **Note On with velocity 0 is treated as Note Off**, which lets a transmitter use the same status byte for both events (slightly cheaper on the wire because of MIDI running-status):

```text
0x90 0x3C 0x7F     // press
0x90 0x3C 0x00     // release (Note On vel 0 == Note Off)
```

Firmware:

```cpp
constexpr uint8_t BUTTON_1_NOTE = 60;

void on_button_1_press()   { send_note(BUTTON_1_NOTE, 127); }
void on_button_1_release() { send_note(BUTTON_1_NOTE,   0); }

static void send_note(uint8_t note, uint8_t velocity) {
    Serial1.write(0x90 | MIDI_CHANNEL);       // Note On (vel 0 == Note Off)
    Serial1.write(note);
    Serial1.write(velocity);
}
```

### Approach 4 — Program Change for profile-switch actions

**Mapping:** Button 1's action is "switch to profile 7" → emit PC#6 on channel 1.

```text
0xC0 0x06          // PC, ch 1, program 6   (MIDI is 0-indexed; "profile 7" → wire value 6)
```

No release message — Program Change is a one-shot.

```cpp
void on_button_1_press_action_switch_profile(uint8_t profile_1_indexed) {
    Serial1.write(0xC0 | MIDI_CHANNEL);
    Serial1.write(profile_1_indexed - 1);     // convert UI 1–128 → wire 0–127
}
```

> ⚠ **0-vs-1 indexing trap.** The MIDI spec uses 0–127 for PC numbers, but most consumer gear and DAWs display them as 1–128. Document the UI/wire convention explicitly in the app's mapping editor so users don't off-by-one their patches.

### Hybrid (likely v1 default) — PC for profile actions, Note On/Off for everything else, CC for long/double

```cpp
void on_button_1_press()        { send_note(BUTTON_1_NOTE, 127); }
void on_button_1_release()      { send_note(BUTTON_1_NOTE,   0); }
void on_button_1_long_press()   { send_cc(BUTTON_1_LONG_CC,  127); }
void on_button_1_double_press() { send_cc(BUTTON_1_DOUBLE_CC, 127); }
// when the button's bound action is "switch profile":
void on_button_1_profile_switch(uint8_t profile_1_indexed) { send_pc(profile_1_indexed - 1); }
```

Off-the-shelf gear understands the Note On/Off and PC traffic out of the box. The CC traffic for long / double is "ours" — only the AwesomeStudioPedal ecosystem needs to know what it means.

### BLE-MIDI variant of the same scenario

BLE-MIDI wraps each MIDI message in a 13-bit timestamp header before writing it to the BLE characteristic. The payload bytes are identical to DIN/TRS MIDI; only the framing changes:

```text
[0x80 | (ts_high & 0x3F)]  [0x80 | (ts_low & 0x7F)]  0x90 0x3C 0x7F
└── header byte ─────────┘└── timestamp byte ─────┘ └── MIDI bytes ─┘
```

So the firmware emit-path looks like:

```cpp
static void ble_midi_send(const uint8_t* msg, size_t n) {
    uint16_t ts = millis() & 0x1FFF;          // 13-bit ms timestamp
    uint8_t  packet[5];
    packet[0] = 0x80 | ((ts >> 7) & 0x3F);    // header byte
    packet[1] = 0x80 | ( ts       & 0x7F);    // timestamp byte
    memcpy(&packet[2], msg, n);
    ble_midi_characteristic_notify(packet, 2 + n);
}
```

Same Note On bytes (`0x90 0x3C 0x7F`) — different transport.

### Receiving side (if we ever go bidirectional)

A minimal byte-level state machine for parsing incoming MIDI:

```cpp
static uint8_t status_, data_[2], data_count_, expected_;

void midi_rx_poll() {
    while (Serial1.available()) {
        uint8_t b = Serial1.read();
        if (b & 0x80) {                       // status byte
            status_     = b;
            expected_   = ((b & 0xE0) == 0xC0) ? 1 : 2;   // PC takes 1 data byte; most others 2
            data_count_ = 0;
        } else if (status_) {                 // data byte
            data_[data_count_++] = b;
            if (data_count_ == expected_) {
                handle_message(status_, data_[0], data_[1]);
                data_count_ = 0;              // ready for "running status" repeat
            }
        }
    }
}
```

This is roughly 30 lines of code per direction — small, but enough to be wrong in subtle ways (running status, real-time messages 0xF8–0xFF interleaved mid-message, SysEx framing). A vetted library (e.g. `arduino-midi-library`, `MIDIUSB`, or rolling our own on top of the project's existing event-loop) avoids the foot-guns.

## Helpful ICs and off-the-shelf modules

MIDI hardware is well-trodden territory; we wouldn't be designing anything novel. The likely parts list:

**MIDI input (optoisolation — required by spec):**

- **H11L1** — fast logic-output optocoupler; the modern MIDI-input default. Output is already logic-level, no extra Schmitt trigger needed. ~€0.50.
- **6N137** — fast, single-channel, very common; needs a pull-up. ~€0.50.
- **6N138** — the classic. Cheap and everywhere, but its slow rise time (~3–5 µs) is marginal at MIDI's 31.25 kbaud and gets called out as the cause of flaky inputs in old designs. Avoid for new builds.
- **PC900V / TLP2361 / ACPL-M61L** — modern fast alternatives if H11L1 is hard to source.

**MIDI output (line driver):**

- No dedicated IC needed. The spec is a current-loop driven from 3.3–5 V logic through two series resistors. A **74HCT14** (Schmitt inverter) or **74HCT04** in the canonical inverter-pair configuration works; many hobby designs just use a single GPIO with two resistors and skip the buffer entirely.

**UART:**

- **Not needed as a separate IC.** MIDI is async serial at 31.25 kbaud — both the ESP32 and the nRF52840 have spare hardware UARTs. Dedicated MIDI chips don't exist in the modern parts catalogue; people just use a UART.
- Only consideration: if all UARTs are already allocated, **MAX3110/MAX3111** (SPI-to-UART) buys an extra port. Almost certainly overkill here.

**USB-MIDI:**

- Both the **ESP32-S2/S3** and the **nRF52840** have native USB device controllers, so USB-MIDI is a TinyUSB firmware feature with **zero extra hardware**. The classic ESP32 (no native USB) would need a USB controller IC — not worth it; pick a different transport.

**BLE-MIDI:**

- Zero extra hardware on either MCU — pure firmware (custom GATT service implementing Apple's BLE-MIDI profile).

**Galvanic isolation alternatives (overkill for us):**

- **Si8662 / ADuM12xx** digital isolators — faster than optos but more expensive and harder to source. The MIDI spec was written around opto-isolators; sticking with them keeps us in well-trodden territory.

**Drop-in modules — the interesting one for this project:**

- **Adafruit MIDI FeatherWing** — a stackable add-on board for the Adafruit Feather family with DIN-5 in + out, the optoisolator, the driver, and the jacks all already on it. Since the pedal already uses the **Adafruit Feather nRF52840**, this is literally a plug-on prototype path — no custom PCB needed to validate the DIN-5 story. (Verify current availability before committing.)
- **SparkFun MIDI Shield** — Arduino form factor, less relevant for our enclosure.
- Various Tindie / AliExpress "MIDI breakout" boards — cheap but quality varies.

**Implication for phasing:** the existence of the Adafruit MIDI FeatherWing means the "DIN-5 hardware" step (phase 3 below) is *much* cheaper to prototype than building a custom MIDI front-end. We can validate the user-facing question ("does anyone actually plug this into a MIDI pedalboard?") without committing to a hardware revision.

## Architecture options — integrated vs. co-processor

There are two fundamentally different shapes this can take, and the choice ripples through everything else.

### Option A — MIDI handled by the main MCU

The ESP32 / nRF52840 directly drives the MIDI UART, runs the BLE stack, and handles the button event loop on the same chip.

- **Pros:** one MCU, one firmware, one build, no inter-chip protocol to design or debug.
- **Cons:** MIDI timing competes with BLE stack and main-loop work; jitter on stomp-triggered program changes is a real risk. UART pin budget tightens (especially on the nRF52840 once BLE and serial-log are accounted for).
- **Best fit when:** v1 is BLE-MIDI only (zero extra hardware) or DIN-5 *output* only (low real-time pressure).

### Option B — Dedicated MIDI co-processor on a private serial bus

A small companion MCU owns MIDI exclusively (input optoisolation, output drive, USB-MIDI if any) and exposes it to the main pedal MCU via a private serial protocol (UART, SPI, or I²C).

- **Pros:**
  - **Isolation:** real-time MIDI work is physically separated from BLE and the main loop — no jitter cross-talk.
  - **Modularity:** the MIDI board becomes an optional add-on; the base pedal doesn't pay BOM cost if the user doesn't want MIDI.
  - **Portability:** the same MIDI module works whether the pedal is ESP32- or nRF52840-based.
  - **Electrical isolation:** the noisy MIDI front-end (and its DIN cabling) stays away from the BLE radio.
- **Cons:**
  - Two MCUs to flash, version, and debug.
  - Two firmwares; protocol-drift risk between them.
  - Higher BOM and a more complex enclosure story (the co-processor needs a home).
- **Bus question — and the link to [IDEA-013](idea-013-bus-system.md):**
  - The "private serial protocol" between main MCU and MIDI co-processor is essentially **the same problem as IDEA-013 (Bus System)**. If we pursue this option, MIDI becomes the first concrete consumer of that bus — which is a healthy forcing function (designing a bus *for* something beats designing one in the abstract).
  - Inversely, if IDEA-013 is far off, designing a one-off main↔MIDI protocol here risks duplicating later work.

**Candidate co-processor MCUs (all cheap, all over-spec for MIDI alone):**

| MCU | Approx. unit cost | Why it fits |
|---|---|---|
| RP2040 | ~€1 | 2 cores, multiple UARTs, PIO can bit-bang exotic transports |
| ATSAMD21 | ~€2 | Native USB → could even own USB-MIDI directly |
| STM32F0 / STM32G0 | ~€1 | Multiple UARTs, well-supported toolchain |
| CH32V003 (RISC-V) | ~€0.15 | Aggressively cheap; one UART is enough for output-only |
| ATmega328P | ~€2 | Familiar / Arduino-class, but only one HW UART |

**When this option wins:** v2 hardware revision targeting users with real MIDI pedalboards, especially if [IDEA-013 (Bus System)](idea-013-bus-system.md) is moving in parallel and needs a real consumer. Also a natural fit for [IDEA-008 (Hybrid Tool with DSP)](idea-008-hybrid-tool-with-dsp.md), where a co-processor pattern already makes sense for audio/DSP work.

**Decision deferred** — pick after the audience question is answered. If BLE-MIDI alone satisfies real users, Option A is enough forever. If real DIN/TRS demand exists, Option B opens cleaner upgrade paths.

## Firmware challenges

- **MIDI message routing:** which pedal events map to which MIDI messages? Single-press → PC#7? Double-press → CC#64? This needs to be configurable per profile.
- **Mapping model:** does MIDI sit *alongside* the existing action model (Key / Macro / etc.), or is it a new **MIDI** action type? The latter is cleaner but multiplies the action-type matrix.
- **BLE-MIDI vs the existing BLE HID service:** can both coexist on one BLE connection, or does the user have to pick a mode at boot?
- **Timing:** MIDI is real-time; jitter on a stomp-triggered program change is noticeable. ISR path needs to be cheap.

## Configuration questions

- **Where does the MIDI config live in `config.json`?** New top-level `midi` block, or per-profile, or per-action?
- **Channel selection:** MIDI has 16 channels — global setting or per-action?
- **Discoverability:** how does the app expose MIDI options without overwhelming users who don't care? Probably hidden behind an "Advanced / MIDI" toggle on the profile screen (relates to [IDEA-059](idea-059-reduced-expert-views-for-profiles.md)).
- **App support:** does the Flutter app need a MIDI mapping editor, or is it text-only in `config.json` for v1?

## Rough phasing if pursued

A possible minimum-viable shape, in order of increasing commitment:

1. **BLE-MIDI only**, output direction only, hardcoded PC messages on button events — proves user demand at near-zero hardware cost.
2. **Per-profile MIDI mapping** in `config.json`, app editor optional.
3. **TRS MIDI output** on a hardware revision — opens the real pedalboard market.
4. **Input + bidirectional sync** (the pedal reflects rig state).

Each step is its own go/no-go.

## Open questions before this becomes a task

- [ ] Verify the audience: is there a measurable population of MIDI-pedalboard users in our target persona?
- [ ] Pick the protocol variant(s) for v1.
- [ ] Pick the architecture: **Option A (integrated)** vs **Option B (dedicated MIDI co-processor on a private serial bus)** — and if B, decide whether to lift the bus design into [IDEA-013](idea-013-bus-system.md) or design a one-off main↔MIDI link.
- [ ] Decide whether MIDI is a new action type or a parallel transport for existing actions.
- [ ] Decide whether this lives on the current ESP32 / nRF52840 hardware or waits for [IDEA-008 (Hybrid Tool with DSP)](idea-008-hybrid-tool-with-dsp.md).
