---
id: IDEA-019
title: Wiring as Code — Schematic-as-Code for Documentation and Netlist Generation
category: tooling
description: Replace the Fritzing-exported PNG diagrams with code-defined schematics that CI renders automatically, produce readable diffs on circuit changes, and lay the groundwork for PCB design (IDEA-011).
---

> **IMPLEMENTED (Schemdraw)** — Circuit schematics are generated from Python source via
> [`scripts/generate-schematic.py`](../../../../scripts/generate-schematic.py) using Schemdraw.
> SVGs live at `docs/builders/wiring/<target>/main-circuit.svg` and are kept in sync by a
> pre-commit hook and CI staleness guard. Implemented: 2026-04-22 via TASK-200–TASK-207.
> See [idea-019-simplified-solution.md](idea-019-simplified-solution.md) for the full solution history.

## The Problem

The wiring diagrams in `docs/builders/BUILD_GUIDE.md` are PNG exports from a single Fritzing
file (`docs/media/AwesomeStudioPedal_esp32_wiring.fzz`). Fritzing's `.fzz` format is a ZIP
archive containing XML, so it is *technically* text — but in practice the XML is a thousand-line
blob of absolute coordinates and UUIDs. A one-wire change produces a diff that is unreadable.

The rest of the project is already docs-as-code:

- Architecture diagrams → Mermaid (validated in CI)
- Profile configuration → JSON with a schema (validated in CI)
- Firmware API docs → Doxygen (generated in CI)

The wiring source is the odd one out. It requires Fritzing installed locally, changes cannot be
meaningfully reviewed in a PR, and the exported PNGs are binary blobs with no provenance.

There are already **three separate diagram views** in the build guide:

| View | Current source | Purpose |
|---|---|---|
| Logical GPIO map | Mermaid in `BUILD_GUIDE.md` | Quick mental model — which pin does what |
| Circuit schematic | Fritzing PNG export | Electrical correctness — component values, connections |
| Breadboard layout | Fritzing PNG export | Physical assembly aid for beginners |

The goal is to make all three reproducible from source code that lives in the repo and renders
automatically in CI.

---

## Tool Landscape

### SKiDL (the user's suggestion)

SKiDL is a Python library that describes electronic circuits as code and outputs KiCad-compatible
netlists. A circuit becomes a Python function:

```python
from skidl import *

esp32 = Part('ESP32', 'NodeMCU-32S', footprint='...')
btn_a = Part('Device', 'SW_Push', footprint='...')
r1 = Part('Device', 'R', value='220', footprint='...')

esp32['GPIO13'] & btn_a[1] & gnd
esp32['GPIO26'] & r1[1]; r1[2] & led['+']
```

- **Strengths**: truly code-first; generates KiCad netlists; direct path to PCB layout
  (IDEA-011); can run DRC-style checks in pure Python; diffable.
- **Weaknesses**: generating a *visual* schematic still requires KiCad or `netlistsvg` as a
  rendering step; the tool is less maintained than KiCad itself; part libraries need setup.
- **Best suited for**: netlist generation as the authoritative source when the project moves
  toward a real PCB (IDEA-011). Not the best fit for documentation-quality diagrams *today*.

### Schemdraw

Schemdraw is a Python library for drawing circuit diagrams as code. The mental model is
"turtle graphics for schematics":

```python
import schemdraw
import schemdraw.elements as elm

with schemdraw.Drawing() as d:
    d += elm.Dot().label('GPIO13')
    d += elm.Line().right()
    d += (btn := elm.Switch().right().label('BTN_A'))
    d += elm.Line().right()
    d += elm.Dot().label('GND')
```

Output: SVG, PNG, or PDF. No KiCad dependency. Pure Python, `pip install schemdraw`.

- **Strengths**: beautiful documentation-quality output; simple Python API; CI-friendly with
  zero new infrastructure (just `pip install`); outputs SVG (scalable, web-native).
- **Weaknesses**: it is a *drawing* tool, not a *netlist* tool — it has no concept of nets,
  DRC, or component databases; it cannot generate a PCB netlist.
- **Best suited for**: replacing the Fritzing schematic PNG with a CI-generated SVG that is
  always in sync with the code and renders cleanly in docs and GitHub Pages.

### KiCad + KiBot

KiCad stores schematics in `.kicad_sch` format — a human-readable S-expression text file.
KiBot is a CI runner that takes a KiCad project and outputs: schematic PDFs/SVGs, rendered
PCB images, BOM exports, and Gerbers.

```
.kicad_sch (text, diffable) → KiBot in CI → schematic.svg + bom.csv + gerbers/
```

- **Strengths**: industry standard; `.kicad_sch` is actually readable and diffable; KiBot is
  production-tested in CI; one tool covers schematic + PCB + BOM; full DRC support; the
  KiCad project becomes the authoritative source for IDEA-011 PCB design.
