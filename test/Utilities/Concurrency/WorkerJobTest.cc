#include "WorkerJobTest.h"

#include <atomic>
#include <functional>
#include <memory>
#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QThreadPool>

#include "ProcessFixtures.h"
#include "WorkerJob.h"

namespace {

/// What happened to a WorkerTrackedResult and its copies
struct WorkerTrackedRecord
{
    std::atomic_int copies{0};
    std::atomic_int destroyed{0};
    std::atomic<QThread*> destroyedOn{nullptr};

    int instances() const { return 1 + copies - destroyed; }
};

/// A result that counts its copies and records the thread on which an instance was last destroyed
class WorkerTrackedResult
{
public:
    explicit WorkerTrackedResult(std::shared_ptr<WorkerTrackedRecord> record)
        : _record(std::move(record))
    {}

    WorkerTrackedResult(const WorkerTrackedResult& other)
        : _record(other._record)
    {
        if (_record) {
            ++_record->copies;
        }
    }

    WorkerTrackedResult(WorkerTrackedResult&& other) noexcept
        : _record(std::exchange(other._record, nullptr))
    {}

    WorkerTrackedResult& operator=(const WorkerTrackedResult&) = delete;
    WorkerTrackedResult& operator=(WorkerTrackedResult&&) = delete;

    ~WorkerTrackedResult()
    {
        if (_record) {
            _record->destroyedOn = QThread::currentThread();
            ++_record->destroyed;
        }
    }

private:
    std::shared_ptr<WorkerTrackedRecord> _record;
};

/// Shared with a work, so that it outlives a test that fails while the work runs
struct WorkerJobProbe
{
    QSemaphore started;
    QSemaphore mayEnd;
    std::atomic<QThread*> thread{nullptr};
};

/// What a dropped handler saw
struct WorkerDroppedProbe
{
    std::atomic_int calls{0};
    std::atomic<QThread*> thread{nullptr};
    std::atomic_int instances{0};
};

/// A work that makes a WorkerTrackedResult once the test lets it end
auto trackedWork(std::shared_ptr<WorkerTrackedRecord> record, std::shared_ptr<WorkerJobProbe> probe)
{
    return [record = std::move(record), probe = std::move(probe)]() {
        probe->thread = QThread::currentThread();
        probe->started.release();
        (void) probe->mayEnd.tryAcquire(1, TestTimeout::mediumMs());
        return WorkerTrackedResult(record);
    };
}

bool waitForWorkerPool()
{
    return QThreadPool::globalInstance()->waitForDone(TestTimeout::mediumMs());
}

}  // namespace

// done runs once, from the event loop, on the thread of the context, with the only instance of the result, which is
// destroyed there. The job is a child of the context and is gone when done runs. The dropped handler is not called
void WorkerJobTest::_testDoneGetsTheOnlyResultOnContextThread()
{
    const auto record = std::make_shared<WorkerTrackedRecord>();
    const auto probe = std::make_shared<WorkerJobProbe>();
    const auto dropped = std::make_shared<WorkerDroppedProbe>();
    probe->mayEnd.release();

    QObject context;
    QPointer<QObject> job;
    int doneCalls = 0;
    QThread* doneThread = nullptr;
    int instancesInDone = 0;
    bool jobGoneInDone = false;
    job = QGC::runOnWorker(
        &context, trackedWork(record, probe),
        [&](WorkerTrackedResult) {
            ++doneCalls;
            doneThread = QThread::currentThread();
            instancesInDone = record->instances();
            jobGoneInDone = job.isNull();
        },
        [dropped](WorkerTrackedResult) { ++dropped->calls; });
    QCOMPARE(job->parent(), &context);

    QVERIFY(waitForWorkerPool());
    QCOMPARE(doneCalls, 0);
    QCoreApplication::sendPostedEvents();

    QCOMPARE(doneCalls, 1);
    QCOMPARE(doneThread, QThread::currentThread());
    QVERIFY(probe->thread.load() != QThread::currentThread());
    QCOMPARE(instancesInDone, 1);
    QVERIFY(jobGoneInDone);
    QCOMPARE(record->copies.load(), 0);
    QCOMPARE(record->destroyed.load(), 1);
    QCOMPARE(record->destroyedOn.load(), QThread::currentThread());
    QCOMPARE(dropped->calls.load(), 0);
}

