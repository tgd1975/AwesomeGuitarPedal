// Wiring-test tool — line-oriented status display.
//
// IDEA-062 explicitly rejected ANSI / dashboard rendering: the tool
// must work on every serial monitor including a stock ``pio device
// monitor``. So "re-rendering" means *reprinting the block*, with a
// blank line in front so the new block is visually separated from
// whatever scrolled past.
//
// TASK-384 extends the block to include per-button press counters.
// TASK-385/386 will add LED mode and selection highlight.

#pragma once

#include "button_tracker.h"
#include "wiring_config.h"

namespace wiring_test
{

    // Print the one-screen status block. ``states`` may be nullptr
    // (TASK-383 boot banner before the tracker initialises) — in
    // which case all press counters render as 0.
    void printStatusBlock(const WiringConfig& cfg,
                          const char* configFilename,
                          const ButtonState* states);

} // namespace wiring_test