- **Weaknesses**: KiCad is primarily a GUI tool — the `.kicad_sch` file is diffable but not
  the kind of "code you write by hand"; KiBot setup requires a `kibot.yaml` config file and
  Docker/native KiCad in CI; steeper learning curve than Schemdraw.
- **Best suited for**: when IDEA-011 (PCB design) is in progress — the KiCad project is the
  natural source of truth for both schematic and PCB.

### Pure Mermaid

Mermaid already renders the logical GPIO map. Its `graph` syntax could express connections,
but it has no concept of component symbols, values, or electrical conventions. A Mermaid
diagram of the schematic would look like a flowchart, not a schematic — acceptable for quick
reference but not a substitute for a real schematic view.

---

## Recommended Approach — Two Phases

The project's current needs (documentation) and future needs (PCB design via IDEA-011) call
for different tools. Rather than choosing one and compromising on either, use both, each in
its natural role:

### Phase 1 — Schemdraw for Documentation (do now)

Replace the Fritzing-exported schematic PNG with a CI-generated SVG produced by a Python
script using Schemdraw.

```
src/schematic/esp32_wiring.py   ← the source of truth
CI: python esp32_wiring.py      → docs/media/esp32_schematic.svg
```

The script lives in the repo. Changing a resistor value or adding a button means editing a
Python file — which produces a readable, reviewable diff. CI regenerates the SVG and includes
it in the GitHub Pages deployment via the existing `docs.yml` workflow.

The Fritzing `.fzz` and its PNG exports remain in the repo as a breadboard illustration (see
below), but the *schematic* view is now owned by the Python script.

**What the script defines:**

- All five GPIO outputs (LEDs + resistor values)
- All five GPIO inputs (buttons with pull-up notation)
- ESP32 as a named component block
- Power and GND rails

### Phase 2 — KiCad + KiBot for PCB Design (when IDEA-011 starts)

When the project is ready to design a real PCB, start a KiCad project. The `.kicad_sch` file
becomes the new authoritative source, and the Schemdraw script is retired. KiBot runs in CI
to generate: schematic SVG (replacing the Schemdraw output), BOM CSV (replacing IDEA-018's
hand-maintained table), and eventually Gerbers for fabrication.

This is also the phase where SKiDL could be evaluated as an alternative to hand-drawing in
the KiCad GUI — generating the `.kicad_sch` programmatically from Python rather than
drawing it interactively.

---

## Example Circuit — Default 4-Button Build

The circuit for the default ESP32 build (4 action buttons, 1 select button, 3 profile-select
LEDs, 1 Bluetooth LED, 1 power LED) has been implemented as a proof of concept in
[`idea-019-esp32-wiring.py`](idea-019-esp32-wiring.py), living alongside this document.
When the idea is implemented, the script moves to `src/schematic/esp32_wiring.py`.

Pin assignments from `data/config.json`:

| Signal | GPIO | Direction | Notes |
|---|---|---|---|
| LED: Power indicator | 25 | Output | 3.3 V → 220 Ω → LED → GND |
| LED: Bluetooth status | 26 | Output | 3.3 V → 220 Ω → LED → GND |
| LED: Profile select 1 | 5 | Output | 3.3 V → 220 Ω → LED → GND |
| LED: Profile select 2 | 18 | Output | 3.3 V → 220 Ω → LED → GND |
| LED: Profile select 3 | 19 | Output | 3.3 V → 220 Ω → LED → GND |
| Button: SELECT | 21 | Input | Pull-up; other leg to GND |
| Button: A | 13 | Input | Pull-up; other leg to GND |
| Button: B | 12 | Input | Pull-up; other leg to GND |
| Button: C | 27 | Input | Pull-up; other leg to GND |
| Button: D | 14 | Input | Pull-up; other leg to GND |

### Rendered output

![ESP32 wiring schematic](idea-019-esp32-schematic.svg)

### Schemdraw script

The script [`idea-019-esp32-wiring.py`](idea-019-esp32-wiring.py) is a working implementation.
Run it directly to regenerate the diagram above, or pass `--output` to write elsewhere.
When this idea is implemented it moves to `src/schematic/esp32_wiring.py`.

