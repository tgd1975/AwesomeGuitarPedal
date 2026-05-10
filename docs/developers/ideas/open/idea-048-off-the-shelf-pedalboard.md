---
id: IDEA-048
title: Off-the-shelf pedalboard build — 2 pedals from mostly stock components
category: hardware
description: A 2-pedal board assembled from as many off-the-shelf parts as possible (enclosure, switches, jacks, hardware) — minimise custom fabrication.
---

## Motivation

Most current pedal builds in this project lean on 3D-printed enclosures and
custom-cut wood/metal panels. That works for one-offs but raises the barrier
for builders who don't own a printer, a CNC, or a workshop. An "off-the-shelf"
variant — where every mechanical and electrical part can be bought as-is from
Amazon / Reichelt / Thomann / Mouser — would:

- give builders a low-effort entry path (no fabrication required),
- pin down a reproducible BOM that doesn't drift with printer settings,
- and serve as a sanity-check on what the project really needs to fabricate
  vs. what we're fabricating out of habit.

Scope is **2 pedals on a small board** — not the full 4-button stomp setup —
because two is the minimum useful count (one action + one navigation, or
two foot switches on a bus) and keeps the BOM tight.

## Constraints

These are hard requirements that pre-empt some otherwise tempting "off-the-shelf"
choices (e.g. a stock diecast aluminium guitar-pedal enclosure):

- **Not a fully metallic enclosure.** The controller communicates over BLE,
  and a closed metal box acts as a Faraday cage that kills or severely
  attenuates the signal. Acceptable: plastic, wood, or hybrid enclosures
  with a non-metallic lid / window / panel over the antenna. A metal
  *bottom plate* is fine; a metal *top* over the radio is not.
- **Solid enough to be operated by foot.** It doesn't need to survive a
  heavy stomp like a touring-grade guitar pedal, but it must withstand
  normal foot pressure on the switches without flexing the enclosure,
  popping the lid, or lifting off the floor. Rubber feet / non-slip pads
  are part of the BOM, not optional.
- **Switches rated for foot use.** Tactile micro-switches are out — they
  fail mechanically under foot pressure even with a button cap. Momentary
  stomp switches or equivalent foot-rated push-buttons only.
- **Minimal fabrication.** Drilling holes for the foot switches and LEDs
  in an otherwise stock enclosure is acceptable — that's a hand-drill
  job most builders can do on a kitchen table. Anything beyond that
  (cutting slots, milling rectangular jack windows, custom-printed
  panels, structural CNC work) needs a really good reason and should be
  called out explicitly. The default brief is: buy parts, drill a few
  round holes, assemble.

## Rough approach

- Pick a stock pedalboard / hobby-electronics enclosure that's deep enough for
  the controller + battery and wide enough for two foot switches.
- Use commodity 3PDT or SPST momentary stomp switches (the ones every guitar
  pedal kit uses) instead of arcade buttons or custom mounts.
- Standard 6.3 mm jacks for any audio/sleeve passthrough; barrel jack for
  power; off-the-shelf USB-C breakout for charging if a LiPo is included.
- Controller: whichever ESP32 / nRF52840 dev board the rest of the project
  already supports — no custom PCB.
- Wiring: hand-soldered to a perfboard or a generic prototype board. No
  custom PCB in the v1.

## Open questions

- What's the enclosure? A Hammond 1590BB-style diecast is ruled out by
  the no-fully-metallic constraint. Candidates: pre-drilled 2-switch
  pedal kits with a plastic top, plastic project boxes from Hammond's
  ABS lines, wooden cigar-box-style enclosures, or hybrids (metal frame
  with a plastic lid over the radio). Which one is actually in stock at
  a sensible price and only needs round holes drilled for switches/LEDs?
- Two switches connected how — direct GPIO, or via the bus system
  (IDEA-013) so the same hardware scales to N pedals later?
- Is the "off-the-shelf" constraint hard (zero custom parts) or soft
  (printed top label / sticker is OK, but no printed structural parts)?
- Does this overlap with IDEA-012 (rugged metal/wood two-button pedal),
  or is it a deliberately *cheaper / simpler* sibling? Both can coexist
  if the personas differ — one is "production-grade", this one is
  "anyone can build it in an evening".
- BOM target price? Useful to set a number now so the enclosure / switch
  choice doesn't drift upmarket.

## Relation to other ideas

- **IDEA-011 (PCB Board Design)** — explicitly *not* this idea. The point
  here is to avoid custom PCBs.
- **IDEA-012 (Two-Button Rugged Pedal)** — same form factor, opposite
  philosophy (workshop-grade fabrication vs. parts-bin assembly).
- **IDEA-013 (Bus System)** — if adopted, this 2-pedal build is the
  natural minimum-viable demonstrator.
- **IDEA-018 (BOM — Prototype)** — the deliverable here is essentially a
  second BOM document for the off-the-shelf variant.
