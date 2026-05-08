---
id: IDEA-018
title: Bill of Materials — Prototype
category: docs
description: A complete, sourced BOM for the default 4-button prototype with part numbers, quantities, costs, and substitution notes — replacing the current placeholder list in BUILD_GUIDE.md.
---

## Problem

The current `docs/builders/BUILD_GUIDE.md` lists component types but nothing a builder can
actually act on:

> - ESP32 NodeMCU-32S development board
> - 1–26 momentary tactile buttons …
> - 1–6 LEDs + current-limiting resistors …

No part numbers. No quantities for the default build. No costs. No sourcing links. No note on
what can be substituted. A first-time builder has to make a pile of guesses before they can
order anything.

The fix is a proper BOM scoped to the **default 4-button prototype** (buttons A–D + SELECT,
3 profile-select LEDs, 1 BLE LED, 1 power LED, breadboard wiring, USB power).

---

## Scope

**Default build** (what the existing Fritzing diagrams and prototype photos show):

- 4 action buttons (A, B, C, D)
- 1 SELECT button
- 3 profile-select LEDs
- 1 Bluetooth-status LED
- 1 power-indicator LED
- Breadboard / stripboard wiring
- USB-C power (no battery)
- 3D-printed enclosure (separate line item, links to Printables)

Variants (LiPo battery, more buttons, soft foot switches) are listed separately as optional
add-ons, not part of the baseline.

---

## Draft BOM — Default 4-Button Build

> Prices are estimates as of 2026 from common EU/DE sources. Prices vary by region and supplier.
> "Generic" = AliExpress / Amazon Basics tier.

### Core Electronics

| # | Component | Specification | Qty | Est. unit price | Notes |
|---|---|---|---|---|---|
| 1 | Microcontroller | ESP32 NodeMCU-32S (38-pin, 30-pin, or compatible) | 1 | €4–8 | 30-pin and 38-pin boards both work; pin assignments in `builder_config.h` match the standard NodeMCU-32S pinout. |
| 2 | Action button (A–D) | Momentary push button, NO, 24 mm body diameter, panel-mount | 4 | €1–3 each | The 3D-printed enclosure is designed for 24 mm buttons. Smaller buttons fit with a printed adapter ring; see Printables model page. |
| 3 | SELECT button | Momentary push button, NO, any tactile type, ≥6 mm | 1 | €0.10–0.50 | Standard 6×6 mm PCB tactile switch works. A panel-mount version is more ergonomic. |
| 4 | LED — profile select | 5 mm, any colour, ~20 mA forward current | 3 | €0.05–0.20 each | Single colour per LED or RGB (using one colour channel). Colours should be visually distinct from each other. |
| 5 | LED — Bluetooth status | 5 mm, blue preferred | 1 | €0.05–0.20 | Any colour; blue is conventional for Bluetooth. |
| 6 | LED — power indicator | 5 mm, red or green preferred | 1 | €0.05–0.20 | Any colour not used for profile LEDs. |
| 7 | Current-limiting resistor | 220 Ω or 330 Ω, 1/4 W | 5 | €0.02–0.05 each | One per LED. Value is not critical: 220 Ω gives ~10 mA at 3.3 V; 330 Ω gives ~7 mA. Both are visible. Higher resistance = dimmer LED = longer battery life. |

### Wiring & Prototyping

| # | Component | Specification | Qty | Est. unit price | Notes |
|---|---|---|---|---|---|
| 8 | Breadboard | 830-tie (full size) or 400-tie (half size) | 1 | €1–3 | Half-size (400-tie) is sufficient for this circuit. Full-size is more comfortable to work with. |
| 9 | Jumper wire set | Male-male, 20–26 AWG, assorted lengths | 1 set | €2–5 | Pre-cut jumper wire sets are the easiest option. Solid core hookup wire also works. |
| 10 | USB cable | USB-C (or Micro-USB for older NodeMCU-32S boards) | 1 | €1–3 | Check your specific ESP32 board — some use Micro-USB. |
| 11 | USB power supply | 5 V, ≥500 mA, USB-A or USB-C output | 1 | €3–8 | Any phone charger or USB power bank works. A power bank enables untethered use. |

### Enclosure

