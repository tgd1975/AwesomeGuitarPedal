#!/usr/bin/env python3
"""
Circuit schematic generator for AwesomeStudioPedal.

Draws a documentation-quality SVG schematic showing the full dev-board pinout as
an IC block. Used GPIO pins have their circuits drawn (LED+resistor or button+GND);
unused pins get NC markers; power pins get supply/ground symbols.

Usage:
    python scripts/generate-schematic.py --target esp32
    python scripts/generate-schematic.py --target nrf52840
    python scripts/generate-schematic.py --target esp32 --output path/to/out.svg
"""

import argparse
import json
import pathlib

import matplotlib
import schemdraw
import schemdraw.elements as elm

matplotlib.use("Agg")
# Deterministic clipPath ids in the emitted SVG. Without this, matplotlib
# salts each run with system entropy, producing a different
# `<clipPath id="pXXXXXXXXXX">` on every regen — which makes the docs.yml
# staleness guard fire even when the circuit is unchanged. Pin a fixed
# salt so successive regens produce byte-identical output.
matplotlib.rcParams["svg.hashsalt"] = "asp-schematic-v1"

REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
LED_R = 220    # current-limiting resistor, ohms
BTN_PU = 10    # button pull-up resistor, kΩ

# ── Pin-type constants ──────────────────────────────────────────────────────
LED   = "led"    # GPIO output → 220 Ω → LED → GND
BTN   = "btn"    # GPIO input  → push-button → GND
GND   = "gnd"    # GND pin → ground symbol
PWR   = "pwr"    # Power rail → labelled power symbol (3V3, VIN, VBAT, VBUS)
NC    = "nc"     # Unused GPIO → no-connect marker
FLASH = "flash"  # SPI-flash internal pin → no-connect, labelled FLASH
SPEC  = "spec"   # Special (EN, RST, AREF) → no-connect, labelled as-is


# ── Board definitions ───────────────────────────────────────────────────────
# Each pin: (anchor_name, outside_label, pin_type, circuit_label_or_None)
# anchor_name must be a valid Python identifier (no spaces, no leading digit).
# outside_label is what is printed on the pin lead (GPIO number / board label).

def _esp32_pins(cfg: dict) -> tuple[list, list]:
    """
    Joy-IT SBC-NodeMCU-ESP32 (30-pin, 15 per side).
    Source: https://joy-it.net/files/files/Produkte/SBC-NodeMCU-ESP32/SBC-NodeMCU-ESP32-Manual-2021-06-29.pdf
    Left/right columns read top-to-bottom, USB/antenna end at the top.

    Used pins (from data/config.json):
      Outputs: D2=STATUS (onboard blue LED), D26=BT_LED, D5/D18/D19=SEL_1/2/3
      Inputs:  D21=SELECT, D13/D12/D27/D14=BTN_A/B/C/D

    Note on STATUS: the firmware drives `ledPower` for boot-mismatch,
    delayed-action and load-error signals. On NodeMCU-style boards GPIO 2
    carries an onboard blue LED, so the schematic labels that pin STATUS
    rather than PWR_LED — the panel "power" LED is hardwired to VCC and
    is not firmware-controlled. See HARDWARE_CONFIG.md (TASK-231).
    """
    used_leds = {
        cfg["ledPower"]:     "STATUS",
        cfg["ledBluetooth"]: "BT_LED",
    }
    for i, g in enumerate(cfg["ledSelect"]):
        used_leds[g] = f"SEL_{i + 1}"

    used_btns = {cfg["buttonSelect"]: "SELECT"}
    for i, g in enumerate(cfg["buttonPins"]):
        used_btns[g] = f"BTN_{chr(ord('A') + i)}"

    def gpio(num: int, label: str) -> tuple:
        if num in used_leds:
            return (f"D{num}", label, LED, used_leds[num])
        if num in used_btns:
            return (f"D{num}", label, BTN, used_btns[num])
        return (f"D{num}", label, NC, None)

    left = [
        ("EN",   "EN",         SPEC, "EN"),
        ("VP",   "VP (D36)",   NC,   None),   # GPIO36, input-only
        ("VN",   "VN (D39)",   NC,   None),   # GPIO39, input-only
        gpio(34, "D34"),
        gpio(35, "D35"),
        gpio(32, "D32"),
        gpio(33, "D33"),
        gpio(25, "D25 DAC1"),
        gpio(26, "D26 DAC2"),
        gpio(27, "D27"),
        gpio(14, "D14"),
        gpio(12, "D12"),
        gpio(13, "D13"),
        ("GNDL", "GND",        GND,  None),
        ("VIN",  "VIN",        PWR,  "VIN"),
    ]
    right = [
        gpio(23, "D23"),
        gpio(22, "D22"),
        ("TX0",  "TX0 (D1)",   NC,   None),   # GPIO1
        ("RX0",  "RX0 (D3)",   NC,   None),   # GPIO3
        gpio(21, "D21"),
        gpio(19, "D19"),
        gpio(18, "D18"),
        gpio(5,  "D5"),
        ("TX2",  "TX2 (D17)",  NC,   None),   # GPIO17
        ("RX2",  "RX2 (D16)",  NC,   None),   # GPIO16
        gpio(4,  "D4"),
        gpio(2,  "D2"),
        gpio(15, "D15"),
        ("GNDR", "GND",        GND,  None),
        ("V33",  "3.3V",       PWR,  "3.3V"),
    ]
    return left, right


