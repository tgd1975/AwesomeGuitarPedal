---
id: IDEA-046
title: Investigate the ble_config_service.cpp copy loop — possible build-system smell
category: firmware
status: open
opened: 2026-04-28
---

In chat session `7f6bff0f-287d-4a16-b1fa-2bef94b998a5` the agent ran
`cp /home/tobias/Dokumente/Projekte/AwesomeStudioPedal/lib/hardware/esp32/src/ble_config_service.cpp \ …`
**16 times** alongside 7 invocations of
`python3 test/test_ble_config_esp32/runner.py --port /dev/ttyUSB0`.

This pattern — repeatedly copying a single source file between locations
in tight rotation with on-device test runs — strongly suggests one of:

- The on-device test build is not picking up edits to the canonical
  source automatically (missing include path, wrong source root in
  `platformio.ini`).
- The agent misunderstood where the test harness reads the source from
  and is keeping two copies in sync by hand.
- A symlink that should exist is a real file (or vice versa).
- The test harness has its own private copy under `test/` that drifted.

The fix could be small (one `platformio.ini` line, one symlink) or
larger (restructure the test build). It is not safe to scaffold a task
until the root cause is understood.

## Open questions

- What were the source and destination paths of the `cp`? (Need to read
  the session.)
- Did the test runner output change after the copy, or was the copy a
  workaround for something else?
- Does the issue still reproduce on the current branch, or has the HAL
  refactor (EPIC-020) already addressed it?

## Next step

Read the session transcript at
`~/.claude/projects/-home-tobias-Dokumente-Projekte-AwesomeStudioPedal/7f6bff0f-287d-4a16-b1fa-2bef94b998a5.jsonl`
and report the actual paths and the test outcome. Promote to a task only
after the root cause is identified.

## Notes

- Source signal: 16× `cp ble_config_service.cpp` in one session.
- Filed as an idea per the "uncertainty → idea" rule
  (memory: `feedback_idea_vs_task.md`) — root cause is unknown, so
  acceptance criteria cannot yet be written.

## Update 2026-04-29 — likely already addressed by the HAL refactor

A scan of the current tree suggests the underlying smell may already be
gone:

- The path the agent was copying *from* — `lib/hardware/esp32/src/ble_config_service.cpp` —
  **no longer exists**. The HAL refactor (IDEA-028, archived) consolidated
  ESP32 sources under [src/esp32/](../../../../src/esp32/). The file now
  lives at [src/esp32/src/ble_config_service.cpp](../../../../src/esp32/src/ble_config_service.cpp)
  with its header at [src/esp32/include/ble_config_service.h](../../../../src/esp32/include/ble_config_service.h).
- [platformio.ini:233](../../../../platformio.ini#L233) builds the BLE-config
  test firmware with `build_src_filter = -<*> +<../test/test_ble_config_esp32/test_main.cpp> +<esp32/> -<esp32/main.cpp> +<pedal_app.cpp> +<ble_pedal_app.cpp>`,
  i.e. it picks up the canonical source tree directly. There is no
  separate copy under `test/` for the runner to fall behind on.
- This matches the third open question above ("has the HAL refactor
  already addressed it?") — the answer appears to be yes.

**Investigation still worth doing** because:

- The session transcript at
  `~/.claude/projects/-home-tobias-Dokumente-Projekte-AwesomeStudioPedal/7f6bff0f-287d-4a16-b1fa-2bef94b998a5.jsonl`
  may reveal a *different* smell (e.g. the agent copying *into* a stale
  location that we didn't catch) that the file-tree scan can't see.
- Worst case: the transcript confirms the HAL refactor fixed it, and we
  archive this idea with a pointer to IDEA-028. Cheap.
- Best case: we find a residual issue (stale symlink, drifted helper
  file, runner workaround) and fix it before it bites another session.

Proceeding with the original "next step" — read the transcript, report
paths and outcome — even though the prior cause looks dissolved.
