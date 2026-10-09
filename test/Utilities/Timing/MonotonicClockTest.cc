#include "MonotonicClockTest.h"

#include "MonotonicClock.h"

using namespace std::chrono_literals;

void MonotonicClockTest::_fresh_data()
{
    QTest::addColumn<quint64>("timestampUs");
    QTest::addColumn<qint64>("lifetimeUs");
    QTest::addColumn<bool>("fresh");
    QTest::addColumn<bool>("withinAge");
    constexpr quint64 NOW_US = 10'000'000;
    QTest::newRow("current") << NOW_US << qint64(1000) << true << true;
    QTest::newRow("younger-than-lifetime") << NOW_US - 999 << qint64(1000) << true << true;
    // withinAge() includes its limit.
    QTest::newRow("lifetime-reached") << NOW_US - 1000 << qint64(1000) << false << true;
    QTest::newRow("older-than-lifetime") << NOW_US - 1001 << qint64(1000) << false << false;
    QTest::newRow("missing") << quint64(0) << qint64(NOW_US) << false << false;
    QTest::newRow("future") << NOW_US + 1 << qint64(1000) << false << false;
    QTest::newRow("no-lifetime") << NOW_US << qint64(0) << false << true;
}

void MonotonicClockTest::_fresh()
{
    QFETCH(quint64, timestampUs);
    QFETCH(qint64, lifetimeUs);
    QFETCH(bool, fresh);
    QFETCH(bool, withinAge);
    constexpr uint64_t NOW_US = 10'000'000;
    QCOMPARE(MonotonicClock::fresh(timestampUs, NOW_US, std::chrono::microseconds(lifetimeUs)), fresh);
    QCOMPARE(MonotonicClock::withinAge(timestampUs, NOW_US, std::chrono::microseconds(lifetimeUs)), withinAge);
    static_assert(MonotonicClock::fresh(NOW_US - 1, NOW_US, 1s));
}

void MonotonicClockTest::_age_data()
{
    QTest::addColumn<qint64>("timestampMs");
    QTest::addColumn<qint64>("expectedMs");
    constexpr qint64 NOW_MS = 10'000;
    // -1 means no valid age.
    QTest::newRow("current") << NOW_MS << qint64(0);
    QTest::newRow("past") << NOW_MS - 250 << qint64(250);
    QTest::newRow("missing") << qint64(0) << qint64(-1);
    QTest::newRow("negative") << qint64(-5) << qint64(-1);
    QTest::newRow("future") << NOW_MS + 1 << qint64(-1);
}

void MonotonicClockTest::_age()
{
    QFETCH(qint64, timestampMs);
    QFETCH(qint64, expectedMs);
    const auto age = MonotonicClock::age(std::chrono::milliseconds(timestampMs), 10'000ms);
    QCOMPARE(age.has_value(), expectedMs >= 0);
    if (age) {
        QCOMPARE(age->count(), expectedMs);
    }
    static_assert(MonotonicClock::age(uint64_t{1}, uint64_t{3}) == 2us);
    static_assert(!MonotonicClock::age(uint64_t{0}, uint64_t{3}));
    static_assert(!MonotonicClock::age(uint64_t{4}, uint64_t{3}));
}

QGC_REGISTER_PORTABLE_TEST(MonotonicClockTest, TestLabel::Unit, TestLabel::Utilities)
