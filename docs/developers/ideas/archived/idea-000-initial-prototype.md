---
id: IDEA-000
title: Initial Prototype
category: hardware
description: The working ESP32 BLE pedal that existed before any ideas were tracked — hardcoded, single-platform, no configurator.
---

Everything that was in place at **v0.1.0** (December 2021), before the idea and task
system existed. This is the baseline the project grew from.

## What was there

A single ESP32 board wired up as a BLE HID keyboard device. Four action buttons
(A, B, C, D) and one SELECT button, all interrupt-driven. Three binary-encoded
profile-select LEDs plus a power LED and a Bluetooth status LED.

Button-to-action mappings were hardcoded in `main.cpp`. Swapping an action meant
editing C++ and reflashing. There was no JSON, no profile file, no configurator,
no CLI, and no app.

The action vocabulary was small but functional: send a single character, send a
string, send a media key. Profile switching worked — SELECT cycled through up to
seven profiles encoded in the three LEDs — but the profiles themselves were baked
into the source.

## What was not there

- No hardware abstraction — ESP32-specific code lived directly in `main.cpp`
- No configuration system — no JSON, no LittleFS, no file upload
- No test infrastructure — no unit tests, no CI
- No documentation beyond a one-line README
- No web tools, no mobile app, no CLI
- No nRF52840 support

## What this became

Everything from v0.2.0 onward: a JSON configuration system, a hardware abstraction
layer, nRF52840 support, a host test suite, web-based simulator and configurators,
a CLI tool, a Flutter mobile app, community profiles, and a full design system.
