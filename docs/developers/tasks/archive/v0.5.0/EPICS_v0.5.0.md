# Epics

**Overall:** 🔵 **active** — ░░░░░░░░░░ 0/26 (0%) across 8 groups — 19 open · 0 active · 7 paused · 0 closed

## Index

| Epic | Title | Status | Open | Active | Paused | Closed | Done |
|------|-------|--------|-----:|-------:|-------:|-------:|------|
| [EPIC-012](#epic-012-app-store-distribution) | App store distribution | ⚪ _open_ | 1 | 0 | 0 | 0 | ░░░░░░░░░░ 0% |
| [EPIC-014](#epic-014-end-to-end-feature-tests) | End-to-end feature tests | 🔵 **active** | 1 | 0 | 1 | 0 | ░░░░░░░░░░ 0% |
| [EPIC-017](#epic-017-video-content-and-channel) | Video content and channel | ⚪ _open_ | 7 | 0 | 0 | 0 | ░░░░░░░░░░ 0% |
| [EPIC-019](#epic-019-iphone-app--build-test-and-ship) | iPhone app — build, test and ship | 🔵 **active** | 0 | 0 | 2 | 0 | ░░░░░░░░░░ 0% |
| [EPIC-025](#epic-025-nrf52840-hardware-blocked-work) | nRF52840 hardware-blocked work | 🔵 **active** | 0 | 0 | 4 | 0 | ░░░░░░░░░░ 0% |
| [EPIC-026](#epic-026-pedal-details-app-pages--firmware-readback-surfaces) | Pedal-details app pages — firmware readback surfaces | ⚪ _open_ | 3 | 0 | 0 | 0 | ░░░░░░░░░░ 0% |
| [EPIC-027](#epic-027-ble-services-developer-guide) | BLE services developer guide | ⚪ _open_ | 4 | 0 | 0 | 0 | ░░░░░░░░░░ 0% |
| [—](#unassigned) | _(no epic)_ | ⚪ _open_ | 3 | 0 | 0 | 0 | ░░░░░░░░░░ 0% |

---

## EPIC-012: App store distribution

[↑ back to top](#index)

**Status:** ⚪ _open_ — ░░░░░░░░░░ 0/1 (0%)

```mermaid
graph LR
    TASK_160["TASK-160"]:::open
    click TASK_160 "open/task-160-publish-android-play-store.md"
    classDef open    fill:#FAFAFA,stroke:#555,stroke-width:8px,color:#000
    classDef active  fill:#FAFAFA,stroke:#1A6FA8,stroke-width:8px,color:#000
    classDef closed  fill:#FAFAFA,stroke:#3F8B53,stroke-width:8px,color:#000
    classDef paused  fill:#FAFAFA,stroke:#B07810,stroke-width:8px,color:#000
    classDef openExt    fill:#000,stroke:#888,stroke-width:8px,color:#FFF
    classDef activeExt  fill:#000,stroke:#3FA9F5,stroke-width:8px,color:#FFF
    classDef closedExt  fill:#000,stroke:#7CC68A,stroke-width:8px,color:#FFF
    classDef pausedExt  fill:#000,stroke:#F0B030,stroke-width:8px,color:#FFF
```

| Order | ID | Title | Status | Effort |
|-------|----|-------|--------|--------|
| 2 | [TASK-160](open/task-160-publish-android-play-store.md) | Publish app to Google Play Store | ⚪ _open_ | Large (8-24h) |

## EPIC-014: End-to-end feature tests

[↑ back to top](#index)

**Status:** 🔵 **active** — ░░░░░░░░░░ 0/2 (0%)

```mermaid
graph TD
    TASK_248["TASK-248"]:::open
    TASK_226["TASK-226"]:::paused
    TASK_150["TASK-150"]:::openExt
    TASK_247["TASK-247"]:::openExt
    TASK_247 --> TASK_248
    TASK_150 --> TASK_226
    click TASK_248 "open/task-248-ble-pairing-test-windows-fallback.md"
    click TASK_226 "open/task-226-feature-test-cli-scan-two-pedals.md"
    classDef open    fill:#FAFAFA,stroke:#555,stroke-width:8px,color:#000
    classDef active  fill:#FAFAFA,stroke:#1A6FA8,stroke-width:8px,color:#000
    classDef closed  fill:#FAFAFA,stroke:#3F8B53,stroke-width:8px,color:#000
    classDef paused  fill:#FAFAFA,stroke:#B07810,stroke-width:8px,color:#000
    classDef openExt    fill:#000,stroke:#888,stroke-width:8px,color:#FFF
    classDef activeExt  fill:#000,stroke:#3FA9F5,stroke-width:8px,color:#FFF
    classDef closedExt  fill:#000,stroke:#7CC68A,stroke-width:8px,color:#FFF
    classDef pausedExt  fill:#000,stroke:#F0B030,stroke-width:8px,color:#FFF
```

| Order | ID | Title | Status | Effort |
|-------|----|-------|--------|--------|
| 2 | [TASK-248](open/task-248-ble-pairing-test-windows-fallback.md) | BLE pairing test — Windows manual fallback (and macOS if a host appears) | ⚪ _open_ | Small (&lt;2h) |
| 1 | [TASK-226](paused/task-226-feature-test-cli-scan-two-pedals.md) | Feature Test — CLI scan with two pedals (S-04) | 🟡 **paused** | Small (&lt;2h) |

## EPIC-017: Video content and channel

[↑ back to top](#index)

**Status:** ⚪ _open_ — ░░░░░░░░░░ 0/7 (0%)

```mermaid
graph LR
    TASK_033["TASK-033"]:::open
    TASK_034["TASK-034"]:::open
    TASK_035["TASK-035"]:::open
    TASK_036["TASK-036"]:::open
    TASK_037["TASK-037"]:::open
    TASK_038["TASK-038"]:::open
    TASK_049["TASK-049"]:::open
    TASK_033 ~~~ TASK_034 ~~~ TASK_035 ~~~ TASK_036 ~~~ TASK_037 ~~~ TASK_038 ~~~ TASK_049
    click TASK_033 "open/task-033-create-setup-installation-demo-video.md"
    click TASK_034 "open/task-034-create-button-configuration-demo-video.md"
    click TASK_035 "open/task-035-create-builder-workflow-demo-video.md"
    click TASK_036 "open/task-036-create-advanced-features-demo-video.md"
    click TASK_037 "open/task-037-create-real-world-usage-demo-video.md"
    click TASK_038 "open/task-038-create-troubleshooting-demo-video.md"
    click TASK_049 "open/task-049-setup-video-platform-channel.md"
    classDef open    fill:#FAFAFA,stroke:#555,stroke-width:8px,color:#000
    classDef active  fill:#FAFAFA,stroke:#1A6FA8,stroke-width:8px,color:#000
    classDef closed  fill:#FAFAFA,stroke:#3F8B53,stroke-width:8px,color:#000
    classDef paused  fill:#FAFAFA,stroke:#B07810,stroke-width:8px,color:#000
    classDef openExt    fill:#000,stroke:#888,stroke-width:8px,color:#FFF
    classDef activeExt  fill:#000,stroke:#3FA9F5,stroke-width:8px,color:#FFF
    classDef closedExt  fill:#000,stroke:#7CC68A,stroke-width:8px,color:#FFF
    classDef pausedExt  fill:#000,stroke:#F0B030,stroke-width:8px,color:#FFF
```

| Order | ID | Title | Status | Effort |
|-------|----|-------|--------|--------|
| 1 | [TASK-033](open/task-033-create-setup-installation-demo-video.md) | Create Setup/Installation Demo Video | ⚪ _open_ | Large (8-24h) |
| 2 | [TASK-034](open/task-034-create-button-configuration-demo-video.md) | Create Button Configuration Demo Video | ⚪ _open_ | Large (8-24h) |
| 3 | [TASK-035](open/task-035-create-builder-workflow-demo-video.md) | Create Builder Workflow Demo Video | ⚪ _open_ | Large (8-24h) |
| 4 | [TASK-036](open/task-036-create-advanced-features-demo-video.md) | Create Advanced Features Demo Video | ⚪ _open_ | Extra Large (24-40h) |
| 5 | [TASK-037](open/task-037-create-real-world-usage-demo-video.md) | Create Real-World Usage Demo Video | ⚪ _open_ | Extra Large (24-40h) |
| 6 | [TASK-038](open/task-038-create-troubleshooting-demo-video.md) | Create Troubleshooting Demo Video | ⚪ _open_ | Large (8-24h) |
| 7 | [TASK-049](open/task-049-setup-video-platform-channel.md) | Setup video platform channel | ⚪ _open_ | Small (&lt;2h) |

## EPIC-019: iPhone app — build, test and ship

[↑ back to top](#index)

**Status:** 🔵 **active** — ░░░░░░░░░░ 0/2 (0%)

```mermaid
graph LR
    TASK_158["TASK-158"]:::paused
    TASK_161["TASK-161"]:::paused
    TASK_158 ~~~ TASK_161
    click TASK_158 "open/task-158-feature-test-ios-build-deploy.md"
    click TASK_161 "open/task-161-publish-ios-app-store.md"
    classDef open    fill:#FAFAFA,stroke:#555,stroke-width:8px,color:#000
    classDef active  fill:#FAFAFA,stroke:#1A6FA8,stroke-width:8px,color:#000
    classDef closed  fill:#FAFAFA,stroke:#3F8B53,stroke-width:8px,color:#000
    classDef paused  fill:#FAFAFA,stroke:#B07810,stroke-width:8px,color:#000
    classDef openExt    fill:#000,stroke:#888,stroke-width:8px,color:#FFF
    classDef activeExt  fill:#000,stroke:#3FA9F5,stroke-width:8px,color:#FFF
    classDef closedExt  fill:#000,stroke:#7CC68A,stroke-width:8px,color:#FFF
    classDef pausedExt  fill:#000,stroke:#F0B030,stroke-width:8px,color:#FFF
```

| Order | ID | Title | Status | Effort |
|-------|----|-------|--------|--------|
| 1 | [TASK-158](paused/task-158-feature-test-ios-build-deploy.md) | Feature Test — Build, deploy and test the iOS app on iPhone | 🟡 **paused** | Medium (4-8h) |
| 2 | [TASK-161](paused/task-161-publish-ios-app-store.md) | Publish app to Apple App Store | 🟡 **paused** | Large (8-24h) |

## EPIC-025: nRF52840 hardware-blocked work

[↑ back to top](#index)

**Status:** 🔵 **active** — ░░░░░░░░░░ 0/4 (0%)

```mermaid
graph LR
    TASK_249["TASK-249"]:::paused
    TASK_358["TASK-358"]:::paused
    TASK_359["TASK-359"]:::paused
    TASK_360["TASK-360"]:::paused
    TASK_246["TASK-246"]:::openExt
    TASK_353["TASK-353"]:::openExt
    TASK_354["TASK-354"]:::openExt
    TASK_355["TASK-355"]:::openExt
    TASK_356["TASK-356"]:::openExt
    TASK_246 --> TASK_249
    TASK_353 --> TASK_358
    TASK_354 --> TASK_358
    TASK_355 --> TASK_358
    TASK_356 --> TASK_358
    TASK_359 --> TASK_358
    TASK_358 --> TASK_360
    TASK_359 --> TASK_360
    click TASK_249 "open/task-249-nrf52840-pairing-pin-unwired.md"
    click TASK_358 "open/task-358-nrf52840-ble-readback-surfaces.md"
    click TASK_359 "open/task-359-remove-nrf5-task-routing-skill.md"
    click TASK_360 "open/task-360-nrf52840-esp32-parity-audit.md"
    click TASK_354 "open/task-354-firmware-version-read-characteristic.md"
    click TASK_355 "open/task-355-firmware-config-readback.md"
    click TASK_356 "open/task-356-firmware-active-profile-notify.md"
    classDef open    fill:#FAFAFA,stroke:#555,stroke-width:8px,color:#000
    classDef active  fill:#FAFAFA,stroke:#1A6FA8,stroke-width:8px,color:#000
    classDef closed  fill:#FAFAFA,stroke:#3F8B53,stroke-width:8px,color:#000
    classDef paused  fill:#FAFAFA,stroke:#B07810,stroke-width:8px,color:#000
    classDef openExt    fill:#000,stroke:#888,stroke-width:8px,color:#FFF
    classDef activeExt  fill:#000,stroke:#3FA9F5,stroke-width:8px,color:#FFF
    classDef closedExt  fill:#000,stroke:#7CC68A,stroke-width:8px,color:#FFF
    classDef pausedExt  fill:#000,stroke:#F0B030,stroke-width:8px,color:#FFF
```

| Order | ID | Title | Status | Effort |
|-------|----|-------|--------|--------|
| 1 | [TASK-359](paused/task-359-remove-nrf5-task-routing-skill.md) | Remove nrf5-task-routing skill once nRF52840 hardware is available | 🟡 **paused** | XS (&lt;30m) |
| 2 | [TASK-358](paused/task-358-nrf52840-ble-readback-surfaces.md) | nRF52840 BLE readback surfaces (firmware-version DIS + config readback + active-profile notify) | 🟡 **paused** | Large (8-24h) |
| 3 | [TASK-360](paused/task-360-nrf52840-esp32-parity-audit.md) | nRF52840 — verify functional parity with ESP32 across the codebase | 🟡 **paused** | Medium (2-8h) |
| 4 | [TASK-249](paused/task-249-nrf52840-pairing-pin-unwired.md) | nRF52840 pairing_pin is entirely unwired (security parity with ESP32) | 🟡 **paused** | Medium (2-8h) |

## EPIC-026: Pedal-details app pages — firmware readback surfaces

[↑ back to top](#index)

**Status:** ⚪ _open_ — ░░░░░░░░░░ 0/3 (0%)

```mermaid
graph TD
    TASK_354["TASK-354"]:::open
    TASK_355["TASK-355"]:::open
    TASK_356["TASK-356"]:::open
    TASK_353["TASK-353"]:::openExt
    TASK_353 --> TASK_354
    TASK_353 --> TASK_355
    TASK_353 --> TASK_356
    click TASK_354 "open/task-354-firmware-version-read-characteristic.md"
    click TASK_355 "open/task-355-firmware-config-readback.md"
    click TASK_356 "open/task-356-firmware-active-profile-notify.md"
    classDef open    fill:#FAFAFA,stroke:#555,stroke-width:8px,color:#000
    classDef active  fill:#FAFAFA,stroke:#1A6FA8,stroke-width:8px,color:#000
    classDef closed  fill:#FAFAFA,stroke:#3F8B53,stroke-width:8px,color:#000
    classDef paused  fill:#FAFAFA,stroke:#B07810,stroke-width:8px,color:#000
    classDef openExt    fill:#000,stroke:#888,stroke-width:8px,color:#FFF
    classDef activeExt  fill:#000,stroke:#3FA9F5,stroke-width:8px,color:#FFF
    classDef closedExt  fill:#000,stroke:#7CC68A,stroke-width:8px,color:#FFF
    classDef pausedExt  fill:#000,stroke:#F0B030,stroke-width:8px,color:#FFF
```

| Order | ID | Title | Status | Effort |
|-------|----|-------|--------|--------|
| 1 | [TASK-354](open/task-354-firmware-version-read-characteristic.md) | Firmware — expose firmware-version read characteristic (+ DIS 0x180A decision) | ⚪ _open_ | Small (&lt;2h) |
| 2 | [TASK-355](open/task-355-firmware-config-readback.md) | Firmware — config readback (option chosen in TASK-353) | ⚪ _open_ | Medium (2-8h) |
| 3 | [TASK-356](open/task-356-firmware-active-profile-notify.md) | Firmware — active-profile-index notify characteristic | ⚪ _open_ | Small (&lt;2h) |

## EPIC-027: BLE services developer guide

[↑ back to top](#index)

**Status:** ⚪ _open_ — ░░░░░░░░░░ 0/4 (0%)

```mermaid
graph LR
    TASK_365["TASK-365"]:::open
    TASK_366["TASK-366"]:::open
    TASK_367["TASK-367"]:::open
    TASK_368["TASK-368"]:::open
    TASK_365 --> TASK_366
    TASK_365 --> TASK_367
    TASK_366 --> TASK_368
    TASK_367 --> TASK_368
    click TASK_365 "open/task-365-scope-ble-services-guide.md"
    click TASK_366 "open/task-366-draft-catalog-and-recipe-sections.md"
    click TASK_367 "open/task-367-draft-conventions-invariants-gotchas.md"
    click TASK_368 "open/task-368-draft-tests-references-and-verify.md"
    classDef open    fill:#FAFAFA,stroke:#555,stroke-width:8px,color:#000
    classDef active  fill:#FAFAFA,stroke:#1A6FA8,stroke-width:8px,color:#000
    classDef closed  fill:#FAFAFA,stroke:#3F8B53,stroke-width:8px,color:#000
    classDef paused  fill:#FAFAFA,stroke:#B07810,stroke-width:8px,color:#000
    classDef openExt    fill:#000,stroke:#888,stroke-width:8px,color:#FFF
    classDef activeExt  fill:#000,stroke:#3FA9F5,stroke-width:8px,color:#FFF
    classDef closedExt  fill:#000,stroke:#7CC68A,stroke-width:8px,color:#FFF
    classDef pausedExt  fill:#000,stroke:#F0B030,stroke-width:8px,color:#FFF
```

| Order | ID | Title | Status | Effort |
|-------|----|-------|--------|--------|
| 1 | [TASK-365](open/task-365-scope-ble-services-guide.md) | Scope the BLE services developer guide — file structure and existing-doc disposition | ⚪ _open_ | Small (&lt;2h) |
| 2 | [TASK-366](open/task-366-draft-catalog-and-recipe-sections.md) | Draft Service catalog + Recipe sections of the BLE services guide (with TASK-354 worked example) | ⚪ _open_ | Medium (2-8h) |
| 3 | [TASK-367](open/task-367-draft-conventions-invariants-gotchas.md) | Draft Conventions, Cross-cutting invariants, and Gotchas sections of the BLE services guide | ⚪ _open_ | Medium (2-8h) |
| 4 | [TASK-368](open/task-368-draft-tests-references-and-verify.md) | Draft Tests + References sections of the BLE services guide; verify success criterion; close epic | ⚪ _open_ | Small (&lt;2h) |

## Unassigned

[↑ back to top](#index)

**Status:** ⚪ _open_ — ░░░░░░░░░░ 0/3 (0%)

```mermaid
graph TD
    TASK_148["TASK-148"]:::open
    TASK_352["TASK-352"]:::open
    TASK_370["TASK-370"]:::open
    TASK_147["TASK-147"]:::openExt
    TASK_363["TASK-363"]:::openExt
    TASK_147 --> TASK_148
    TASK_363 --> TASK_370
    click TASK_148 "open/task-148-reorganise-developer-documentation.md"
    click TASK_352 "open/task-352-investigate-pre-commit-hook-latency.md"
    click TASK_370 "open/task-370-make-schemdraw-svgs-deterministic.md"
    classDef open    fill:#FAFAFA,stroke:#555,stroke-width:8px,color:#000
    classDef active  fill:#FAFAFA,stroke:#1A6FA8,stroke-width:8px,color:#000
    classDef closed  fill:#FAFAFA,stroke:#3F8B53,stroke-width:8px,color:#000
    classDef paused  fill:#FAFAFA,stroke:#B07810,stroke-width:8px,color:#000
    classDef openExt    fill:#000,stroke:#888,stroke-width:8px,color:#FFF
    classDef activeExt  fill:#000,stroke:#3FA9F5,stroke-width:8px,color:#FFF
    classDef closedExt  fill:#000,stroke:#7CC68A,stroke-width:8px,color:#FFF
    classDef pausedExt  fill:#000,stroke:#F0B030,stroke-width:8px,color:#FFF
```

| Order | ID | Title | Status | Effort |
|-------|----|-------|--------|--------|
| ? | [TASK-148](open/task-148-reorganise-developer-documentation.md) | Reorganise Developer Documentation | ⚪ _open_ | Medium (2-8h) |
| ? | [TASK-352](open/task-352-investigate-pre-commit-hook-latency.md) | Investigate pre-commit hook latency — reorganize, parallelize, or skip irrelevant checks | ⚪ _open_ | Medium (2-8h) |
| ? | [TASK-370](open/task-370-make-schemdraw-svgs-deterministic.md) | Make Schemdraw-generated SVGs deterministic so the Docs CI staleness guard can pass | ⚪ _open_ | Small (&lt;2h) |