def _nrf52840_pins() -> tuple[list, list]:
    """
    Adafruit Feather nRF52840 Express (#4062) pinout.
    Source: https://github.com/adafruit/Adafruit-nRF52-Bluefruit-Feather-PCB
    Right header (12 pins) and left header (16 pins), USB at the top.

    Used pins (from lib/hardware/nrf52840/include/builder_config.h):
      Outputs: D5(P1.08)=BT_LED, D6(P0.07)=PWR_LED,
               D9(P0.26)=SEL_1, D10(P0.27)=SEL_2, D11(P0.06)=SEL_3
      Inputs:  D12(P0.08)=SELECT, A0(P0.04)=BTN_A, A1(P0.05)=BTN_B,
               A2(P0.30)=BTN_C, A3(P0.28)=BTN_D
    """
    right = [
        ("VBAT", "VBAT",          PWR,  "VBAT"),
        ("EN",   "EN",            SPEC, "EN"),
        ("VBUS", "VBUS",          PWR,  "VBUS"),
        ("D13",  "D13 P1.09",     NC,   None),
        ("D12",  "D12 P0.08",     BTN,  "SELECT"),
        ("D11",  "D11 P0.06",     LED,  "SEL_3"),
        ("D10",  "D10 P0.27",     LED,  "SEL_2"),
        ("D9",   "D9 P0.26",      LED,  "SEL_1"),
        ("D6",   "D6 P0.07",      LED,  "PWR_LED"),
        ("D5",   "D5 P1.08",      LED,  "BT_LED"),
        ("SCL",  "SCL P0.11",     NC,   None),
        ("SDA",  "SDA P0.12",     NC,   None),
    ]
    left = [
        ("RST",  "RESET",         SPEC, "RST"),
        ("V33",  "3.3V",          PWR,  "3.3V"),
        ("AREF", "AREF P0.31",    SPEC, "AREF"),
        ("GNDL", "GND",           GND,  None),
        ("A0",   "A0 P0.04",      BTN,  "BTN_A"),
        ("A1",   "A1 P0.05",      BTN,  "BTN_B"),
        ("A2",   "A2 P0.30",      BTN,  "BTN_C"),
        ("A3",   "A3 P0.28",      BTN,  "BTN_D"),
        ("A4",   "A4 P0.02",      NC,   None),
        ("A5",   "A5 P0.03",      NC,   None),
        ("SCK",  "SCK P0.14",     NC,   None),
        ("MOSI", "MOSI P0.13",    NC,   None),
        ("MISO", "MISO P0.15",    NC,   None),
        ("RX",   "RX P0.24",      NC,   None),
        ("TX",   "TX P0.25",      NC,   None),
        ("D2",   "D2 P0.10",      NC,   None),
    ]
    return right, left


