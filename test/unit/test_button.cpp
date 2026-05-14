#include "arduino_shim.h"
#include "button.h"
#include <gtest/gtest.h>

class ButtonTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        fake_time::value = 0;
        fake_gpio::pin_state = LOW; // default: button pressed (active-low)
    }
};

// Helper: simulate a press ISR (pin LOW)
static void press(Button& btn)
{
    fake_gpio::pin_state = LOW;
    btn.isr();
}

// Helper: simulate a release ISR (pin HIGH)
static void release(Button& btn)
{
    fake_gpio::pin_state = HIGH;
    btn.isr();
}

TEST_F(ButtonTest, EventReturnsFalseInitially)
{
    Button btn(5);
    EXPECT_FALSE(btn.event());
}

TEST_F(ButtonTest, PressIsAccepted)
{
    Button btn(5);
    fake_time::value = 200; // 200ms past lastDebounceTime=0, well over 100ms debounce
    press(btn);
    fake_time::value = 550; // past 300ms double-press window so event() may fire
    EXPECT_TRUE(btn.event());
}

TEST_F(ButtonTest, EventClearsAfterSingleConsumption)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    fake_time::value = 550;
    EXPECT_TRUE(btn.event());
    EXPECT_FALSE(btn.event());
}

TEST_F(ButtonTest, BounceDuringPressSideIsIgnored)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn); // accepted
    fake_time::value = 205;
    press(btn); // bounce — blocked by awaitingRelease, not debounce
    EXPECT_EQ(btn.pressCount, 1);
}

TEST_F(ButtonTest, SecondPressWithinDebounceWindowAfterReleaseIsIgnored)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn); // accepted, lastDebounceTime=200
    btn.event();
    fake_time::value = 310;
    release(btn); // 110ms after press — debounce elapsed, lastDebounceTime=310
    fake_time::value = 380;
    press(btn); // 70ms after release — within 100ms debounce window → rejected
    EXPECT_FALSE(btn.event());
}

TEST_F(ButtonTest, SecondPressAfterDebounceWindowIsAccepted)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn); // accepted, lastDebounceTime=200
    btn.event();
    fake_time::value = 310;
    release(btn); // 110ms after press — debounce elapsed, lastDebounceTime=310
    // Second press at t=420: 220ms after first press, within 300ms double-press window.
    // This triggers a double-press: event() is suppressed, doublePressEvent() fires.
    fake_time::value = 420;
    press(btn); // debounce elapsed (110ms > 100ms) → accepted but suppressed as double press
    EXPECT_FALSE(btn.event());           // suppressed — this is a double press
    EXPECT_TRUE(btn.doublePressEvent()); // double press confirmed
}

TEST_F(ButtonTest, SecondPressOutsideDoublePressWindowAcceptedAsSinglePress)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);             // first press at t=200
    fake_time::value = 550; // past double-press window — first press may now be consumed
    btn.event();            // consume
    fake_time::value = 600;
    release(btn);
    fake_time::value = 900;  // 700ms after first press — outside 300ms double-press window
    press(btn);              // second press at t=900, not a double press
    fake_time::value = 1250; // past double-press window for the second press
    EXPECT_TRUE(btn.event());
    EXPECT_FALSE(btn.doublePressEvent());
}

// THE CRITICAL CASE: hold > debounceDelay, then release with bounce
TEST_F(ButtonTest, ReleaseBounceAfterLongHoldDoesNotFireSpuriousEvent)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn); // accepted at t=200
    EXPECT_EQ(btn.pressCount, 1);

    // hold for 400ms (well over 100ms debounce delay)
    fake_time::value = 600;
    release(btn); // HIGH edge: awaitingRelease=false, lastDebounceTime=600

    // contact bounce on LOW during release — must be rejected
    fake_time::value = 660;
    press(btn);                   // 60ms after release — within 100ms window → rejected
    EXPECT_EQ(btn.pressCount, 1); // still only 1, no spurious event
}

TEST_F(ButtonTest, ReleaseEdgeAloneDoesNotFireEvent)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    btn.event(); // consume the press
    fake_time::value = 300;
    release(btn); // release should produce no event
    EXPECT_FALSE(btn.event());
}

