#include "KeyframeIntervalMeterTest.h"

#include "KeyframeIntervalMeter.h"

using namespace std::chrono_literals;

void KeyframeIntervalMeterTest::_testAveragesOverWindow()
{
    KeyframeIntervalMeter meter;
    QCOMPARE(meter.intervalMs(), -1.0);
    meter.note(0ms);
    QCOMPARE(meter.intervalMs(), -1.0);
    // The same keyframe again, as its parameter sets in a buffer of their own.
    meter.note(0ms);
    QCOMPARE(meter.intervalMs(), -1.0);

    // A 500 ms GOP whose keyframes arrive bunched up, read from arrival times: the average evens it out.
    meter.note(500ms);
    QCOMPARE(meter.intervalMs(), 500.0);
    meter.note(1080ms);
    meter.note(1420ms);
    meter.note(2000ms);
    QCOMPARE(meter.intervalMs(), 500.0);

    // The window moves on: the average of the last five keyframes.
    meter.note(2700ms);
    QCOMPARE(meter.intervalMs(), 550.0);
}

void KeyframeIntervalMeterTest::_testTimeGoingBackStartsOver()
{
    KeyframeIntervalMeter meter;
    meter.note(1000ms);
    meter.note(2000ms);
    meter.note(3000ms);
    QCOMPARE(meter.intervalMs(), 1000.0);

    // The last interval stands until the stream that started over has two keyframes of its own.
    meter.note(100ms);
    QCOMPARE(meter.intervalMs(), 1000.0);
    meter.note(400ms);
    QCOMPARE(meter.intervalMs(), 300.0);
}

UT_REGISTER_TEST(KeyframeIntervalMeterTest, TestLabel::Unit)
