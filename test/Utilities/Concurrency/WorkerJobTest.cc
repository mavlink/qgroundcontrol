#include "WorkerJobTest.h"

#include <atomic>
#include <functional>
#include <memory>
#include <utility>

#include <QtCore/QFuture>
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
    QTest::addColumn<bool>("droppedHandler");

    const QString beforeStart = QStringLiteral("before start");
    const QString whileRunning = QStringLiteral("while running");
    const QString afterEnd = QStringLiteral("after end");

    QTest::newRow("job deleted before the work starts") << false << beforeStart << false;
    QTest::newRow("job deleted while the work runs") << false << whileRunning << false;
    QTest::newRow("job deleted after the work ended, before the event") << false << afterEnd << false;
    QTest::newRow("context deleted before the work starts") << true << beforeStart << false;
    QTest::newRow("context deleted while the work runs") << true << whileRunning << false;
    QTest::newRow("context deleted after the work ended, before the event") << true << afterEnd << false;
    QTest::newRow("dropped handler, job deleted before the work starts") << false << beforeStart << true;
    QTest::newRow("dropped handler, context deleted while the work runs") << true << whileRunning << true;
    QTest::newRow("dropped handler, job deleted after the work ended") << false << afterEnd << true;
}

// Deleting the job, directly or with its context, before done drops the result: done is not called, and the result,
// never copied, is destroyed once. It goes to a dropped handler, if there is one, once and as the only instance: on
// the worker thread when the work ends, or at once, on the thread that deletes the job, if it has ended. ended()
// tells when the work has ended, not when the job is gone: a caller waits for it before it removes what the work reads
void WorkerJobTest::_testDeletingJobOrContextDropsResult()
{
    QFETCH(bool, deleteContext);
    QFETCH(QString, when);
    QFETCH(bool, droppedHandler);
    const bool beforeStart = (when == QStringLiteral("before start"));
    const bool afterEnd = (when == QStringLiteral("after end"));

    const auto record = std::make_shared<WorkerTrackedRecord>();
    const auto probe = std::make_shared<WorkerJobProbe>();
    const auto dropped = std::make_shared<WorkerDroppedProbe>();
    std::unique_ptr<TestFixtures::ThreadPoolBlocker> blocker;
    if (beforeStart) {
        blocker = std::make_unique<TestFixtures::ThreadPoolBlocker>();
        QVERIFY(blocker->isBlocking());
    }

    auto context = std::make_unique<QObject>();
    bool doneCalled = false;
    QGC::WorkerJob<WorkerTrackedResult>::Handler droppedTo;
    if (droppedHandler) {
        droppedTo = [record, dropped](WorkerTrackedResult) {
            dropped->thread = QThread::currentThread();
            dropped->instances = record->instances();
            ++dropped->calls;
        };
    }
    auto* const startedJob = QGC::runOnWorker(
        context.get(), trackedWork(record, probe), [&doneCalled](WorkerTrackedResult) { doneCalled = true; },
        droppedTo);
    const QPointer<QObject> job = startedJob;
    const QFuture<void> ended = startedJob->ended();
    if (when == QStringLiteral("while running")) {
        QVERIFY(probe->started.tryAcquire(1, TestTimeout::mediumMs()));
    } else if (afterEnd) {
        probe->mayEnd.release();
        QVERIFY(waitForWorkerPool());
    }

    if (deleteContext) {
        context.reset();
    } else {
        delete job.data();
    }
    QVERIFY(job.isNull());
    if (beforeStart) {
        // Every thread is still held, so the work waits for one: the row is not one of a running work by another name
        QVERIFY(QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount());
        QVERIFY(probe->thread.load() == nullptr);
    }
    QCOMPARE(ended.isFinished(), afterEnd);
    QCOMPARE(dropped->calls.load(), (droppedHandler && afterEnd) ? 1 : 0);

    probe->mayEnd.release();
    if (blocker) {
        blocker->release();
    }
    QVERIFY(waitForWorkerPool());
    QCoreApplication::sendPostedEvents();

    QVERIFY(!doneCalled);
    QVERIFY(ended.isFinished());
    QCOMPARE(record->copies.load(), 0);
    QCOMPARE(record->destroyed.load(), 1);
    QCOMPARE(dropped->calls.load(), droppedHandler ? 1 : 0);
    if (droppedHandler) {
        QCOMPARE(dropped->thread.load(), afterEnd ? QThread::currentThread() : probe->thread.load());
        QCOMPARE(dropped->instances.load(), 1);
    }
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
