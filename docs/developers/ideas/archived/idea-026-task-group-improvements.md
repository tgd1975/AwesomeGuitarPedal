---
id: IDEA-026
title: Task Group Improvements
category: tooling
description: Enhance task group organization and visibility in OVERVIEW.md
---

## Archive Reason

2026-04-28 — Superseded by IDEA-021. The `group:` concept evolved into `epic:` with dedicated EPICS.md and KANBAN.md visualizations, dependency graphs, and per-epic progress bars in OVERVIEW.md. The original "Ungrouped + group index + alphabetical sorting" proposal no longer applies in the epic-based model.

## Details

The current task overview in `OVERVIEW.md` lacks a clear way to see ungrouped tasks and does not provide an index of task groups. This idea proposes improvements to the task group organization and visibility.

### Proposed Changes

1. **Ungrouped Tasks Group**: Add a new task group called "Ungrouped" or "Miscellaneous" that automatically includes all tasks not assigned to any other group. This will make it easier to identify tasks that need to be categorized.

2. **Alphabetical Sorting**: Sort all task groups alphabetically to make it easier to find specific groups.

3. **Group Index**: Add an index at the top of the task groups section that lists all groups along with their open and completed task counts. This will provide a quick overview of the distribution of tasks across groups.

### Example

#### Group Index

| Group Name | Open Tasks | Completed Tasks | Total Tasks |
|-------------|-----------|-----------------|-------------|
| distribution | 2 | 1 | 3 |
| feature_test | 9 | 1 | 10 |
| LongDoublePress | 1 | 7 | 8 |
| Ungrouped | 15 | 59 | 74 |

#### Task Groups

### distribution

| Order | ID | Title | Effort | Complexity | Human-in-loop |
|-------|----|-------|--------|------------|---------------|
| 1 | [TASK-179](open/task-179-determine-android-app-release.md) | Determine how to add the Android app to the release on GitHub | Small (<2h) | Junior | No |
| ? | [TASK-160](open/task-160-publish-android-play-store.md) | Publish app to Google Play Store | Large (8-24h) | Medium | Main ★ |
| ? | [TASK-161](open/task-161-publish-ios-app-store.md) | Publish app to Apple App Store | Large (8-24h) | High | Main ★ |

### feature_test

| Order | ID | Title | Effort | Complexity | Human-in-loop |
|-------|----|-------|--------|------------|---------------|
| ? | [TASK-150](open/task-150-feature-test-cli-scan.md) | Feature Test — CLI scan command | Small (1-2h) | Low | Main ★ |
| ? | [TASK-151](open/task-151-feature-test-cli-upload.md) | Feature Test — CLI upload command | Small (2-4h) | Low | Main ★ |
| ? | [TASK-152](open/task-152-feature-test-cli-upload-config.md) | Feature Test — CLI upload-config command | Small (1-2h) | Low | Main ★ |
| ? | [TASK-153](open/task-153-feature-test-app-home-ble.md) | Feature Test — App home screen & BLE connection | Small (2-4h) | Low | Main ★ |
| ? | [TASK-154](open/task-154-feature-test-app-profiles.md) | Feature Test — App profile list & profile editor | Small (2-4h) | Low | Main ★ |
| ? | [TASK-155](open/task-155-feature-test-app-action-editor.md) | Feature Test — App action editor (all action types) | Small (2-4h) | Low | Main ★ |
| ? | [TASK-156](open/task-156-feature-test-app-upload-preview.md) | Feature Test — App upload screen & JSON preview | Small (2-4h) | Low | Main ★ |
| ? | [TASK-157](open/task-157-feature-test-e2e-integration-edge.md) | Feature Test — E2E musician workflows, integration & edge cases | Medium (4-8h) | Medium | Main ★ |
| ? | [TASK-158](open/task-158-feature-test-ios-build-deploy.md) | Feature Test — Build, deploy and test the iOS app on iPhone | Medium (4-8h) | Medium | Main ★ |

### LongDoublePress