// A work that has ended does not call done before the event loop runs, however early it ended: here before any event
// was handled, run on this thread by waitForFinished() as no worker thread is free
void WorkerJobTest::_testDoneWaitsForTheEventLoop()
{
    TestFixtures::ThreadPoolBlocker blocker;
    QVERIFY(blocker.isBlocking());

    QObject context;
    int doneCalls = 0;
    QGC::WorkerJob<int>* const job = QGC::runOnWorker(&context, []() { return 1; }, [&doneCalls](int) { ++doneCalls; });
    job->ended().waitForFinished();
    QCOMPARE(doneCalls, 0);

    blocker.release();
    QCoreApplication::sendPostedEvents();
    QCOMPARE(doneCalls, 1);
}

void WorkerJobTest::_testDeletingJobOrContextDropsResult_data()
{
    QTest::addColumn<bool>("deleteContext");
    QTest::addColumn<QString>("when");

    const QString beforeStart = QStringLiteral("before start");
    const QString whileRunning = QStringLiteral("while running");
    const QString afterEnd = QStringLiteral("after end");

    QTest::newRow("job deleted before the work starts") << false << beforeStart;
    QTest::newRow("job deleted while the work runs") << false << whileRunning;
    QTest::newRow("job deleted after the work ended, before the event") << false << afterEnd;
    QTest::newRow("context deleted before the work starts") << true << beforeStart;
    QTest::newRow("context deleted while the work runs") << true << whileRunning;
    QTest::newRow("context deleted after the work ended, before the event") << true << afterEnd;
}

// Deleting the job, directly or with its context, before done drops the result: done is not called, and the result,
// never copied, is destroyed once
void WorkerJobTest::_testDeletingJobOrContextDropsResult()
{
    QFETCH(bool, deleteContext);
    QFETCH(QString, when);

    const auto record = std::make_shared<WorkerTrackedRecord>();
    const auto probe = std::make_shared<WorkerJobProbe>();
    std::unique_ptr<TestFixtures::ThreadPoolBlocker> blocker;
    if (when == QStringLiteral("before start")) {
        blocker = std::make_unique<TestFixtures::ThreadPoolBlocker>();
        QVERIFY(blocker->isBlocking());
    }

    auto context = std::make_unique<QObject>();
    bool doneCalled = false;
    const QPointer<QObject> job = QGC::runOnWorker(context.get(), trackedWork(record, probe),
                                                   [&doneCalled](WorkerTrackedResult) { doneCalled = true; });
    if (when == QStringLiteral("while running")) {
        QVERIFY(probe->started.tryAcquire(1, TestTimeout::mediumMs()));
    } else if (when == QStringLiteral("after end")) {
        probe->mayEnd.release();
        QVERIFY(waitForWorkerPool());
    }

    if (deleteContext) {
        context.reset();
    } else {
        delete job.data();
    }
    QVERIFY(job.isNull());

    probe->mayEnd.release();
    if (blocker) {
        blocker->release();
    }
    QVERIFY(waitForWorkerPool());
    QCoreApplication::sendPostedEvents();

    QVERIFY(!doneCalled);
    QCOMPARE(record->copies.load(), 0);
    QCOMPARE(record->destroyed.load(), 1);
}

void WorkerJobTest::_testDroppedHandlerGetsTheResult_data()
{
    QTest::addColumn<bool>("deleteContext");
    QTest::addColumn<QString>("when");
    QTest::addColumn<bool>("onWorker");

    QTest::newRow("job deleted before the work starts") << false << QStringLiteral("before start") << true;
    QTest::newRow("context deleted while the work runs") << true << QStringLiteral("while running") << true;
    QTest::newRow("job deleted after the work ended") << false << QStringLiteral("after end") << false;
}

