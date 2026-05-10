# Tasks archived in v0.4.1

**25 task(s) closed in this release.**

| ID | Title | Effort | Complexity |
|----|-------|--------|------------|
| [TASK-261](task-261-reorganize-entry-points-per-target-subfolders.md) | Reorganize entry points into per-target subfolders | Small (&lt;2h) | Medium |
| [TASK-280](task-280-defect-action-editor-value-carries-over-on-type-change.md) | Defect — Action Editor value field carries over when Action Type changes | Small (&lt;2h) | Junior |
| [TASK-281](task-281-defect-validation-banner-stale-on-action-edit.md) | Defect — Validation banner stale until profile count changes | XS (&lt;30m) | Junior |
| [TASK-282](task-282-introduce-pedalapp-and-esp32pedalapp.md) | Introduce PedalApp base + Esp32PedalApp, migrate ESP32 #ifdef blocks | Medium (2-8h) | Medium |
| [TASK-283](task-283-ui-driving-skill.md) | Make UI-driving via adb a first-class skill the agent actually uses | Medium (2-8h) | Medium |
| [TASK-284](task-284-housekeep-skill.md) | Add /housekeep skill wrapping scripts/housekeep.py --apply | Small (&lt;2h) | Junior |
| [TASK-285](task-285-commit-skill.md) | Add /commit skill encoding the --no-verify decision protocol | Small (&lt;2h) | Medium |
| [TASK-286](task-286-doc-check-auto-trigger.md) | Promote doc-check from advisory to auto-trigger on .md file moves | Small (&lt;2h) | Junior |
| [TASK-287](task-287-status-skill.md) | Add /status skill bundling branch + last 3 commits + git status --short | XS (&lt;30m) | Junior |
| [TASK-288](task-288-explore-subagent-guidance.md) | Add CLAUDE.md guidance to prefer Explore subagent for multi-step searches | XS (&lt;30m) | Junior |
| [TASK-289](task-289-add-nrf52840pedalapp-retire-shared-main.md) | Add Nrf52840PedalApp, retire shared main.cpp | Small (&lt;2h) | Medium |
| [TASK-290](task-290-direnv-project-envvars.md) | Project-level env-var setup (direnv or equivalent) to kill source/cd churn | Small (&lt;2h) | Junior |
| [TASK-291](task-291-ble-reset-skill.md) | Add /ble-reset skill encapsulating the flaky-pairing recovery dance | Small (&lt;2h) | Junior |
| [TASK-292](task-292-extract-blepedalapp-shared-layer.md) | Extract BlePedalApp shared layer (Phase 2) | Medium (2-8h) | Medium |
| [TASK-293](task-293-hostpedalapp-fake-eliminate-guards-logger-timing.md) | Phase 3a — HostPedalApp fake; eliminate HOST_TEST_BUILD from logger / timing | Medium (2-8h) | Medium |
| [TASK-294](task-294-eliminate-host-test-build-from-actions.md) | Phase 3b — eliminate HOST_TEST_BUILD from action implementations | Small (&lt;2h) | Medium |
| [TASK-295](task-295-ifilesystem-di-finish-phase-3.md) | Phase 3c — IFileSystem DI; eliminate HOST_TEST_BUILD from littlefs (finish Phase 3) | Medium (2-8h) | Senior |
| [TASK-296](task-296-collapse-lib-hardware-esp32-into-src.md) | Phase 4a — collapse lib/hardware/esp32 into src/esp32 | Small (&lt;2h) | Medium |
| [TASK-297](task-297-collapse-lib-hardware-nrf52840-into-src.md) | Phase 4b — collapse lib/hardware/nrf52840 into src/nrf52840; delete lib/hardware/ | Small (&lt;2h) | Medium |
| [TASK-298](task-298-mirror-include-src-layout-under-target.md) | Phase 4c — mirror include/ + src/ layout under each target | XS (&lt;30m) | Junior |
| [TASK-299](task-299-consolidate-host-platform-under-src.md) | Phase 4d — consolidate host platform implementations under src/host/ | XS (&lt;30m) | Junior |
| [TASK-300](task-300-host-pedal-app-inherits-blepedalapp.md) | Phase 4e — HostPedalApp inherits BlePedalApp; accepts injected dependencies | Small (&lt;2h) | Medium |
| [TASK-301](task-301-per-pin-fake-gpio.md) | Phase 4f — per-pin fake_gpio for multi-button host tests | Small (&lt;2h) | Medium |
| [TASK-302](task-302-end-to-end-app-integration-tests.md) | Phase 4g — end-to-end PedalApp integration tests on host | Small (&lt;2h) | Medium |
| [TASK-303](task-303-phase-5-docs-overhaul.md) | Phase 5 — docs overhaul to match the post-EPIC-020 architecture | Medium (2-8h) | Medium |
