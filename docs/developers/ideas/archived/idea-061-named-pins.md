---
id: IDEA-061
title: Named pins — portable pin vocabulary for shareable profiles
description: Map pin IDs (D23 → pedal_a) to a curated standard vocabulary so profiles are portable across builds; no free-form names.
category: ⚡ firmware
---

## Archive Reason

2026-05-11 — Promoted to EPIC-029 (named-pins) with TASK-377 through TASK-382.

## Problem

Today profiles reference pins by hardware identifier (e.g. `D23`). That couples a profile to one specific board layout: a profile authored on one builder's wiring is meaningless on another builder's. For shareable profiles to be useful across the community, pins need a portable name.

## Proposal

Two modes, side-by-side, builder picks per pin:

a) **Direct** — `D23` (current behaviour, unchanged; always works).

b) **Named (standard)** — hardware config maps `D23 → pedal_a`; profiles reference `pedal_a`. The name comes from a **strongly encouraged standard set** (e.g. `pedal_a`, `pedal_b`, `expression_1`, `bank_up`, …) baked into the apps/configurators/schemas so autocomplete and validation push builders toward the standard.

## Rejected: free-form names

A `D23 → "user text"` third mode was considered and dropped. Free text in a community-shared artifact needs human curation to keep inappropriate strings out of the community section, and the curation cost outweighs the flexibility. Instead: **if a name is missing from the standard set, the builder files a GitHub idea** to extend the set. That is acceptable builder-scope friction — we are not asking musicians to do it.

## Why the standard set is load-bearing

Without it, named pins fragment immediately and shareable profiles regress to the same coupling problem the direct mode has. The whole value of mode (b) depends on convergence on a curated vocabulary.

## Scope

Builder-facing. Hardware config schema change, profile schema change to accept either form, configurator/app UI surfacing the standard set (autocomplete, validation), and a documented process for proposing additions to the standard set via GitHub.

## Open questions

- What is the v1 standard name set? Needs an inventory of common pedal roles.
  - **Answer:** Defer to a subsequent task. We already have a lot of text in this project (docs, profiles, examples) where the list can plausibly be inferred automatically rather than enumerated by hand.
- Does the profile schema accept mixed direct + named in one profile, or all-or-nothing per profile?
  - **Answer:** Mixed. A single profile may reference some pins by direct ID and others by name.
- How does the configurator surface "this profile uses `pedal_a` but your hardware config has no mapping for it" — error, warning, prompt to map?
  - **Answer:** Warning, possibly with a prompt to map — **not** an error. A missing mapping for one pin (e.g. `button_d`) may be perfectly acceptable when the builder only cares about `button_a`, `button_b`, and `button_c` on their hardware.
- Where does the standard-name list live (one file in this repo? generated from schema?) and how is it kept in sync across firmware / app / docs?
  - **Answer:** One file — likely a schema. Exact shape and sync mechanism to be figured out in the implementing task.