// A dropped result goes to the dropped handler, once and as the only instance: on the worker thread when the work
// ends, or at once, on the thread that deletes the job, if it has ended
void WorkerJobTest::_testDroppedHandlerGetsTheResult()
{
    QFETCH(bool, deleteContext);
    QFETCH(QString, when);
    QFETCH(bool, onWorker);

    const auto record = std::make_shared<WorkerTrackedRecord>();
    const auto probe = std::make_shared<WorkerJobProbe>();
    const auto dropped = std::make_shared<WorkerDroppedProbe>();
    std::unique_ptr<TestFixtures::ThreadPoolBlocker> blocker;
    if (when == QStringLiteral("before start")) {
        blocker = std::make_unique<TestFixtures::ThreadPoolBlocker>();
        QVERIFY(blocker->isBlocking());
    }

    auto context = std::make_unique<QObject>();
    bool doneCalled = false;
    const QPointer<QObject> job = QGC::runOnWorker(
        context.get(), trackedWork(record, probe), [&doneCalled](WorkerTrackedResult) { doneCalled = true; },
        [record, dropped](WorkerTrackedResult) {
            dropped->thread = QThread::currentThread();
            dropped->instances = record->instances();
            ++dropped->calls;
        });
    if (when == QStringLiteral("while running")) {
        QVERIFY(probe->started.tryAcquire(1, TestTimeout::mediumMs()));
    } else if (when == QStringLiteral("after end")) {
        probe->mayEnd.release();
        QVERIFY(waitForWorkerPool());
    }

    if (deleteContext) {
        context.reset();
    } else {
        delete job.data();
    }
    QCOMPARE(dropped->calls.load(), onWorker ? 0 : 1);

    probe->mayEnd.release();
    if (blocker) {
        blocker->release();
    }
    QVERIFY(waitForWorkerPool());
    QCoreApplication::sendPostedEvents();

    QVERIFY(!doneCalled);
    QCOMPARE(dropped->calls.load(), 1);
    QCOMPARE(dropped->thread.load(), onWorker ? probe->thread.load() : QThread::currentThread());
    QCOMPARE(dropped->instances.load(), 1);
    QCOMPARE(record->copies.load(), 0);
    QCOMPARE(record->destroyed.load(), 1);
}

// done may delete the context, which the job belongs to
void WorkerJobTest::_testDoneDeletesContext()
{
    auto context = std::make_unique<QObject>();
    int doneCalls = 0;
    const QPointer<QObject> job = QGC::runOnWorker(
        context.get(), []() { return 1; },
        [&](int) {
            ++doneCalls;
            context.reset();
        });

    QVERIFY(waitForWorkerPool());
    QCoreApplication::sendPostedEvents();
    QCOMPARE(doneCalls, 1);
    QVERIFY(!context);
    QVERIFY(job.isNull());
}

// done may start the next job and, as a call site that drops the job it tracks does, delete the one that called it
void WorkerJobTest::_testDoneStartsNextJob()
{
    QObject context;
    QPointer<QObject> tracked;
    QList<int> results;
    std::function<void(int)> start;
    start = [&](int value) {
        delete tracked.data();
        tracked = QGC::runOnWorker(
            &context, [value]() { return value; },
            [&](int result) {
                results.append(result);
                if (result == 1) {
                    start(2);
                }
            });
    };

    start(1);
    QTRY_COMPARE_WITH_TIMEOUT(results, (QList<int>{1, 2}), TestTimeout::mediumMs());
    QVERIFY(tracked.isNull());
}

QGC_REGISTER_PORTABLE_TEST(WorkerJobTest, TestLabel::Unit, TestLabel::Utilities)
