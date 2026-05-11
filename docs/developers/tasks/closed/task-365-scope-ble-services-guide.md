---
id: TASK-365
title: Scope the BLE services developer guide — file structure and existing-doc disposition
status: closed
closed: 2026-05-11
opened: 2026-05-02
effort: Small (<2h)
effort_actual: Small (<2h)
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

## Scoping decisions (2026-05-11)

User-confirmed via clarification on 2026-05-11. These decisions bind TASK-366 / TASK-367 / TASK-368.

### File structure

**Single file** at `docs/developers/BLE_SERVICES_GUIDE.md`. All sections (Service catalog, Recipe, Conventions, Cross-cutting invariants, Gotchas, Tests, References) live in this one file. Rationale: matches existing convention (`ARCHITECTURE.md`, `TESTING.md`, `CODING_STANDARDS.md`), keeps navigation simple, and the expected total size (~500–1200 lines) is well under the 800-line per-section threshold that would justify splitting.

### Disposition of existing BLE docs

| Doc | Decision | Action |
|---|---|---|
| `BLE_CONFIG_PROTOCOL.md` | **Keep** | Cross-link from the new guide's *Service catalog* and *Conventions* sections for byte-level packet format. Externally cross-linked from firmware and tests; rewriting is risky for no clear win. |
| `BLE_CONFIG_IMPLEMENTATION_NOTES.md` | **Fold** | Content reshape into the new guide's *Conventions* and *Gotchas* sections (mapping below); original deleted as part of TASK-367. |
| `BLE_READBACK_IMPACT.md` | **Keep** | Cross-link from the new guide. It is a frozen-in-time analysis artifact from TASK-353 that gates EPIC-026; absorbing it into a living guide would distort both. |

### Git-mv / reshape plan for `BLE_CONFIG_IMPLEMENTATION_NOTES.md`

Lands in TASK-367. The reshape maps the existing sections onto the new guide:

| Source section (BLE_CONFIG_IMPLEMENTATION_NOTES.md) | Target section in BLE_SERVICES_GUIDE.md |
|---|---|
| Stack choice: NimBLE vs classic ESP32 BLE | Conventions → Stack choice |
| Challenge 1: Registering GATT services at the right moment | Gotchas → ESP32 NimBLE GATT registration timing |
| Challenge 2: Multiple-inheritance return-type conflict | Gotchas → BleKeyboard adapter quirks |
| Challenge 3: BlueZ HID daemon causing disconnects (historical) | Gotchas → BlueZ HID daemon (historical) |
| Challenge 4: Write characteristics require WRITE \| WRITE_NR | Conventions → Write characteristic properties |
| Challenge 5: BlueZ GATT cache stale entries | Gotchas → BlueZ GATT cache invalidation |
| Challenge 6: Missing READY line — serial timing | Gotchas → Serial timing (READY line) |
| Challenge 7: Serial line race — PROFILE vs RESET | Gotchas → Serial line race |
| Challenge 8: Active profile not reported after upload | Gotchas → Active-profile reporting timing |
| Challenge 9: Persistence across soft-reset — profiles not loaded on boot | Gotchas → Persistence across soft-reset |
| Security model | Conventions → Security model |
| Architecture summary | Conventions → Architecture summary |

Because every paragraph in the source has a home in the new guide, the final step in TASK-367 is `git rm docs/developers/BLE_CONFIG_IMPLEMENTATION_NOTES.md` after content extraction (the source contents are reshaped, not copied verbatim, so a plain `git mv` would not be faithful).

### Success criterion (verified in TASK-368)

**The next BLE-touching task references the guide in its description and checks off its recipe items.** Verifies adoption — once the guide exists, the next BLE work post-EPIC-027 should be visibly informed by it. Logged here so TASK-368 has the exact wording to assert against.

### Branch decision

Epic frontmatter rewritten from `feature/ble-services-developer-guide` to `feature/firmware`. The epic work proceeds on the current branch alongside other docs work.

## Acceptance Criteria — verification

- [x] File-structure decision recorded — single file `BLE_SERVICES_GUIDE.md` (above).
- [x] Disposition decision recorded for each of the three existing BLE docs — table above.
- [x] `git rm` plan documented for the folded doc — `BLE_CONFIG_IMPLEMENTATION_NOTES.md` reshape table above; deletion lands in TASK-367.
