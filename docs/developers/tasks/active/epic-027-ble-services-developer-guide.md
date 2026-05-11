---
id: EPIC-027
name: ble-services-developer-guide
title: BLE services developer guide
status: open
opened: 2026-05-02
closed:
assigned:
branch: feature/firmware
---

Seeded by IDEA-056 (BLE services developer guide — authoritative doc for implementing/changing BLE services).

Produce a single authoritative developer guide under `docs/developers/` that becomes the first thing a developer reads when adding or changing a BLE service or characteristic. The motivating pain is that BLE knowledge is scattered across firmware code, three existing BLE docs (`BLE_CONFIG_PROTOCOL.md`, `BLE_CONFIG_IMPLEMENTATION_NOTES.md`, `BLE_READBACK_IMPACT.md`), the host/app code, the tests, and a long tail of rationale that lives only in commit messages and closed-task postmortems. Recent BLE work (TASK-357, EPIC-026) keeps re-answering the same questions and rediscovering the same cross-cutting invariants.

**This epic is docs-only.** It must not change firmware, app code, tests, build scripts, CI, hooks, or any other implementation surface. Any catalog-generation tooling, skill integration, or staleness-check automation is explicitly out of scope and belongs in a separate non-docs-only idea/epic.

## Scope

- One or more `.md` files under `docs/developers/` covering: *Service catalog* (hand-written), *Recipe: add/change a service*, *Conventions*, *Cross-cutting invariants table*, *Gotchas*, *Tests*, *References*.
- Disposition of the three existing BLE docs (keep / fold / replace, with `git mv` if folding).
- A worked example based on EPIC-026's TASK-354 (firmware-version READ) — *described in the guide*, not implemented as part of this epic.

## Out of scope

- Catalog-generation tooling (YAML manifests, extraction scripts, firmware-side schemas).
- A new BLE-implementation skill that triggers on edits to BLE files.
- A CI / pre-commit staleness check.
- Any change to firmware, app, tests, or build scripts.

## Tasks

Tasks are listed automatically in the Task Epics section of `docs/developers/tasks/OVERVIEW.md` and in `EPICS.md` / `KANBAN.md`.