```python
#!/usr/bin/env python3
"""
Schematic for AwesomeStudioPedal — default ESP32 4-button build.
Reads pin config from data/config.json so the diagram stays in sync
with the firmware's hardware configuration automatically.

Usage:
    python docs/developers/ideas/open/idea-019-esp32-wiring.py [--output PATH]

Output defaults to docs/developers/ideas/open/idea-019-esp32-schematic.svg.
"""

import argparse
import json
import pathlib

import schemdraw
import schemdraw.elements as elm

REPO_ROOT = pathlib.Path(__file__).resolve().parents[4]
CONFIG = json.loads((REPO_ROOT / "data/config.json").read_text())

LED_R = 220  # current-limiting resistor, ohms


def main(output: str) -> None:
    with schemdraw.Drawing(show=False) as d:
        d.config(fontsize=10, unit=3)

        led_signals = [
            (CONFIG["ledPower"],     "PWR LED"),
            (CONFIG["ledBluetooth"], "BT LED"),
        ] + [(gpio, f"SEL LED {i + 1}") for i, gpio in enumerate(CONFIG["ledSelect"])]

        for gpio, label in led_signals:
            d += elm.Label().label(f"GPIO {gpio}", loc="left")
            d += elm.Resistor().right().label(f"{LED_R} Ω", loc="top")
            d += elm.LED().right().label(label, loc="top")
            d += elm.Ground()
            d += elm.Line().left(d.unit * 3)
            d += elm.Line().down(d.unit * 0.6)

        d += elm.Line().down(d.unit * 0.6)

        btn_signals = [
            (CONFIG["buttonSelect"], "SELECT"),
        ] + [(gpio, f"BTN {chr(ord('A') + i)}") for i, gpio in enumerate(CONFIG["buttonPins"])]

        for gpio, label in btn_signals:
            d += elm.Label().label(f"GPIO {gpio}", loc="left")
            d += elm.Button().right().label(label, loc="top")
            d += elm.Ground()
            d += elm.Line().left(d.unit * 2)
            d += elm.Line().down(d.unit * 0.6)

        d.save(output)

    # Inject a white background (matplotlib backend defaults to transparent)
    svg = pathlib.Path(output).read_text()
    svg = svg.replace(
        '<g id="figure_1">',
        '<rect width="100%" height="100%" fill="white"/>\n <g id="figure_1">',
    )
    pathlib.Path(output).write_text(svg)
    print(f"Saved: {output}")


if __name__ == "__main__":
    default_out = str(pathlib.Path(__file__).parent / "idea-019-esp32-schematic.svg")
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default=default_out)
    main(**vars(parser.parse_args()))
```

### Design decisions in the script

**Config-driven, not hardcoded.** The script reads `data/config.json` rather than
hardcoding GPIO numbers. If a builder changes `buttonPins` or adds a fourth select LED,
the schematic regenerates correctly without touching the Python file.

**Internal pull-ups, no external resistors on buttons.** The ESP32 firmware configures
button pins as `INPUT_PULLUP`, so no external pull-up resistor is needed. The schematic
shows the switch directly to GND, matching the physical build.

**220 Ω LED resistors.** The ESP32 outputs 3.3 V. A standard red/green/amber LED has a
forward voltage of ~2.0 V and a typical operating current of 10 mA. R = (3.3 − 2.0) / 0.010 = 130 Ω.
220 Ω is used instead — a standard E12 value that runs the LED slightly dimmer (~6 mA)
but well within safe limits and gives adequate visibility. The value is defined as a
named constant (`LED_R`) so it can be updated in one place.

**SVG output, not PNG.** Schemdraw's SVG backend produces resolution-independent output
that renders cleanly in both GitHub's Markdown preview and the GitHub Pages site. SVG files
are also text — a resistor value change produces a readable diff in the SVG source.

**`show=False` for CI safety.** Without this flag, Schemdraw tries to open a Matplotlib
window, which fails in a headless CI environment. `show=False` suppresses the GUI and
writes only to the output file.

---

## What to Do About the Breadboard View

The breadboard illustration (the colourful "spaghetti" wiring diagram) is genuinely useful
for first-time builders. There is no good code-first tool for this style of diagram — it is
inherently a visual, manually-positioned illustration.

Options:

| Option | Tradeoff |
|---|---|
| Keep the Fritzing PNG as a static asset | Simple; the breadboard layout never needs to change if the circuit doesn't change |
| Unpack the `.fzz` (it is a ZIP) into versioned XML files in the repo | Diffs become possible but still noisy; no visual output improvement |
| Replace with an annotated photo of the actual prototype | More authentic; easier to update when the circuit changes; zero tooling |
| Generate with a Python breadboard library (e.g. `bb` or `fritzing-python`) | These tools are immature and produce lower-quality output than Fritzing |

**Recommendation**: keep the current Fritzing breadboard PNG as a static file. Unpack the
`.fzz` ZIP in CI and commit the XML components as separate files so that diffs are at least
possible on component/connection changes. The schematic and logical diagrams are the ones
that benefit most from a code-first approach.

---

## CI Integration

The existing `docs.yml` workflow already runs on every push to `main` and publishes to
GitHub Pages. Adding Schemdraw is a small addition:

```yaml
- name: Install Schemdraw
  run: pip install schemdraw matplotlib

- name: Generate wiring schematic
  run: python src/schematic/esp32_wiring.py --output docs/media/esp32_schematic.svg
```

The generated SVG is included in the Pages build the same way the Fritzing PNGs currently are.
No new workflow file is needed.

For the Fritzing unpack step (optional):

```yaml
- name: Unpack Fritzing source
  run: |
    mkdir -p docs/media/fritzing_src
    unzip -o docs/media/AwesomeStudioPedal_esp32_wiring.fzz -d docs/media/fritzing_src
```

This produces XML files that land in `docs/media/fritzing_src/` and produce meaningful diffs
when the circuit is edited in Fritzing and the `.fzz` is re-committed.

---

## Migration Checklist

When this idea is implemented, the following changes are needed:

- [x] Write Schemdraw script for the default 4-button build —
      proof of concept at [`idea-019-esp32-wiring.py`](idea-019-esp32-wiring.py);
      move to `src/schematic/esp32_wiring.py` when implementing
- [ ] Add `schemdraw` and `matplotlib` to the CI pip install step in `docs.yml`
- [ ] Replace the `![Circuit schematic]` PNG reference in `BUILD_GUIDE.md` with the
      CI-generated SVG
- [ ] Add the Fritzing unpack step to CI (optional but recommended)
- [ ] Add a `scripts/validate-schematic.sh` or equivalent that runs the Python script as a
      dry-run to catch syntax errors in PRs
- [ ] Update `CONTRIBUTING.md` to explain how to edit the schematic source
- [ ] When IDEA-011 (PCB design) starts: evaluate KiCad + KiBot as the replacement for the
      Schemdraw script and the Fritzing source

---

## Relationship to Other Ideas

- **IDEA-011 (PCB Design)**: This idea is the natural predecessor. Schemdraw gives us a
  documented, CI-validated schematic *now*; KiCad + KiBot takes over when PCB design begins.
  The Schemdraw script also acts as a specification for what the PCB schematic must match.
- **IDEA-018 (BOM)**: KiBot can auto-generate a BOM CSV from the KiCad schematic in Phase 2,
  making the hand-maintained BOM in IDEA-018 redundant. Until then, the two are maintained
  in parallel.
- **IDEA-017 (Community Profiles)**: No direct relationship, but the docs-as-code philosophy
  is consistent — profiles are JSON validated by schema in CI; schematics are Python rendered
  by Schemdraw in CI; both are reproducible from source without installing GUI tools.

---

## Far-Fetched Progression — Bidirectional Fritzing ↔ Schemdraw Converter

Both Fritzing and Schemdraw store the same information: component types, positions, and
connections. Fritzing's `.fzz` XML contains explicit coordinates and wire routing alongside
the netlist. Schemdraw encodes the same thing implicitly — following the turtle, every element
has a start and end coordinate, and two elements that share a coordinate are connected.

This means conversion in both directions is theoretically just format translation:

**Fritzing → Schemdraw**: parse the schematic XML, read each component's position and
orientation, map it to the equivalent `elm.*` element, and emit `Drawing` calls that
reproduce the same layout. Wires become `elm.Line()` segments. The result is a runnable
Python script that regenerates the original diagram — and is now editable as code.

**Schemdraw → Fritzing**: walk the drawing sequence, record the start/end coordinates of
each element, identify shared endpoints as connections, and emit the equivalent Fritzing
schematic XML. Component type mapping (Resistor → `<part moduleId="ResistorModuleID"/>`
etc.) is a lookup table.

The only real asymmetry is component libraries. Fritzing has a rich library of
microcontroller-specific symbols (ESP32 with labelled pins, USB connectors, etc.).
Schemdraw's elements are generic (IC box, switch, resistor). A Fritzing schematic with
complex ICs would map to unlabelled boxes in Schemdraw — workable for documentation but
not a lossless round-trip. For simple passive circuits — which is exactly what this project
uses — the mapping is 1:1.

**What a converter tool would look like:**

```
fritzing2schemdraw input.fzz > output.py   # edit as code, regenerate SVG with python output.py
schemdraw2fritzing output.py > output.fzz  # re-import into Fritzing GUI for breadboard layout
```

This would close the gap between GUI-first and code-first workflows: builders who prefer
Fritzing can keep using it; the converter produces a diffable Python script for CI; changes
made in code can be pushed back to Fritzing for breadboard editing. Neither tool would need
to know about the other.

Building this is out of scope for this project, but the Fritzing XML schema is documented
and Schemdraw's drawing model is simple enough that a focused weekend project could produce
a working converter for the subset of components this circuit uses (resistors, LEDs,
push-buttons, wires, a microcontroller block).
