#include "EdgeTriggeredCounterTest.h"

#include <cstdint>
#include <limits>

#include "EdgeTriggeredCounter.h"

void EdgeTriggeredCounterTest::_testFiresOnceWhenThresholdIsReached()
{
    QGC::EdgeTriggeredCounter<int> counter(3);

    QVERIFY(!counter.record());
    QVERIFY(!counter.record());
    QVERIFY(!counter.armed());

    QVERIFY(counter.record());
    QVERIFY(counter.armed());
    QCOMPARE(counter.count(), 3);

    QVERIFY(!counter.record());
    QCOMPARE(counter.count(), 4);
}

void EdgeTriggeredCounterTest::_testResetStartsNewBurst()
{
    QGC::EdgeTriggeredCounter<int> counter(2);

    // A reset before the threshold discards the events counted so far
    QVERIFY(!counter.record());
    counter.reset();
    QCOMPARE(counter.count(), 0);
    QVERIFY(!counter.record());
    QVERIFY(counter.record());

    // A reset after it has fired lets it fire again
    counter.reset();
    QVERIFY(!counter.armed());
    QVERIFY(!counter.record());
    QVERIFY(counter.record());
}

void EdgeTriggeredCounterTest::_testLongBurstSaturatesCount()
{
    // SigningController counts bad signatures in a uint8_t, which a long burst would overflow
    QGC::EdgeTriggeredCounter<uint8_t> counter(3);

    int fired = 0;
    for (int i = 0; i < 600; ++i) {
        fired += counter.record() ? 1 : 0;
    }
    QCOMPARE(fired, 1);
    QCOMPARE(static_cast<int>(counter.count()), static_cast<int>(std::numeric_limits<uint8_t>::max()));
}

UT_REGISTER_TEST(EdgeTriggeredCounterTest, TestLabel::Unit, TestLabel::Utilities)
