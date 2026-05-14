#include "button.h"
#include "config.h"
#include "null_logger.h"
#include "pedal_config.h"
#include "pin_name_table.h"
#include <Arduino.h>
#include <unity.h>

// --- Buttons under test -----------------------------------------------------

static Button btnA(GPIO_NUM_13);
static Button btnB(GPIO_NUM_12);
static Button btnC(GPIO_NUM_27);
static Button btnD(GPIO_NUM_14);
static Button btnSel(GPIO_NUM_21);

// --- Helpers ----------------------------------------------------------------

static Button* btn = nullptr;

static void isr_handler() { btn->isr(); }
static void isr_handler_a() { btnA.isr(); }
static void isr_handler_b() { btnB.isr(); }

static void setup_button(Button& b, uint8_t pin)
{
    btn = &b;
    b.setup();
    b.reset();
    attachInterrupt(pin, isr_handler, CHANGE);
}

static void teardown_button(uint8_t pin) { detachInterrupt(pin); }

static int count_events(Button& b, unsigned long window_ms)
{
    int count = 0;
    unsigned long start = millis();
    while (millis() - start < window_ms)
    {
        if (b.event())
            count++;
    }
    return count;
}

static void countdown(int seconds)
{
    for (int i = seconds; i > 0; i--)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "  %ds remaining...", i);
        TEST_MESSAGE(buf);
        delay(1000);
    }
}

static void prompt(const char* msg, int seconds)
{
    TEST_MESSAGE("--------------------------------------------------");
    TEST_MESSAGE(msg);
    countdown(seconds);
}

// --- Tests ------------------------------------------------------------------

void test_single_press_fires_one_event()
{
    setup_button(btnA, GPIO_NUM_13);
    prompt("ACTION: Press button A ONCE", 3);
    int n = count_events(btnA, 500);
    teardown_button(GPIO_NUM_13);
    TEST_ASSERT_EQUAL_MESSAGE(1, n, "Expected exactly 1 event for single press");
}

void test_no_event_before_press()
{
    setup_button(btnA, GPIO_NUM_13);
    prompt("ACTION: Do NOT press any button", 2);
    int n = count_events(btnA, 500);
    teardown_button(GPIO_NUM_13);
    TEST_ASSERT_EQUAL_MESSAGE(0, n, "Expected 0 events with no press");
}

void test_double_press_fires_two_events()
{
    setup_button(btnA, GPIO_NUM_13);
    prompt("ACTION: Press button A TWICE — press, wait 1s, press again", 7);
    int n = count_events(btnA, 500);
    teardown_button(GPIO_NUM_13);
    TEST_ASSERT_EQUAL_MESSAGE(2, n, "Expected exactly 2 events for double press");
}

void test_hold_fires_one_event()
{
    setup_button(btnA, GPIO_NUM_13);
    prompt("ACTION: Press and HOLD button A for 2s then release", 6);
    int n = count_events(btnA, 1000);
    teardown_button(GPIO_NUM_13);
    TEST_ASSERT_EQUAL_MESSAGE(1, n, "Expected exactly 1 event when holding button");
}

void test_release_does_not_fire_event()
{
    setup_button(btnA, GPIO_NUM_13);
    prompt("ACTION: Press button A once, then do NOT press again", 3);
    count_events(btnA, 200);          // drain the press event
    int n = count_events(btnA, 1000); // should be 0 on release
    teardown_button(GPIO_NUM_13);
    TEST_ASSERT_EQUAL_MESSAGE(0, n, "Expected 0 events on release");
}

void test_reset_clears_stale_event()
{
    setup_button(btnA, GPIO_NUM_13);
    prompt("ACTION: Press button A once, then do NOT press again", 2);
    btnA.reset();
    // drain anything pressed during countdown, then check nothing fires in 1s
    unsigned long start = millis();
    while (millis() - start < 200)
    {
        btnA.event();
    } // flush
    int n = count_events(btnA, 800);
    teardown_button(GPIO_NUM_13);
    TEST_ASSERT_EQUAL_MESSAGE(0, n, "Expected 0 events after reset()");
}