# ── Drawing ─────────────────────────────────────────────────────────────────

def _draw(mcu_label: str, right_pins: list, left_pins: list, output: str) -> None:
    ic_right = [elm.IcPin(name=a, side="right", pin=lbl) for a, lbl, *_ in right_pins]
    ic_left  = [elm.IcPin(name=a, side="left",  pin=lbl) for a, lbl, *_ in left_pins]

    with schemdraw.Drawing(show=False) as d:
        d.config(fontsize=9, unit=3)

        mcu = d.add(elm.Ic(
            pins=ic_right + ic_left,
            edgepadW=3.0,
            edgepadH=0.5,
            pinspacing=2.0,
            leadlen=1.5,
            label=mcu_label,
        ))

        def draw_side(pins: list, direction: str) -> None:
            for anchor, _lbl, ptype, clabel in pins:
                pt = getattr(mcu, anchor)
                if ptype == LED:
                    d.add(elm.Resistor().at(pt).theta(180 if direction == "left" else 0)
                          .label(f"{LED_R} Ω", loc="top"))
                    d.add(elm.LED().theta(180 if direction == "left" else 0)
                          .label(clabel, loc="top"))
                    d.add(elm.Ground())
                elif ptype == BTN:
                    theta = 180 if direction == "left" else 0
                    lead = d.add(elm.Line().at(pt).theta(theta).length(0.6))
                    d.add(elm.Dot().at(lead.end))
                    pullup = d.add(elm.Resistor().up().at(lead.end).length(1.0)
                                   .label(f"{BTN_PU} kΩ", loc="right", ofst=(0.1, 0)))
                    d.add(elm.Label().at(pullup.end).label("3V3", loc="top"))
                    d.add(elm.Button().at(lead.end).theta(theta)
                          .label(clabel, loc="bot"))
                    d.add(elm.Ground())
                elif ptype == GND:
                    d.add(elm.Ground().at(pt))
                elif ptype == PWR:
                    d.add(elm.Dot().at(pt))
                    d.add(elm.Label().at(pt).label(clabel,
                          loc="left" if direction == "left" else "right"))
                elif ptype in (NC, FLASH, SPEC):
                    d.add(elm.NoConnect().at(pt))

        draw_side(right_pins, "right")
        draw_side(left_pins, "left")

        d.save(output)

    svg_path = pathlib.Path(output)
    svg = svg_path.read_text()
    svg = svg.replace(
        '<g id="figure_1">',
        '<rect width="100%" height="100%" fill="white"/>\n <g id="figure_1">',
    )
    # Strip matplotlib's wall-clock <dc:date> metadata so successive
    # regens produce byte-identical SVGs (paired with the
    # svg.hashsalt rcParam above). Without this the docs.yml
    # staleness guard fires on every regen.
    import re
    svg = re.sub(r"\s*<dc:date>[^<]*</dc:date>\n?", "", svg)
    svg_path.write_text(svg)
    print(f"Saved: {output}")


# ── Entry point ──────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(description="Generate circuit schematic SVG.")
    parser.add_argument("--target", choices=["esp32", "nrf52840"], required=True)
    parser.add_argument("--output",
                        help="Output path (default: docs/builders/wiring/<target>/main-circuit.svg)")
    args = parser.parse_args()

    default_out = str(REPO_ROOT / "docs/builders/wiring" / args.target / "main-circuit.svg")
    output = args.output or default_out

    if args.target == "esp32":
        cfg = json.loads((REPO_ROOT / "data/config.json").read_text())
        left, right = _esp32_pins(cfg)
        _draw("ESP32 NodeMCU-32S", right, left, output)
    else:
        right, left = _nrf52840_pins()
        _draw("nRF52840 Feather", right, left, output)


if __name__ == "__main__":
    main()
