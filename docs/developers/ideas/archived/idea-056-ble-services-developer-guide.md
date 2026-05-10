---
id: IDEA-056
title: BLE services developer guide — authoritative doc for implementing/changing BLE services
description: Docs-only. Single source-of-truth document for adding/changing BLE services in this project.
category: 📖 docs
---

## Archive Reason

2026-05-02 — Converted to EPIC-027 (TASK-365 / TASK-366 / TASK-367 / TASK-368).

## Motivation

BLE work in this project is currently scattered across several places:

- The firmware-side service code (`src/esp32/`, `src/ble_pedal_app.cpp`).
- Three existing BLE docs that overlap in places and are silent in others:
  - [BLE_CONFIG_PROTOCOL.md](../../BLE_CONFIG_PROTOCOL.md)
  - [BLE_CONFIG_IMPLEMENTATION_NOTES.md](../../BLE_CONFIG_IMPLEMENTATION_NOTES.md)
  - [BLE_READBACK_IMPACT.md](../../BLE_READBACK_IMPACT.md)
- The host/app-side code that consumes characteristics.
- The on-device and host tests.
- A long tail of decisions, gotchas, and "why did we do it that way" rationale that lives only in commit messages and closed-task postmortems.

When a service or characteristic is added or changed (most recently TASK-357 around `MAX_CONFIG_BYTES` reconciliation, and TASK-354 firmware-version exposure — currently blocked and re-homed to EPIC-026), there is no single document a developer can read first. This means:

- The same questions get re-answered ("what UUID convention do we use?", "READ vs NOTIFY?", "where does the byte-layout doc live?", "what test layers must be touched?").
- Easy-to-miss steps (updating the protocol doc, the reassembler header, the host parser, any BLE-specific documentation that touches the changed surface, the on-device test) are missed.
- Cross-cutting invariants (max payload sizes, MTU assumptions, endianness, version negotiation) are rediscovered rather than referenced.

## Goal

A single authoritative developer guide — somewhere under `docs/developers/` — that answers, for this project specifically:

- Which services and characteristics exist today, with UUIDs and a one-line purpose each (or a pointer to the canonical list).
- The end-to-end recipe for adding or changing a service/characteristic: every file that must change, every test layer that must be touched, every doc that must be updated.
- The conventions: UUID assignment, READ/WRITE/NOTIFY choice, payload framing, max-size constants, version handling, error semantics.
- A **cross-cutting invariants** table — for each constant or convention that spans layers (e.g. `MAX_CONFIG_BYTES`, MTU floor, endianness, version negotiation), the full list of files that must change together when it changes. This is the section that exists specifically to stop the next TASK-357.
- The gotchas: known platform quirks (ESP32 NimBLE vs Bluefruit), MTU negotiation, pairing/bonding behaviour, parallel-session test interference.
- Pointers to the relevant tests on each layer (host fakes, on-device, app integration).

## Drift acknowledgement — hand-written catalog, no codegen

This is a **docs-only** idea. The guide will not introduce a YAML manifest, a firmware-side schema, an extraction script, or any other tooling that touches the implementation. The service/characteristic catalog will be hand-written and will drift over time — accept that explicitly rather than designing around it.

Mitigations available within the docs-only constraint:

- Keep the catalog narrow (UUID, properties, one-line purpose, source-of-truth file pointer) so a refresh is cheap.
- Make the recipe step "update the catalog row" the first checklist item when adding a characteristic, so drift is caught at change time.
- If drift turns out to bite hard, file a separate non-docs-only idea for sync tooling — do not smuggle it in here.

## Decide consolidation up front

The three existing BLE docs already cover parts of this surface. Before drafting, decide for each:

| Doc | Keep / fold into guide / replace? |
|---|---|
| `BLE_CONFIG_PROTOCOL.md` | ? |
| `BLE_CONFIG_IMPLEMENTATION_NOTES.md` | ? |
| `BLE_READBACK_IMPACT.md` | ? |

Otherwise the new guide either duplicates content (drift in two places) or sits next to them as a fourth file with no clear precedence.

## Inputs to draw on

- **Codebase** — current BLE service code, the three existing BLE docs above, the host parsers, the test files. Source of truth for what *is*.
- **Closed-task postmortems and commit messages** — recent BLE tasks (TASK-357, the EPIC-026 cluster, IDEA-046 around the copy loop) carry the rationale that never made it into a committed doc. Durable, version-controlled, mineable. (Chat history is per-session and not reliably retrievable — don't depend on it.)
- **External references** — Bluetooth SIG conventions, NimBLE and Bluefruit documentation. Cite, don't copy.

## Success criterion

One concrete fail-condition, picked before promoting to a task. Candidates:

- A new dev can add a characteristic to an existing service following only the guide, without asking questions.
- The next BLE-touching task references the guide in its description and checks off its recipe items.
- The guide's recipe checklist catches at least one of the missed-step classes that doc/code reconciliation work like TASK-357 has historically surfaced (e.g. "host parser updated but on-device test not extended").

## Out of scope for this idea

**This idea is docs-only. It must not change firmware, app, tests, build scripts, CI, hooks, or any other implementation surface.** The deliverable is one or more `.md` files under `docs/developers/` and any consequential moves/redirects of existing docs. Anything else is a separate idea.

Specifically deferred:

- Any catalog-generation tooling (YAML manifests, extraction scripts, firmware-side schemas). See *Drift acknowledgement* above.
- A new `ble-implementation` skill that triggers on BLE-file edits — promote to a follow-on idea once the guide exists and a checklist has settled.
- A CI / pre-commit staleness check (warn when BLE code changes without touching the guide) — bring back only if drift actually bites.

## Open questions

- Where should the guide live? `docs/developers/BLE_SERVICES_GUIDE.md`, or a folder with sub-pages per service?
- Single doc with a *Service catalog* section, or split into a recipe doc + a reference doc (catalog) that get refreshed at different cadences?

## Rough first cut

1. Inventory the three existing BLE docs and answer the consolidation table above.
2. Draft the guide as a single file with sections: *Service catalog* (hand-written), *Recipe: add/change a service*, *Conventions*, *Cross-cutting invariants*, *Gotchas*, *Tests*, *References*.
3. **Walk one real example end-to-end before generalising** — read EPIC-026's first characteristic (e.g. the firmware-version READ described in TASK-354) as a worked example, *describing in the guide* every file an implementer would touch. Do not edit the firmware/app/tests as part of this idea — the guide describes the path, the implementer (in EPIC-026 itself) walks it.
4. Mine recent BLE-related closed tasks and commits for rationale that belongs in *Gotchas*, *Conventions*, or the invariants table.
5. Pick one success criterion from the list above and verify against it before closing the resulting task.
