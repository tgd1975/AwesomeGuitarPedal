---
id: IDEA-012
title: Two-Button Rugged Pedal — Metal/Wood Enclosure
category: hardware
description: A stomp-proof two-button pedal variant with a metal top panel, wooden side elements, LiPo battery, ESP32 with display, back panel, and dual LEDs per button — the production-grade alternative to the 3D-printed prototype.
---

## Vision

A rugged, stage-ready enclosure for the two-button variant. This is explicitly **not**
a 3D-printed prototype — it is built to be stomped on night after night. The aesthetic
combines an industrial metal top with warm wooden sides, similar to boutique guitar pedal
construction.

---

## Enclosure Design

### Top panel — brushed steel sheet

- Laser-cut or CNC-milled **steel sheet**, 2–3 mm thickness.
- Surface finish: **brushed metal** (grain direction along the long axis) — no powder coat,
  no anodising. The raw brushed texture is the intended aesthetic.
- Carries all user-facing elements: foot switches, LEDs, display cutout.
- Must withstand repeated stomping force without flex or deformation.

### Side elements — solid figured hardwood

- **Solid wood, 20 mm thick** — not plywood, not veneered MDF.
- Species: a dark wood with pronounced, attractive grain figure (*Maserung*) —
  e.g. walnut (Nussbaum), smoked oak (Räuchereiche), or wenge. The grain pattern is a
  deliberate design feature, not just a structural choice.
- Finish: oil or hard-wax oil to enhance the grain without hiding it (no opaque lacquer).
- Joinery: TBD in the implementation phase once mechanical drawings are complete.
- Provides grip and dampens resonance on hard stage floors.

### Bottom — plywood sheet

- Good-quality birch plywood (9–12 mm), painted or oiled on the exterior face.
- Screwed (not glued) to the side cheeks for serviceability.
- Rubber feet at the corners to prevent sliding.
- Recessed battery compartment or through-holes for internal access if needed.

### Back panel — plywood with 3D-printed frame elements

- Base material: plywood (matching bottom), cut to fit between the side cheeks.
- **3D-printed frames/inserts** for the panel cutouts (USB-C port, power switch, BLE
  pairing button, charging LED) — printed in black PETG or ASA for durability and clean
  port geometry. This is the one place where 3D printing is appropriate: small, precise
  framing parts that would be difficult to cut cleanly in plywood.
- The plywood provides the structural backing; the printed frames provide flush, finished
  openings for each component.
- Carries: USB-C charge port, power switch, **dedicated BLE pairing button**, charging
  indicator LED, strain-relief for any wired fallback connector.

#### BLE pairing button — design constraint

The BLE pairing button **must be a physically distinct, panel-mounted button on the back
panel** — not a GPIO pin exposed on the PCB as in the current prototype. Rationale: the
current approach requires opening the enclosure or reaching a recessed pin, which is
unacceptable for a stage device. On the rugged version, pairing must be triggerable by
hand without tools, in the dark, without disassembly.

- Recessed slightly (or guarded by a small lip) to prevent accidental activation.
- Clearly labelled ("PAIR" or BLE icon).
- Wired directly from the panel cutout to the ESP32 GPIO — same firmware interface as
  the current pairing trigger, just physically relocated to the enclosure exterior.

### Bottom

- Rubber feet at the corners to prevent the pedal from sliding on smooth stage floors.
- Recessed battery compartment or sealed base plate (screwed, not glued) for serviceability.

---

## Electronics

### Foot switches

- Two **soft foot switches** (momentary, normally open) — same electrical interface as the
  existing button abstraction, so no firmware changes required.
- Rated for high cycle count and full body weight (standard 3PDT or SPST soft-click
  variants used in guitar pedal builds, e.g. Gorva Design or equivalent).

### LEDs — two per button

Each button position has two independent LEDs:

| LED | Role | Colour (proposed) |
|---|---|---|
| Status LED | Lit while the button is physically pressed (GPIO, firmware-driven) | White or blue |
| User LED | Freely assignable via profile config — mirrors the existing LED action system | Any single colour TBD |

- **Type:** 5 mm through-hole LEDs in **panel-mount sockets** — the socket clips into the
  drilled hole in the metal top from below and accepts a standard 5 mm LED from inside.
  This allows LED replacement without soldering and gives a clean, finished look from above.
- **Lens:** use **diffused** (frosted) lenses, not clear. Diffused lenses spread light
  evenly and appear soft at stage brightness levels; clear lenses are a point source and
  will be harsh and distracting under low stage lighting.
- Both LEDs are wired to ESP32 GPIOs (status LED is firmware-driven, same as user LED,
  with a dedicated handler that mirrors the button press state).

### Display

- Small OLED or TFT display (e.g. 0.96″ or 1.3″ SSD1306/SSD1351) mounted behind a
  cutout in the top panel, flush-fit with a bezel.
- Shows current profile name, BLE connection status, battery level.
- Connects to ESP32 via I²C or SPI — see IDEA-007 for display integration details.

### Power — LiPo battery

