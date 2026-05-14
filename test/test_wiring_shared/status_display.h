// Wiring-test tool — line-oriented status display.
//
// IDEA-062 explicitly rejected ANSI / dashboard rendering: the tool
// must work on every serial monitor including a stock ``pio device
// monitor``. So "re-rendering" means *reprinting the block*, with a
// blank line in front so the new block is visually separated from
// whatever scrolled past.
//
// TASK-383 prints only the boot banner. TASK-384/385/386 will extend
// this header with the press counters, button state, LED mode, and
// selection highlight.

#pragma once

#include "wiring_config.h"

namespace wiring_test
{

    // Print the one-screen banner: tool name, firmware version, embedded
    // config filename, configured buttons, configured LEDs, and a hint
    // pointing at the (TASK-387) help legend.
    void printBanner(const WiringConfig& cfg, const char* configFilename);

} // namespace wiring_test
