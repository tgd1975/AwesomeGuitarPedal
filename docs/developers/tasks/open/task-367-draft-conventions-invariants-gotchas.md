---
id: TASK-367
title: Draft Conventions, Cross-cutting invariants, and Gotchas sections of the BLE services guide
status: open
opened: 2026-05-02
effort: Medium (2-8h)
complexity: Senior
human-in-loop: No
epic: ble-services-developer-guide
order: 3
prerequisites: [TASK-365]
---

## Description

Write the durable-reference body of the BLE services developer guide:

1. **Conventions** — UUID assignment scheme, READ vs WRITE vs NOTIFY decision rule, payload framing (chunked-write reassembler, single-shot read, Read Long auto-segmentation), max-size constants, version handling, error semantics.
2. **Cross-cutting invariants table** — the section that exists specifically to stop the next TASK-357. For each constant or convention that spans layers (e.g. `MAX_CONFIG_BYTES`, MTU floor, endianness, version negotiation), list the full set of files that must change together when it changes. Table rows look like `MAX_CONFIG_BYTES → src/.../foo.h, lib/.../bar.cpp, BLE_CONFIG_PROTOCOL.md, test/.../baz.py`.
3. **Gotchas** — known platform quirks (ESP32 NimBLE vs Bluefruit), MTU negotiation behavior, pairing/bonding behavior, parallel-session test interference, BlueZ GATT cache invalidation. Mine recent BLE-related closed tasks (TASK-357, the EPIC-026 cluster, IDEA-046) and commit messages for the rationale that never made it into a committed doc.

## Acceptance Criteria

- [ ] Conventions section answers each listed sub-question with a one-line rule plus a pointer to a worked example or precedent in the codebase.
- [ ] Cross-cutting invariants table includes at minimum the constants surfaced in TASK-357's reconciliation work (notably `MAX_CONFIG_BYTES`).
- [ ] Gotchas section cites at least three specific closed-task or commit references for "why we do it this way".

## Test Plan

No automated tests required — change is non-functional.

## Prerequisites

- **TASK-365** — settles whether these sections live in the same file as the recipe + catalog or in a separate reference doc.

## Notes

- Can run in parallel with TASK-366 in principle (no content overlap once TASK-365 settles structure), but practical author context favors serial.
- Mining inputs are durable: closed-task postmortems and commit messages. **Do not depend on chat history** — per IDEA-056 / EPIC-027, chat history is per-session and not version-controlled.
- The cross-cutting invariants table is the most load-bearing single artifact of the entire epic — the next TASK-357-class miss is what it exists to prevent. Treat it accordingly: fewer rows, each verified against a `grep` of the actual constant.
- After writing, invoke `/doc-check` since the file lives under `docs/developers/`.
