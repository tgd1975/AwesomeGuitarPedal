---
id: IDEA-053
title: Context-sensitive helper system across the Flutter app
description: Complement the static How-To page with a context-aware helper layer that surfaces the right guidance for the user's current state (no pedal paired, send failed, profile mismatch, etc.).
category: 📱 apps
---

## Motivation

EPIC-023 shipped a static How-To walkthrough — KISS, easier to write,
easier to test. This idea is the deferred follow-up: a helper layer
that watches the app's state and points the user at the next sensible
action.

A static How-To answers "how do I use this app in general". A
context-sensitive helper answers "what is wrong *right now* and what
do I do next".

The load-bearing case is **transient failure mid-flow** — e.g. a send
that aborts halfway through a write. Static help is qualitatively
useless here: the user doesn't need a tutorial, they need a one-tap
retry where they already are. Other states are nice-to-have on top of
that:

| State | Static How-To says | Context helper would say |
|---|---|---|
| Send failed mid-write | (nothing — user opens How-To and reads from the top) | Inline banner: "Send interrupted. Reconnect and retry?" with a one-tap retry |
| No pedal paired | "Step 1 of 5: pair your pedal…" | A persistent "Tap to pair" prompt on the main screen |
| Profile count mismatch with firmware | (nothing) | Inline note: "Pedal supports 4 profiles, this config has 6 — last 2 will be ignored" |
| First launch, BLE permission denied | (nothing useful) | Direct link to OS settings + one-line explainer |

## Rough approach

- Lightweight state-aware helper widget that subscribes to the same
  app-state streams the configurator already reads (connection status,
  pairing state, last send result, validation errors).
- Each "state" has a short message + zero-or-more action buttons.
- Coexists with — does not replace — the static How-To. The static
  page stays as the "I want to read the whole thing top to bottom"
  surface.
- Probably a helper-banner widget at the top of each screen that
  collapses to a single-line "ⓘ" affordance when there's nothing
  urgent.

### Scope split

Two halves with very different cost profiles — worth separating so the
cheap half can ship without committing to the whole framework:

- **Validation-style helpers** (profile count mismatch, config-vs-firmware
  warnings) are pure-Dart, derive from data already in the configurator,
  and could land as a standalone task in the configurator area.
- **Connection/pairing/transient-failure helpers** are the part that needs
  app-wide state plumbing and is the real reason this idea exists.

## Constraints

No telemetry. This is a passive-assets-only project stance — the
helper cannot phone home to learn which states fire in the wild. Any
"which states matter?" signal has to come from GitHub issues, not
in-app analytics.

## Candidate stuck states

Brainstorm input to open question 1 — not a commitment to build any
of these. Grouped by the scope-split halves. Items that already
appear in the Motivation table are repeated here so the list is
self-contained.

**Connection / pairing** (framework half)

- Bluetooth turned off at the OS level
- Location services off (Android BLE-scan requirement)
- BLE permission granted at install, later revoked in OS settings
- Pedal previously paired but currently out of range or powered off
- Stale bond — phone thinks it's paired, pedal does not
- Multiple pedals discovered nearby — which one?
- Pedal battery low, link keeps dropping

**Send / write transients** (framework half)

- Send failed mid-write
- Send timed out — no response within the expected window
- Send succeeded at the BLE layer but the pedal rejected the payload
  (firmware-side validation)
- BLE link dropped during a multi-write batch — partial state left on
  the pedal
- User navigated away with unsaved local edits

**Validation / capability mismatch** (cheap half — validation helpers)

- Profile count mismatch with firmware
- Firmware older than the app expects — missing characteristic or
  feature
- Firmware newer than the app supports — unknown fields, how to
  surface?
- Field value out of range for the connected firmware
- Imported config file targets a different firmware version

**App lifecycle / OS state** (mostly framework half)

- First launch, BLE permission denied
- Cold launch, never paired
- Cold launch, previously paired, pedal not nearby
- App resumed mid-write — what's the actual state of the pedal now?

## Open questions

- Which specific states cross the threshold from "fold into static
  page" to "needs a banner with an action"?
- Animation / dismissal model — sticky until resolved, or
  user-dismissible?
- Localization interplay — every helper string needs translation
  (see IDEA-052).

## Success criterion

Build the framework half only if at least ~3 distinct "stuck" states
show up in real user reports (GitHub issues) where a static How-To
would clearly have been the wrong tool. Below that threshold, the
state plumbing isn't worth the cost — fold any one-off cases into the
static page or handle them as targeted inline messages without a
framework. The validation-style half (see Scope split) is not gated
on this criterion — it can ship whenever it's cheap to add.

## Why deferred

The static How-To is the cheap, KISS path for the initial content-page
ship. The context-sensitive variant requires app-wide state plumbing
and a state catalogue we haven't built yet — easier to add once we
know which "stuck" states real users actually hit.

Decided in TASK-330 (EPIC-023 decision doc).
