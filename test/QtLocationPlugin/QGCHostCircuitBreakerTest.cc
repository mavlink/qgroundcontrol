#include "QGCHostCircuitBreakerTest.h"

#include "QGCHostCircuitBreaker.h"

// HostCircuitBreaker::instance() is a process-wide singleton, so each test uses a
// unique host key to avoid state leaking between cases. The 60s open / 30s window
// transitions are wall-clock based and not exercised here (would require time
// injection); these tests cover the failure-threshold and success-reset logic.

void QGCHostCircuitBreakerTest::_testEmptyHostAlwaysAllowed()
{
    HostCircuitBreaker &cb = HostCircuitBreaker::instance();
    QVERIFY(cb.allowRequest(QString()));
    cb.recordFailure(QString());
    QVERIFY(cb.allowRequest(QString()));
}

void QGCHostCircuitBreakerTest::_testBelowThresholdStaysClosed()
{
    HostCircuitBreaker &cb = HostCircuitBreaker::instance();
    const QString host = QStringLiteral("cb-below.example");
    for (int i = 0; i < 4; ++i) {
        cb.recordFailure(host);
        QVERIFY(cb.allowRequest(host));
    }
}

void QGCHostCircuitBreakerTest::_testFifthFailureOpens()
{
    HostCircuitBreaker &cb = HostCircuitBreaker::instance();
    const QString host = QStringLiteral("cb-open.example");
    for (int i = 0; i < 5; ++i) {
        cb.recordFailure(host);
    }
    QVERIFY(!cb.allowRequest(host));
}

void QGCHostCircuitBreakerTest::_testSuccessResetsFailureCounter()
{
    HostCircuitBreaker &cb = HostCircuitBreaker::instance();
    const QString host = QStringLiteral("cb-reset.example");

    for (int i = 0; i < 4; ++i) {
        cb.recordFailure(host);
    }
    cb.recordSuccess(host);

    // Counter reset: four more failures alone must not trip the breaker.
    for (int i = 0; i < 4; ++i) {
        cb.recordFailure(host);
        QVERIFY(cb.allowRequest(host));
    }
    // The fifth still opens, proving the threshold is intact after reset.
    cb.recordFailure(host);
    QVERIFY(!cb.allowRequest(host));
}

UT_REGISTER_TEST(QGCHostCircuitBreakerTest, TestLabel::Unit)
