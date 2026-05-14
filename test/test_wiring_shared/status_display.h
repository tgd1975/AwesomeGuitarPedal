// Wiring-test tool — line-oriented status display.
//
// IDEA-062 explicitly rejected ANSI / dashboard rendering: the tool
// must work on every serial monitor including a stock ``pio device
// monitor``. So "re-rendering" means *reprinting the block*, with a
// blank line in front so the new block is visually separated from
// whatever scrolled past.
//
// TASK-384 added per-button press counters; TASK-385 added the
// active group-mode label. TASK-386 will add the individual-mode
// selection highlight.

#pragma once

#include "button_tracker.h"
#include "led_mode.h"
#include "wiring_config.h"
#include <stdint.h>

namespace wiring_test
{

    // Print the one-screen status block. ``states`` and ``runtime``
    // may both be nullptr (boot before init) — counters render as 0
    // and the mode label renders as "off (uninit)".
    void printStatusBlock(const WiringConfig& cfg,
                          const char* configFilename,
                          const ButtonState* states,
                          const LedRuntime* runtime,
                          uint32_t nowMs);

} // namespace wiring_test
