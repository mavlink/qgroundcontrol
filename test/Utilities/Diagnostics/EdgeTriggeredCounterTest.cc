#include "EdgeTriggeredCounterTest.h"

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

UT_REGISTER_TEST(EdgeTriggeredCounterTest, TestLabel::Unit, TestLabel::Utilities)
