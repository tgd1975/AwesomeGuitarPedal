---
id: IDEA-065
title: Raspberry Pi Zero W support
description: Pi Zero W support as a Linux-based maker variant — to be discussed; what subset of pedal features makes sense outside the MCU world?
category: 🔧 hardware
---

## Archive Reason

2026-05-11 — Not suitable for this project; see Conclusion section
(Pi Zero W would be a second product line sharing only the
business-logic core — different toolchain, OS-side setup, and release
artefact; cost not justified for a single-person project and dilutes
the no-OS positioning).

## Raspberry Pi Zero W support

Almost an outlier in the current firmware lineup — the pedal targets
bare-metal MCUs (ESP32, nRF52840) with hard real-time GPIO, USB-HID,
and a deterministic BLE stack. The Pi Zero W is a different beast: a
full Linux SoC with Wi-Fi and Bluetooth, no native USB-HID gadget mode
without extra work, and no hard real-time guarantees.

But makers love the Pi Zero W. It's cheap, widely available, has a
massive ecosystem, and is the default "I want to add a microcontroller
to this project" board for a large chunk of the hobbyist community.
Ignoring it means missing those builders entirely.

## Open questions

- **Which features port and which don't?** Button handling and BLE
  obviously work on Linux. USB-HID-as-keyboard is possible via the
  USB gadget framework but is more involved than on ESP32. Real-time
  guarantees (debounce, double-press window timing) need to be
  re-evaluated under a non-RTOS scheduler.
- **What's the deployment model?** A Pi Zero W needs an OS image, not
  a flashed firmware binary. Do we ship a Raspberry Pi OS image? A
  Buildroot/Yocto rootfs? A systemd service installed on a stock image?
- **Does the HAL refactor (IDEA-028) cover this?** The hardware
  abstraction layer was designed to decouple platform from logic, but
  it assumes a freestanding C++ target. A Linux port would likely live
  one layer above that — same logic, different I/O backend
  (libgpiod, BlueZ, /dev/hidg0).
- **Cross-build matrix cost.** Adding a third target multiplies CI
  build time, test surface, and documentation effort. Is the maker
  audience reach worth the ongoing maintenance burden?
- **No PlatformIO path.** PlatformIO supports the Raspberry Pi Pico
  (RP2040) as a first-class bare-metal target, but **not** the Pi
  Zero W. The Zero W is a Linux SBC (Broadcom BCM2835 running a full
  OS), so a port would build with native gcc / cross-compile and
  deploy via systemd or an OS image — entirely outside our existing
  `platformio.ini` build matrix. That's a meaningfully different
  toolchain, CI shape, and release artefact from ESP32/nRF52840.
- **Does it dilute the product?** The pedal's selling point so far is
  "low-power, instant-on, no boot, no OS". A Pi Zero W variant is
  none of those. Maybe it belongs as a separate "maker port" with
  honest framing rather than a first-class target.

## Rough framings to consider

1. **Full port** — Pi Zero W joins ESP32 and nRF52840 as a peer target
   with feature parity.
2. **Subset port** — only the BLE-config + button-event side runs on
   Pi; USB-HID is out of scope.
3. **Reference port only** — publish a working Pi Zero W example as
   docs/community contribution, no CI, no support promises.
4. **Decline** — keep the project MCU-only; point makers at the
   ESP32 build, which is already cheap and beginner-friendly.
5. **Pivot to Pi Pico (RP2040)** — if the goal is "an RPi-branded
   target for makers", the Pi Pico is the natural drop-in: it's a
   bare-metal MCU, PlatformIO supports it as a first-class platform,
   and it fits the existing build matrix and CI shape with far less
   friction than the Zero W. Different audience overlap (Pico is
   newer, Zero W has more Linux/Wi-Fi appeal) — worth weighing.

## Conclusion — not suitable for this project

A Pi Zero W variant is closer to "new product sharing a logic library"
than "third firmware target". Concretely:

**Reusable (the business logic only):**

- Profile model, action dispatch, debounce / double-press timing
  decisions, config parsing — anything platform-independent. The HAL
  refactor (IDEA-028) is exactly what makes this clean.

**Rewritten (every I/O backend):**

- GPIO: `libgpiod` / sysfs instead of Arduino `digitalRead`.
- BLE: BlueZ via D-Bus instead of NimBLE / Bluefruit.
- USB-HID: Linux USB gadget framework (`/dev/hidg0`, configfs) instead
  of TinyUSB.
- Timing: `clock_gettime()` plus acceptance of scheduler jitter
  instead of MCU-deterministic loops.
- Build / deploy: CMake or native gcc instead of PlatformIO; systemd
  service instead of a flashed binary.

**OS-side setup on the Pi (not code, but required):**

- `dtoverlay=dwc2` in `/boot/config.txt` plus `dwc2` / `libcomposite`
  modules to enable USB gadget mode.
- BlueZ configured and running.
- GPIO group permissions (or run as root).
- systemd unit for autostart.
- Probably a read-only rootfs setup so the device behaves like an
  appliance and survives power-pulls.

The cost is essentially a second product line sharing only the
business-logic core — different toolchain, different release artefact,
different failure modes, different support burden. The reach into the
maker audience does not justify that ongoing cost for a single-person
project, and it dilutes the "low-power, instant-on, no-OS" positioning
of the existing pedal.

Archived as not suitable for this project. Revisit if the HAL boundary
ever becomes clean enough that a Linux port is a weekend port rather
than a parallel product, or if a contributor wants to own the Pi line
end-to-end.