| # | Component | Specification | Qty | Est. unit price | Notes |
|---|---|---|---|---|---|
| 12 | 3D-printed enclosure | Printables model 1683455 | 1 set | ~€2–5 filament | PLA for main chassis and top, PETG for mechanical parts (bolts, clips). See model page for print settings. |
| 13 | M3 screws / hardware | Per Printables model description | — | €1–3 | Check the Printables model page for the exact fastener specification — it is the authoritative hardware list for the enclosure. |

### Total Estimated Cost (Default Build, EU)

| Scenario | Estimated cost |
|---|---|
| AliExpress / generic sourcing (slower shipping) | €10–18 |
| Reichelt / Mouser / Amazon (faster, local) | €20–35 |
| Excludes enclosure filament and fasteners | — |

The "under €30" claim used in marketing is achievable via generic sourcing; EU/local sourcing
is typically €25–35 for the electronics alone.

---

## Optional / Variant Add-ons

These are not part of the baseline build but are commonly asked about.

| Add-on | Components needed | Notes |
|---|---|---|
| **LiPo battery (untethered)** | 3.7 V LiPo cell (500–1000 mAh) + TP4056 USB-C charging module + 5 V boost converter or LDO 3.3 V regulator | The ESP32 NodeMCU-32S has an on-board 3.3 V regulator but no LiPo charging circuit. A separate TP4056 + boost/LDO is the simplest approach. |
| **More action buttons** | Same 24 mm momentary buttons + jumper wires | Up to 26 buttons (A–Z) supported by firmware. Each additional button needs a GPIO pin and a pull-up (internal pull-up enabled in software — no external resistor needed for buttons). |
| **More profile-select LEDs** | Additional 5 mm LEDs + 220 Ω resistors | Each LED doubles the maximum profile count (binary encoding). 4 LEDs → 15 profiles max. |
| **Soft foot switches instead of 24 mm buttons** | SPST momentary footswitch, e.g. standard guitar-pedal stomp switch | More robust for heavy foot use. Requires larger panel holes or a different enclosure. See IDEA-012. |

---

## Sourcing Guide

### EU / Germany

- **Reichelt Elektronik** (reichelt.de) — best for resistors, LEDs, and ESP32 boards at reasonable prices with fast DE shipping. Search: `NodeMCU ESP32`, `LED 5mm`, `Widerstand 220 Ohm`.
- **Mouser Electronics** (mouser.de) — broader selection, better for specific part numbers, minimum order may apply.
- **Amazon.de** — convenient for breadboard + jumper wire sets and ESP32 boards as bundles.
- **Eckstein-Shop / Berrybase** — DE-based electronics hobbyist stores; good for NodeMCU boards and starter kits.

### Generic / International

- **AliExpress** — cheapest option for everything except lead time. Search: `ESP32 NodeMCU-32S`, `24mm push button`, `5mm LED assorted`. Allow 2–4 weeks shipping.
- **eBay** — similar to AliExpress but sometimes faster for EU sellers.

### US

- **Adafruit / SparkFun** — quality parts, good documentation, higher price. Adafruit has a compatible ESP32 Feather if you prefer that form factor.
- **Amazon.com / Digi-Key / Mouser** — standard US electronics sourcing.

---

## What the BUILD_GUIDE.md Needs Changed

The current BOM section should be replaced with:

1. A **table** (or link to a standalone `BOM.md`) with the columns above.
2. The **total cost estimate** (the "under €30" claim needs to be substantiated with actual line items).
3. A **note on the enclosure hardware** — currently the Printables model page has its own hardware list (screws, nuts) that is not reflected anywhere in the repo docs.
4. A **substitution note** for the 24 mm button — this is the most common builder question ("what if I can't find exactly 24 mm?").

---

## Relationship to Other Ideas

- **IDEA-011 (PCB Design)**: A PCB BOM will be different from the breadboard BOM — the PCB idea should eventually produce its own component list. This IDEA-018 is scoped to the breadboard/prototype only.
- **IDEA-012 (Two-Button with Soft Switches)**: That variant has its own BOM (soft stomp switches instead of 24 mm buttons, possibly a different enclosure). Out of scope here but should reference back to this document as the baseline.
- **IDEA-005 (Large Button Prototype)**: Also a distinct BOM. Same relationship.
- **IDEA-017 (Community Profiles)**: The Printables model page already has a hardware list for the enclosure. The Printables description update proposed in IDEA-017 should cross-link to this BOM.
- **IDEA-015/016 (Content Strategy)**: The BOM substantiates the cost claim ("under €30") used in marketing. It should be linked from the Maker-Musician article and the Printables description.