- Single-cell LiPo (3.7 V), capacity TBD based on enclosure volume (target ≥ 500 mAh for
  ≥ 8 h stage use).
- Managed by a TP4056-based or DW01-based charge/protect circuit, or an integrated
  LiPo charger module.
- Charged via USB-C on the back panel.
- Battery level reported to the display and optionally via BLE characteristic.

QUESTION: how to turn the pedal on/off?

### ESP32

- ESP32 NodeMCU-32S or equivalent module — same MCU as the standard prototype.
- Mounted on an internal PCB or on standoffs to the base plate.
- No firmware changes required beyond enabling the display driver (IDEA-007).

---

## Mechanical constraints

- Foot switch spacing: minimum 80 mm centre-to-centre so both feet can operate
  independently without overlap.
- Overall footprint: target ≤ 160 mm × 80 mm (fits on a standard pedalboard row).
- Height: target ≤ 50 mm (fits under standard pedalboard lids).
- All internal components must be accessible by removing the base plate — no permanent
  adhesive fixings for serviceable parts.

---

## Open questions

1. **Metal top fabrication** — **laser cutting with bending.** The top is laser-cut from a
   flat steel sheet and then bent into a shallow U-profile with 4 folds (*Knicke*): two
   short side returns and two end lips. This gives the top panel rigidity without added
   thickness and allows it to sit over the wooden side cheeks. Bend radii and tolerances
   need to be specified for the sheet metal supplier or makerspace press brake.
2. **Wood joinery** — to be determined during the implementation phase once the mechanical
   drawings exist. Joinery method will be derived from the drawings, not decided upfront.
3. **LED type** — **5 mm through-hole LEDs in panel-mount sockets.** Decided (see above).
   Specific colour and forward voltage TBD when the full wiring is designed.
4. **Status LED wiring** — **GPIO / firmware-driven.** The status LED is controlled by a
   dedicated firmware handler that mirrors the button press state, exactly like the user
   LED. Both LEDs on one button share the same wiring pattern; no separate hardware path.
5. **PCB vs. point-to-point wiring** — not decided yet. A custom PCB (see IDEA-011) would
   be cleaner and more reproducible; point-to-point is more accessible. Decision deferred
   until the circuit diagram and BOM are complete.

---

## Deliverables — full build tutorial

This idea is not complete until it produces a self-contained build tutorial that a
competent maker can follow from scratch. The tutorial consists of the following artefacts,
all committed to the repository and published via the MkDocs site (IDEA-022):

### 1. Circuit diagram (schematic)

- Full schematic covering: ESP32 pinout, two foot switches, four LEDs (2 × status,
  2 × user), display, LiPo charger circuit, power switch, BLE pairing button, USB-C port.
- Produced as a WireViz YAML (wiring harness view, IDEA-019) **and** a proper schematic
  (atopile or KiCad, depending on IDEA-019 decisions).
- Rendered SVG committed to the repo; schematic PDF included in the tutorial.

### 2. Technische Zeichnungen (mechanical drawings)

- Dimensioned drawings for every custom-fabricated part:
  - Top panel (flat layout with bend lines, hole positions, display cutout)
  - Side cheeks (profile, joinery detail, hole positions)
  - Bottom panel (outline, screw hole positions)
  - Back panel (cutout positions for USB-C, switch, BLE button, charge LED)
- Format: SVG (editable) + PDF (for sending to a laser cutter or sheet metal shop).
- Dimensioned in millimetres.

### 3. Bill of Materials (BOM)

- Complete part list with: component name, quantity, specification, suggested
  supplier/part number, approximate cost.
- Covers: electronics (ESP32, display, charger IC, LEDs, sockets, switches, passives),
  hardware (screws, brass inserts, rubber feet), and materials (steel sheet, wood,
  plywood, 3D print filament).
- Format: Markdown table in the tutorial + CSV for import into ordering tools.

### 4. Step-by-step build guide

Structured as a sequential tutorial with photos/diagrams at each stage:

1. **Fabrication** — ordering the steel sheet cut/bent, cutting wood cheeks, printing
   back panel frames
2. **Enclosure assembly** — joining metal top to wood cheeks, fitting bottom and back
3. **Electronics** — soldering the charger circuit, wiring LEDs into sockets, wiring
   foot switches and BLE button, mounting ESP32 and display
4. **Firmware** — flashing the firmware, configuring the profile JSON for the two buttons
5. **Final assembly** — closing the enclosure, testing, pairing to a host device
6. **Troubleshooting** — common issues (LED not lighting, BLE not pairing, display blank)

---

## Dependencies

| Idea / component | Dependency |
|---|---|
| IDEA-007 (Display integration) | Display driver and profile-name rendering must be implemented before the display on this enclosure is useful. |
| IDEA-011 (PCB board design) | A custom PCB would be the natural carrier for this enclosure's electronics. |
| Existing button abstraction | Soft foot switches reuse the current `IButtonController` interface — no firmware changes needed. |
| Existing LED action system | User LED reuses the current LED action framework — no firmware changes needed. |