// Both HIGH and LOW edges require debounce. Alternating bounce after release:
// the first HIGH is accepted (lastDebounceTime=300), subsequent HIGH bounces within
// 100ms are rejected. LOW bounces within 100ms of the accepted HIGH are also rejected.
TEST_F(ButtonTest, AlternatingBouncesAfterReleaseDoNotFireSpuriousEvent)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    fake_time::value = 550; // past double-press window so event() can consume
    btn.event();            // consume, pressCount=0

    // genuine release 350ms after press — debounce elapsed
    fake_time::value = 600;
    release(btn); // HIGH accepted: lastDebounceTime=600
    fake_time::value = 610;
    press(btn); // LOW bounce: 10ms < 100 → rejected
    fake_time::value = 620;
    release(btn); // HIGH bounce: 20ms < 100 → rejected (debounce on HIGH too)
    fake_time::value = 630;
    press(btn); // LOW bounce: 30ms < 100 → rejected
    EXPECT_EQ(btn.pressCount, 0);

    // 101ms after the accepted HIGH (t=600) — genuine new press
    // Also >300ms after lastPressTime_=200, so not a double press.
    fake_time::value = 701;
    press(btn); // LOW: 701-600=101ms > 100 → accepted
    EXPECT_EQ(btn.pressCount, 1);
}

// A noise spike HIGH during a held press must not prematurely clear awaitingRelease.
// Without debouncing the HIGH edge, the LOW bounce after the spike would be rejected
// and the button would appear unresponsive.
TEST_F(ButtonTest, NoiseHighDuringHoldDoesNotClearAwaitingRelease)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn); // accepted, awaitingRelease=true, lastDebounceTime=200
    EXPECT_EQ(btn.pressCount, 1);

    // noise spike HIGH at t=210 — within 100ms debounce of press → rejected
    fake_time::value = 210;
    release(btn); // awaitingRelease=true but isDebounced(210): 10ms < 100 → ignored
    EXPECT_TRUE(btn.awaitingRelease); // still waiting for real release

    // LOW bounce at t=215 — awaitingRelease still true, so ignored
    fake_time::value = 215;
    press(btn);
    EXPECT_EQ(btn.pressCount, 1); // no double count

    // genuine release 200ms after press
    fake_time::value = 400;
    release(btn); // 400-200=200ms > 100 → accepted
    EXPECT_FALSE(btn.awaitingRelease);
}

// Race: ISR fires on falling edge but digitalRead sees HIGH (pin bounced back before read).
// awaitingRelease=false, so HIGH is ignored entirely — no timer update.
TEST_F(ButtonTest, SpuriousHighEdgeBeforePressDoesNotBlockEvent)
{
    Button btn(5);
    fake_time::value = 200;
    release(btn); // HIGH with awaitingRelease=false → ignored, no timer update
    fake_time::value = 210;
    press(btn); // LOW 10ms later — lastDebounceTime still 0, so 210ms > 100 → accepted
    EXPECT_EQ(btn.pressCount, 1);
}

TEST_F(ButtonTest, ResetClearsPendingEventsAndDebounceTimer)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    btn.reset();
    EXPECT_FALSE(btn.event());
    EXPECT_EQ(btn.lastDebounceTime, 0UL);
}

// ---------------------------------------------------------------------------
// releaseEvent()
// ---------------------------------------------------------------------------

TEST_F(ButtonTest, ReleaseEvent_ReturnsFalseInitially)
{
    Button btn(5);
    EXPECT_FALSE(btn.releaseEvent());
}

TEST_F(ButtonTest, ReleaseEvent_ReturnsTrueAfterPressAndRelease)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    fake_time::value = 310;
    release(btn); // accepted release edge
    EXPECT_TRUE(btn.releaseEvent());
}

TEST_F(ButtonTest, ReleaseEvent_ClearsAfterSingleConsumption)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    fake_time::value = 310;
    release(btn);
    EXPECT_TRUE(btn.releaseEvent());
    EXPECT_FALSE(btn.releaseEvent()); // consumed
}

TEST_F(ButtonTest, ReleaseEvent_NotSetByBouncedRelease)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    fake_time::value = 260; // 60ms < 100ms debounce
    release(btn);           // rejected
    EXPECT_FALSE(btn.releaseEvent());
}

