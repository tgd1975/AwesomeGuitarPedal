---
id: TASK-365
title: Scope the BLE services developer guide — file structure and existing-doc disposition
status: open
opened: 2026-05-02
effort: Small (<2h)
complexity: Medium
human-in-loop: Clarification
epic: ble-services-developer-guide
order: 1
---

## Description

Decide the structural questions that must be settled before any prose is drafted for the BLE services developer guide (EPIC-027):

1. **File structure.** Single file at `docs/developers/BLE_SERVICES_GUIDE.md`, or a folder `docs/developers/ble-services/` with sub-pages per section? Single doc with an in-line *Service catalog* section, or split into a recipe doc + a reference doc that get refreshed at different cadences?
2. **Disposition of the three existing BLE docs** — for each of `BLE_CONFIG_PROTOCOL.md`, `BLE_CONFIG_IMPLEMENTATION_NOTES.md`, `BLE_READBACK_IMPACT.md`: keep as-is (with cross-link from the new guide), fold into the new guide (with `git mv` and content reshape), or replace.

Output: a short scoping note recorded in this task's closing comment or in a small decision-record file. No prose in the main guide drafted yet — that lands in TASK-366 / TASK-367 / TASK-368.

## Acceptance Criteria

- [ ] File-structure decision recorded (single vs folder; single doc vs recipe+reference split).
- [ ] Disposition decision recorded for each of the three existing BLE docs (keep / fold / replace).
- [ ] If any existing doc is to be folded, a `git mv` plan is documented (target path + content-reshape sketch). The `git mv` itself does not happen in this task — it lands as part of TASK-366 when the catalog/recipe sections absorb the content.

## Test Plan

No automated tests required — change is non-functional.

## Notes

- **Docs-only constraint per EPIC-027.** Do not propose tooling, codegen, sync scripts, skills, or CI checks here. Drift is acknowledged in the epic body and addressed (if at all) by a separate non-docs-only idea.
- The single-file vs folder trade-off: a single file is easier to navigate and link to, but mixes update cadences (catalog rots faster than recipe). A split adds a "where do I look first?" question. Recommend defaulting to single file unless one section grows to >800 lines.
- This task is the only one in EPIC-027 with `human-in-loop: Clarification` — the structural decisions need user sign-off before the rest of the epic proceeds.
