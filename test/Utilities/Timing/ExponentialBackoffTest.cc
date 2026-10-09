#include "ExponentialBackoffTest.h"

#include <limits>

#include "ExponentialBackoff.h"

using namespace std::chrono_literals;

void ExponentialBackoffTest::_growsToCapAndResets()
{
    ExponentialBackoff backoff(1s, 2, 30s);
    QCOMPARE(backoff.peek(), 1000ms);
    QCOMPARE(backoff.attempts(), 0);
    for (const auto expected : {1s, 2s, 4s, 8s, 16s, 30s, 30s}) {
        QCOMPARE(backoff.next(), std::chrono::milliseconds(expected));
    }
    QCOMPARE(backoff.attempts(), 7);
    backoff.reset();
    QCOMPARE(backoff.attempts(), 0);
    QCOMPARE(backoff.next(), 1000ms);
}

void ExponentialBackoffTest::_fractionalFactor()
{
    ExponentialBackoff backoff(1s, 1.5, 3s);
    for (const auto expected : {1000ms, 1500ms, 2250ms, 3000ms, 3000ms}) {
        QCOMPARE(backoff.next(), expected);
    }
}

void ExponentialBackoffTest::_retryAfter_data()
{
    QTest::addColumn<qint64>("maxRetryAfterMs");
    QTest::addColumn<int>("attempts");
    QTest::addColumn<qint64>("retryAfterMs");
    QTest::addColumn<qint64>("expectedMs");
    QTest::addColumn<qint64>("followingMs");
    QTest::newRow("longer-hint") << qint64(300000) << 0 << qint64(17000) << qint64(17000) << qint64(2000);
    QTest::newRow("shorter-hint") << qint64(300000) << 4 << qint64(1000) << qint64(16000) << qint64(30000);
    QTest::newRow("negative-hint") << qint64(300000) << 0 << qint64(-1) << qint64(1000) << qint64(2000);
    QTest::newRow("clamped-hint") << qint64(300000) << 0 << std::numeric_limits<qint64>::max() << qint64(300000)
                                  << qint64(2000);
    QTest::newRow("hints-disabled") << qint64(0) << 0 << qint64(17000) << qint64(1000) << qint64(2000);
}

void ExponentialBackoffTest::_retryAfter()
{
    QFETCH(qint64, maxRetryAfterMs);
    QFETCH(int, attempts);
    QFETCH(qint64, retryAfterMs);
    QFETCH(qint64, expectedMs);
    QFETCH(qint64, followingMs);
    ExponentialBackoff backoff(1s, 2, 30s, std::chrono::milliseconds(maxRetryAfterMs));
    for (int attempt = 0; attempt < attempts; ++attempt) {
        (void) backoff.next();
    }
    const std::chrono::milliseconds retryAfter(retryAfterMs);
    QCOMPARE(backoff.peek(retryAfter), std::chrono::milliseconds(expectedMs));
    QCOMPARE(backoff.next(retryAfter), std::chrono::milliseconds(expectedMs));
    // A hint lengthens one delay without changing the exponential sequence.
    QCOMPARE(backoff.next(), std::chrono::milliseconds(followingMs));
}

QGC_REGISTER_PORTABLE_TEST(ExponentialBackoffTest, TestLabel::Unit, TestLabel::Utilities)