TEST_F(ButtonTest, ReleaseEvent_NotSetWithoutPriorPress)
{
    Button btn(5);
    fake_time::value = 200;
    release(btn); // awaitingRelease=false → ignored
    EXPECT_FALSE(btn.releaseEvent());
}

TEST_F(ButtonTest, ResetClearsReleaseEvent)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    fake_time::value = 310;
    release(btn);
    btn.reset();
    EXPECT_FALSE(btn.releaseEvent());
}

// ---------------------------------------------------------------------------
// holdDurationMs()
// ---------------------------------------------------------------------------

TEST_F(ButtonTest, HoldDurationMs_ZeroBeforePress)
{
    Button btn(5);
    fake_time::value = 1000;
    EXPECT_EQ(btn.holdDurationMs(), 0UL);
}

TEST_F(ButtonTest, HoldDurationMs_NonZeroWhileHeld)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    fake_time::value = 700; // held for 500ms
    EXPECT_EQ(btn.holdDurationMs(), 500UL);
}

TEST_F(ButtonTest, HoldDurationMs_ZeroAfterRelease)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    fake_time::value = 700;
    release(btn); // awaitingRelease cleared
    EXPECT_EQ(btn.holdDurationMs(), 0UL);
}

// ---------------------------------------------------------------------------
// doublePressEvent()
// ---------------------------------------------------------------------------

TEST_F(ButtonTest, DoublePressEvent_FalseInitially)
{
    Button btn(5);
    EXPECT_FALSE(btn.doublePressEvent());
}

TEST_F(ButtonTest, DoublePressEvent_FalseForSinglePress)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    EXPECT_FALSE(btn.doublePressEvent());
}

TEST_F(ButtonTest, DoublePressEvent_TrueForTwoPressesWithinWindow)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    btn.event(); // consume
    fake_time::value = 310;
    release(btn);
    fake_time::value = 420; // 220ms after first press — within 300ms window
    press(btn);
    EXPECT_TRUE(btn.doublePressEvent());
}

TEST_F(ButtonTest, DoublePressEvent_FalseForTwoPressesOutsideWindow)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    btn.event();
    fake_time::value = 310;
    release(btn);
    fake_time::value = 520; // 320ms after first press — outside 300ms window
    press(btn);
    EXPECT_FALSE(btn.doublePressEvent());
}

TEST_F(ButtonTest, DoublePressEvent_FiresOnlyOnce)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    fake_time::value = 310;
    release(btn);
    fake_time::value = 420;
    press(btn);
    EXPECT_TRUE(btn.doublePressEvent());
    EXPECT_FALSE(btn.doublePressEvent()); // cleared
}

TEST_F(ButtonTest, DoublePressEvent_SuppressesSinglePress)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    btn.event(); // consume first single press
    fake_time::value = 310;
    release(btn);
    fake_time::value = 420;
    press(btn); // second press — double confirmed
    // doublePressEvent() should be true, event() should be false
    EXPECT_TRUE(btn.doublePressEvent());
    EXPECT_FALSE(btn.event()); // suppressed
}

// The existing DoublePressEvent_SuppressesSinglePress test *consumes* event()
// between the two presses; the tests below do not, covering the realistic
// polling pattern in src/main.cpp:process_events and the on-device interactive
// test for TASK-109.

// event() must not report the first press before the double-press window has
// elapsed — otherwise a second press cannot pre-empt it.
TEST_F(ButtonTest, DoublePressEvent_SingleNotLeakedWhenPolledBetweenPresses)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);

    fake_time::value = 250;
    EXPECT_FALSE(btn.doublePressEvent());
    EXPECT_FALSE(btn.event()); // deferred — second press may still arrive

    fake_time::value = 310;
    release(btn);
    fake_time::value = 350;
    EXPECT_FALSE(btn.event());

    fake_time::value = 420;
    press(btn);

    EXPECT_TRUE(btn.doublePressEvent());
    fake_time::value = 800;    // past the double-press window
    EXPECT_FALSE(btn.event()); // neither tap leaks as single
}

