---
id: IDEA-033
title: Shorten double-press window to reduce single-press latency
category: firmware
description: 300 ms defer on every single-press feels sluggish — find the smallest window that still lets humans double-tap reliably.
---

## Motivation

After fixing the double-press bugs uncovered by TASK-109 (see the re-evaluation
note in [task-109-ondevice-multipress-test.md](../../tasks/open/task-109-ondevice-multipress-test.md)),
`Button::event()` now defers reporting a single press by up to
`doublePressWindow_` (default **300 ms**). This is correct behaviour — without
the defer, the first tap of a double-press leaks out as a single-press action
before the second tap arrives.

But 300 ms is a long time. On every ordinary pedal press the user now waits
nearly a third of a second before anything happens. For a stomp-box musicians
play in time with, this will feel sluggish.

## Open questions

- What is the shortest double-press window that humans can still hit reliably?
  Informal testing suggests **~180–220 ms** is achievable for practised users,
  but we don't have data.
- Should the window be configurable per profile or per button?
- Does it matter whether the button is the one bound to a double-press action
  at all? Buttons without any double-press handler could fire `event()`
  immediately — no defer needed. (Equivalent to the `BUTTON_SELECT` opt-out
  already discussed.)

## Rough approach

1. **Measure, don't guess.** The fair way to pick a number is to use the
   hardware test rig (once it exists) to generate precise double-tap pulses
   at varying intervals, plus a small user study where a handful of people
   try to double-tap at decreasing intervals and we log the success rate.
2. **Pick a default** based on the data — probably somewhere in 180–250 ms.
3. **Per-button opt-out**: add `Button::setDoublePressEnabled(bool)` so
   buttons with no double-press action (e.g. SELECT, or pedal buttons whose
   active profile does not bind a double-press handler) skip the defer
   entirely. `ProfileManager` / config-loader would toggle this when the
   active profile changes.
4. **Per-profile override**: consider exposing `doublePressWindowMs` in the
   profile config so users can tune it to their own double-tap speed.

## Prerequisites

- Hardware test rig (mentioned as the gate in TASK-109) — we need precise,
  repeatable pulses to measure the tradeoff between double-press reliability
  and single-press latency.

## Notes

Deferred until the hardware test rig is available. Raised by TASK-109 review
on 2026-04-24.
