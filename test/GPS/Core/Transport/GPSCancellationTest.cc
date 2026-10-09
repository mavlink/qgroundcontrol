#include "GPSCancellationTest.h"

#include <atomic>
#include <memory>
#include <thread>

#include <QtCore/QScopeGuard>
#include <QtCore/QSemaphore>

#include "GPSCancellation.h"

void GPSCancellationTest::_cancelRunsCallbacksOnce()
{
    int neverCancelled = 0;
    {
        const GPSCancelCallback callback(GPSCancelToken{}, [&] { ++neverCancelled; });
    }
    QVERIFY(!GPSCancelToken{}.isCancelled());

    GPSCancelSource source;
    const GPSCancelToken token = source.token();
    int unregistered = 0;
    {
        const GPSCancelCallback callback(token, [&] { ++unregistered; });
    }
    int runs = 0;
    std::thread::id ranOn;
    const GPSCancelCallback callback(token, [&] {
        ++runs;
        ranOn = std::this_thread::get_id();
    });
    std::thread::id cancelledOn;
    bool cancelled = false;
    bool cancelledAgain = true;
    std::thread canceller([&] {
        cancelledOn = std::this_thread::get_id();
        cancelled = source.cancel();
        // Copies share the cancellation.
        cancelledAgain = GPSCancelSource(source).cancel();
    });
    canceller.join();
    QVERIFY(cancelled);
    QVERIFY(!cancelledAgain);
    QCOMPARE(runs, 1);
    QVERIFY(ranOn == cancelledOn);
    QVERIFY(token.isCancelled());
    QCOMPARE(unregistered, 0);

    int late = 0;
    const GPSCancelCallback lateCallback(token, [&] { ++late; });
    QCOMPARE(late, 1);
    QCOMPARE(neverCancelled, 0);
}

void GPSCancellationTest::_destructionWaitsForRunningCallback()
{
    GPSCancelSource source;
    QSemaphore entered;
    QSemaphore proceed;
    QSemaphore destroyed;
    std::atomic_bool callbackReturned = false;
    auto callback = std::make_unique<GPSCancelCallback>(source.token(), [&] {
        entered.release();
        (void) proceed.tryAcquire(1, TestTimeout::mediumMs());
        callbackReturned = true;
    });
    std::thread canceller([&] { (void) source.cancel(); });
    std::thread destroyer;
    bool returnedBeforeDestruction = false;
    const auto joinThreads = qScopeGuard([&] {
        proceed.release();
        canceller.join();
        if (destroyer.joinable()) {
            destroyer.join();
        }
    });
    QVERIFY(entered.tryAcquire(1, TestTimeout::mediumMs()));
    destroyer = std::thread([&] {
        callback.reset();
        returnedBeforeDestruction = callbackReturned.load();
        destroyed.release();
    });
    // Showing that destruction blocks takes a bounded wait for an absence, so it is kept short.
    QVERIFY(!destroyed.tryAcquire(1, 100));
    proceed.release();
    QVERIFY(destroyed.tryAcquire(1, TestTimeout::mediumMs()));
    QVERIFY(returnedBeforeDestruction);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSCancellationTest, TestLabel::Unit)