// When a double press is detected, neither of the two queued presses should
// leak out as a single-press event — even if event() is drained repeatedly.
TEST_F(ButtonTest, DoublePressEvent_BothQueuedPressesSuppressed)
{
    Button btn(5);
    fake_time::value = 200;
    press(btn);
    fake_time::value = 310;
    release(btn);
    fake_time::value = 420;
    press(btn); // double confirmed — pressCount cleared

    fake_time::value = 1000;

    EXPECT_TRUE(btn.doublePressEvent());
    EXPECT_FALSE(btn.event());
    EXPECT_FALSE(btn.event());
    EXPECT_FALSE(btn.event());
}

// ---------------------------------------------------------------------------
// Configurable debounce window (EPIC-028 / TASK-376)
// ---------------------------------------------------------------------------

TEST_F(ButtonTest, ConstructorWithExplicitDebounceSetsField)
{
    Button btn(5, 50);
    EXPECT_EQ(btn.debounceDelay, 50u);
}

TEST_F(ButtonTest, ConstructorWithDefault100ms)
{
    Button btn(5);
    EXPECT_EQ(btn.debounceDelay, 100u);
}

TEST_F(ButtonTest, ExplicitDebounce50ms_BounceJustOutsideWindowIsAccepted)
{
    // Two presses 60 ms apart: with default 100 ms debounce both would not
    // accept the second (60 < 100); with 50 ms debounce the second clears
    // the window and is accepted. Acceptance manifests as a double-press
    // since both edges fall inside the 300 ms double-press window.
    Button btn(5, 50);
    fake_time::value = 200;
    press(btn); // first press accepted, lastDebounceTime=200
    fake_time::value = 260;
    release(btn); // 60ms past press, debounce passed (60>50) — release accepted
    fake_time::value = 320;
    press(btn); // 60ms past lastDebounceTime=260, accepted under 50ms window
    EXPECT_FALSE(btn.event());
    EXPECT_TRUE(btn.doublePressEvent());
}

TEST_F(ButtonTest, DefaultDebounce100ms_BounceAtSameTimingIsRejected)
{
    // Same timings as the 50 ms test above, but with the default 100 ms
    // window the release at 260 ms (60ms past press) is filtered, so the
    // ISR never transitions to awaitingRelease=false. The second "press"
    // edge at 320 ms is dropped because awaitingRelease is still true.
    Button btn(5); // default 100 ms
    fake_time::value = 200;
    press(btn);
    fake_time::value = 260;
    release(btn); // 60ms < 100ms debounce — release rejected, awaitingRelease stays true
    fake_time::value = 320;
    press(btn); // awaitingRelease==true blocks this — not counted
    fake_time::value = 700;
    EXPECT_TRUE(btn.event()); // only the first press registered
    EXPECT_FALSE(btn.doublePressEvent());
}

TEST_F(ButtonTest, ExplicitDebounce250ms_BounceInsideWindowRejected)
{
    // Press → release 300 ms later (release accepted, 300>250). Then a
    // bounce 200 ms after the release (still inside the 250 ms window) is
    // filtered and never reaches event(). The initial press needs to be
    // past lastDebounceTime=0 by more than 250 ms.
    Button btn(5, 250);
    fake_time::value = 400;
    press(btn);             // accepted (400>250), lastDebounceTime=400
    fake_time::value = 700; // 300ms past press, debounce elapsed
    release(btn);           // release accepted, lastDebounceTime=700
    fake_time::value = 900; // 200ms past release — inside 250ms window
    press(btn);             // bounce filtered by debounce
    fake_time::value = 1500;
    EXPECT_TRUE(btn.event());  // first press only
    EXPECT_FALSE(btn.event()); // bounce was rejected
    EXPECT_FALSE(btn.doublePressEvent());
}

TEST_F(ButtonTest, ExplicitDebounce250ms_PressOutsideWindowAccepted)
{
    // Same setup but the second press lands 350 ms past the release-edge
    // debounce stamp — outside the 250 ms window so it is accepted. Also
    // outside the 300 ms double-press window from the first press, so it
    // counts as a separate single press.
    Button btn(5, 250);
    fake_time::value = 400;
    press(btn);
    fake_time::value = 700;
    release(btn); // lastDebounceTime=700
    fake_time::value = 1050;
    press(btn); // 350ms past lastDebounceTime, accepted. 650ms past first
                // press, outside the 300ms double-press window.
    fake_time::value = 1800;
    EXPECT_TRUE(btn.event()); // first press
    EXPECT_TRUE(btn.event()); // second press, accepted as separate single
}
