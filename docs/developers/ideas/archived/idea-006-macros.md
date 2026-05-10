---
id: IDEA-006
title: Macros
category: firmware
description: Support for macros — sequences or parallel groups of actions triggered with a single button press
---

## Details

Implement support for macros, allowing complex action compositions to be triggered with a single button press.

A macro consists of **steps**, where each step can be:

- **Sequential** — actions executed one after another (e.g. switch profile, then send MIDI CC)
- **Parallel** — multiple actions fired simultaneously within the same step (e.g. send two MIDI notes at once)

Steps are ordered; actions within a step are concurrent.