| Order | ID | Title | Effort | Complexity | Human-in-loop |
|-------|----|-------|--------|------------|---------------|
| 1 | ~~[TASK-104](closed/task-104-button-longpress-doublepress-detection.md)~~ | ~~Button Long-Press and Double-Press Detection~~ | ~~Medium (2-8h)~~ | ~~Medium~~ | ~~No~~ |
| 2 | ~~[TASK-105](closed/task-105-eventdispatcher-multievent-api.md)~~ | ~~EventDispatcher Multi-Event API~~ | ~~Small (<2h)~~ | ~~Junior~~ | ~~No~~ |
| 3 | ~~[TASK-106](closed/task-106-config-schema-multievent.md)~~ | ~~Config Schema Extension for Multi-Event Bindings~~ | ~~Small (<2h)~~ | ~~Junior~~ | ~~No~~ |
| 4 | ~~[TASK-107](closed/task-107-mainloop-multievent-wiring.md)~~ | ~~Wire Multi-Event Dispatch in main.cpp~~ | ~~Small (<2h)~~ | ~~Junior~~ | ~~No~~ |
| 5 | ~~[TASK-108](closed/task-108-host-tests-longpress-doublepress.md)~~ | ~~Host Tests for Long Press and Double Press~~ | ~~Medium (2-8h)~~ | ~~Medium~~ | ~~No~~ |
| 6 | [TASK-109](open/task-109-ondevice-multipress-test.md) | On-Device Multi-Press Integration Test (ESP32) | Medium (2-8h) | Medium | Support |
| 7 | ~~[TASK-138](closed/task-138-simulator-longpress-doublepress.md)~~ | ~~Simulator — Long-Press and Double-Press Support~~ | ~~Medium (2-8h)~~ | ~~Medium~~ | ~~No~~ |
| 8 | ~~[TASK-139](closed/task-139-config-builder-longpress-doublepress.md)~~ | ~~Profile Configurator — Long-Press and Double-Press Fields~~ | ~~Medium (2-8h)~~ | ~~Medium~~ | ~~No~~ |

### Ungrouped

| Order | ID | Title | Effort | Complexity | Human-in-loop |
|-------|----|-------|--------|------------|---------------|
| ? | [TASK-033](open/task-033-create-setup-installation-demo-video.md) | Create Setup/Installation Demo Video | Large (8-24h) | Medium | No |
| ? | [TASK-034](open/task-034-create-button-configuration-demo-video.md) | Create Button Configuration Demo Video | Large (8-24h) | Medium | No |
| ? | [TASK-035](open/task-035-create-builder-workflow-demo-video.md) | Create Builder Workflow Demo Video | Large (8-24h) | Medium | No |
| ? | [TASK-036](open/task-036-create-advanced-features-demo-video.md) | Create Advanced Features Demo Video | Extra Large (24-40h) | Senior | No |
| ? | [TASK-037](open/task-037-create-real-world-usage-demo-video.md) | Create Real-World Usage Demo Video | Extra Large (24-40h) | Senior | No |
| ? | [TASK-038](open/task-038-create-troubleshooting-demo-video.md) | Create Troubleshooting Demo Video | Large (8-24h) | Medium | No |
| ? | [TASK-049](open/task-049-setup-video-platform-channel.md) | Setup video platform channel | Small (<2h) | Junior | No |
| ? | [TASK-140](open/task-140-mobile-responsive-tools.md) | Mobile-Responsive Layout for Simulator and Configurator Tools | Large (8-24h) | Medium | No |
| ? | [TASK-148](open/task-148-reorganise-developer-documentation.md) | Reorganise Developer Documentation | Medium (2-8h) | Medium | No |
| ? | [TASK-179](open/task-179-determine-android-app-release.md) | Determine how to add the Android app to the release on GitHub | Small (<2h) | Junior | No |
| ? | [TASK-184](open/task-184-schema-defect-action-value-pin-required.md) | Schema defect — action value/pin fields not required | Small (<2h) | Junior | No |
| ? | [TASK-186](open/task-186-fix-stale-libdeps-cache-local-hardware-libs.md) | Fix stale libdeps cache for local hardware libs in test environments | Small (<2h) | Junior | No |

### Benefits

- **Better Organization**: Tasks are clearly categorized, making it easier to manage and prioritize work.
- **Quick Overview**: The group index provides a snapshot of the task distribution, helping to identify areas that need attention.
- **Easier Navigation**: Alphabetical sorting and clear grouping make it easier to find specific tasks or task groups.
- **Visibility of Ungrouped Tasks**: The Ungrouped section ensures that no tasks are overlooked and encourages categorization.

### Implementation Steps

1. Update the script or tool that generates `OVERVIEW.md` to include an Ungrouped task group.
2. Modify the generation logic to sort task groups alphabetically.
3. Add logic to count open and completed tasks for each group and include this information in a group index table.
4. Test the changes to ensure that the `OVERVIEW.md` file is generated correctly with the new structure.
5. Update any documentation or scripts that reference the task groups to reflect the new organization.