void test_button_b_single_press()
{
    setup_button(btnB, GPIO_NUM_12);
    prompt("ACTION: Press button B ONCE", 3);
    int n = count_events(btnB, 500);
    teardown_button(GPIO_NUM_12);
    TEST_ASSERT_EQUAL_MESSAGE(1, n, "Button B: expected 1 event");
}

void test_button_c_single_press()
{
    setup_button(btnC, GPIO_NUM_27);
    prompt("ACTION: Press button C ONCE", 3);
    int n = count_events(btnC, 500);
    teardown_button(GPIO_NUM_27);
    TEST_ASSERT_EQUAL_MESSAGE(1, n, "Button C: expected 1 event");
}

void test_button_d_single_press()
{
    setup_button(btnD, GPIO_NUM_14);
    prompt("ACTION: Press button D ONCE", 3);
    int n = count_events(btnD, 500);
    teardown_button(GPIO_NUM_14);
    TEST_ASSERT_EQUAL_MESSAGE(1, n, "Button D: expected 1 event");
}

void test_button_select_single_press()
{
    setup_button(btnSel, GPIO_NUM_21);
    prompt("ACTION: Press SELECT button ONCE", 3);
    int n = count_events(btnSel, 500);
    teardown_button(GPIO_NUM_21);
    TEST_ASSERT_EQUAL_MESSAGE(1, n, "SELECT: expected 1 event");
}

void test_two_buttons_sequential()
{
    btnA.setup();
    btnA.reset();
    attachInterrupt(GPIO_NUM_13, isr_handler_a, CHANGE);
    btnB.setup();
    btnB.reset();
    attachInterrupt(GPIO_NUM_12, isr_handler_b, CHANGE);
    prompt("ACTION: Press button A then button B (wait >300ms between)", 6);
    int na = count_events(btnA, 300);
    int nb = count_events(btnB, 300);
    detachInterrupt(GPIO_NUM_13);
    detachInterrupt(GPIO_NUM_12);
    TEST_ASSERT_EQUAL_MESSAGE(1, na, "Button A: expected 1 event");
    TEST_ASSERT_EQUAL_MESSAGE(1, nb, "Button B: expected 1 event");
}

// --- EPIC-028 / TASK-376: configurable debounce ---------------------------

// Local fixture constructed with the 2-arg constructor so the debounce window
// is locked in before attachInterrupt. The same as the hardware-config path:
// Button(pin, hardwareConfig.debounceMs) → setup → attachInterrupt.
static Button btnA_slow(GPIO_NUM_13, 250);
static void isr_handler_a_slow() { btnA_slow.isr(); }

void test_configurable_debounce_250ms_suppresses_fast_double_press()
{
    btnA_slow.setup();
    btnA_slow.reset();
    attachInterrupt(GPIO_NUM_13, isr_handler_a_slow, CHANGE);
    prompt("ACTION: Press button A TWICE quickly — within 250ms (one tap)", 5);
    int n = count_events(btnA_slow, 500);
    detachInterrupt(GPIO_NUM_13);
    TEST_ASSERT_EQUAL_MESSAGE(
        1, n, "Two presses inside the 250ms debounce window must register as one");
}

void test_configurable_debounce_250ms_accepts_slow_double_press()
{
    btnA_slow.setup();
    btnA_slow.reset();
    attachInterrupt(GPIO_NUM_13, isr_handler_a_slow, CHANGE);
    prompt("ACTION: Press button A TWICE — press, wait ~1s, press again", 7);
    int n = count_events(btnA_slow, 500);
    detachInterrupt(GPIO_NUM_13);
    TEST_ASSERT_EQUAL_MESSAGE(
        2, n, "Two presses >1s apart must register as two events even with 250ms debounce");
}

// --- EPIC-029 / TASK-380: named-pin resolution on real ESP32 RAM ----------
//
// Non-interactive — exercises ArduinoJson + the g_pinNameTable build path
// on the actual device rather than the host shim. Confirms that a
// hardware config with a pinNames object populates the table and a
// looked-up named pin returns the right physical GPIO.

void test_pin_names_resolve_on_device()
{
    NullLogger logger;
    g_pinNameTable.clear();

    const char* configJson = R"json({
        "hardware": "esp32",
        "numProfiles": 3,
        "numSelectLeds": 2,
        "numButtons": 4,
        "ledBluetooth": 26,
        "ledPower": 25,
        "ledSelect": [5, 18],
        "buttonSelect": 21,
        "buttonPins": [13, 12, 27, 14],
        "pinNames": {
            "13": "button_a",
            "12": "button_b",
            "26": "led_bluetooth"
        }
    })json";

    bool ok = loadHardwareConfigFromJson(configJson, &logger);
    TEST_ASSERT_TRUE_MESSAGE(
        ok, "loadHardwareConfigFromJson should accept a valid config with pinNames");
    TEST_ASSERT_EQUAL_UINT8(13, g_pinNameTable.lookup("button_a"));
    TEST_ASSERT_EQUAL_UINT8(12, g_pinNameTable.lookup("button_b"));
    TEST_ASSERT_EQUAL_UINT8(26, g_pinNameTable.lookup("led_bluetooth"));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PinNameTable::kUnresolved),
                            g_pinNameTable.lookup("button_z"));

    g_pinNameTable.clear();
}

// --- Entry point ------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(2000);

    UNITY_BEGIN();

    TEST_MESSAGE("==================================================");
    TEST_MESSAGE("  INTERACTIVE BUTTON TEST — ACTION PLAN");
    TEST_MESSAGE("==================================================");
    TEST_MESSAGE("   1. Do NOT press any button          (2s)");
    TEST_MESSAGE("   2. Press button A ONCE              (3s)");
    TEST_MESSAGE("   3. Press button A TWICE, ~1s apart  (7s)");
    TEST_MESSAGE("   4. Press and HOLD button A          (6s)");
    TEST_MESSAGE("   5. Press button A once, no more     (3s)");
    TEST_MESSAGE("   6. Press button A once, no more     (2s)");
    TEST_MESSAGE("   7. Press button B ONCE              (3s)");
    TEST_MESSAGE("   8. Press button C ONCE              (3s)");
    TEST_MESSAGE("   9. Press button D ONCE              (3s)");
    TEST_MESSAGE("  10. Press SELECT ONCE                (3s)");
    TEST_MESSAGE("  11. Press A then B, ~1s apart        (6s)");
    TEST_MESSAGE("  12. [250ms debounce] Press A FAST    (5s)");
    TEST_MESSAGE("  13. [250ms debounce] Press A, wait,A (7s)");
    TEST_MESSAGE("==================================================");
    TEST_MESSAGE("  Starting in 5 seconds...");
    TEST_MESSAGE("==================================================");
    delay(5000);

    RUN_TEST(test_no_event_before_press);
    RUN_TEST(test_single_press_fires_one_event);
    RUN_TEST(test_double_press_fires_two_events);
    RUN_TEST(test_hold_fires_one_event);
    RUN_TEST(test_release_does_not_fire_event);
    RUN_TEST(test_reset_clears_stale_event);
    RUN_TEST(test_button_b_single_press);
    RUN_TEST(test_button_c_single_press);
    RUN_TEST(test_button_d_single_press);
    RUN_TEST(test_button_select_single_press);
    RUN_TEST(test_two_buttons_sequential);
    RUN_TEST(test_configurable_debounce_250ms_suppresses_fast_double_press);
    RUN_TEST(test_configurable_debounce_250ms_accepts_slow_double_press);
    RUN_TEST(test_pin_names_resolve_on_device);

    UNITY_END();
}

void loop() {}
