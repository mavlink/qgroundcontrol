#include "ScheduledTaskTest.h"

#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>

#include <memory>

#include "ManualScheduler.h"
#include "QtRuntimeScheduler.h"
#include "ScheduledTask.h"

void ScheduledTaskTest::_replacementAndReentrantScheduling()
{
    ManualScheduler scheduler;
    QObject context;
    ScheduledTask first(&scheduler, &context);
    ScheduledTask second(&scheduler, &context);
    QList<int> delivered;
    QVERIFY(first.schedule(std::chrono::milliseconds(10), [&]() { delivered.append(-1); }));
    QVERIFY(first.schedule(std::chrono::milliseconds(5), [&]() {
        QVERIFY(!first.active());
        delivered.append(1);
        QVERIFY(first.schedule(std::chrono::milliseconds(5), [&]() { delivered.append(3); }));
    }));
    QVERIFY(second.schedule(std::chrono::milliseconds(10), [&]() { delivered.append(2); }));
    QVERIFY(delivered.isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(10)));
    QCOMPARE(delivered, QList<int>({1, 2, 3}));
    QVERIFY(!first.active());
    QVERIFY(!second.active());
    QVERIFY(first.schedule(std::chrono::microseconds::zero(), [&]() { delivered.append(-1); }));
    first.cancel();
    QVERIFY(scheduler.advanceBy(std::chrono::microseconds::zero()));
    QCOMPARE(delivered, QList<int>({1, 2, 3}));
}

namespace {
void addBackends()
{
    QTest::addColumn<bool>("useQt");
    QTest::newRow("manual") << false;
    QTest::newRow("qt") << true;
}

std::unique_ptr<RuntimeScheduler> makeScheduler(bool useQt)
{
    if (useQt)
        return std::make_unique<QtRuntimeScheduler>();
    return std::make_unique<ManualScheduler>();
}

bool dispatch(RuntimeScheduler* scheduler)
{
    if (auto* manual = qobject_cast<ManualScheduler*>(scheduler)) {
        return manual->advanceBy(std::chrono::seconds(1));
    }
    QTimer marker;
    marker.setSingleShot(true);
    QSignalSpy fired(&marker, &QTimer::timeout);
    marker.start(0);
    return fired.wait(TestTimeout::shortMs());
}
}  // namespace

void ScheduledTaskTest::_dependencyLifetime_data()
{
    QTest::addColumn<bool>("useQt");
    QTest::addColumn<int>("destroyed");
    for (const bool useQt : {false, true}) {
        for (int destroyed = 0; destroyed < 3; ++destroyed) {
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(useQt).arg(destroyed))) << useQt << destroyed;
        }
    }
}

void ScheduledTaskTest::_dependencyLifetime()
{
    QFETCH(bool, useQt);
    QFETCH(int, destroyed);
    auto scheduler = makeScheduler(useQt);
    auto context = std::make_unique<QObject>();
    auto task = std::make_unique<ScheduledTask>(scheduler.get(), context.get());
    bool called = false;
    QVERIFY(task->schedule(std::chrono::microseconds::zero(), [&] { called = true; }));
    if (destroyed == 0)
        task.reset();
    else if (destroyed == 1)
        context.reset();
    else
        scheduler.reset();
    if (task) {
        QVERIFY(!task->active());
        QVERIFY(!task->schedule(std::chrono::microseconds::zero(), [&] { called = true; }));
    }
    QVERIFY(dispatch(scheduler.get()));
    QVERIFY(!called);
}

void ScheduledTaskTest::_cancellationAndReplacement_data()
{
    addBackends();
}

void ScheduledTaskTest::_cancellationAndReplacement()
{
    QFETCH(bool, useQt);
    auto scheduler = makeScheduler(useQt);
    QObject context;
    ScheduledTask task(scheduler.get(), &context);
    int calls = 0;
    bool stale = false;
    QVERIFY(task.schedule(std::chrono::microseconds::zero(), [&] { stale = true; }));
    task.cancel();
    QVERIFY(!task.active());
    QVERIFY(task.schedule(std::chrono::microseconds::zero(), [&] { stale = true; }));
    QVERIFY(task.schedule(std::chrono::microseconds(-1), [&] {
        QVERIFY(!task.active());
        ++calls;
        QVERIFY(task.schedule(std::chrono::microseconds::zero(), [&] { ++calls; }));
    }));
    QCOMPARE(calls, 0);
    QVERIFY(dispatch(scheduler.get()));
    QTRY_COMPARE_WITH_TIMEOUT(calls, 2, TestTimeout::shortMs());
    QVERIFY(!task.active());
    QVERIFY(!stale);
}

void ScheduledTaskTest::_invalidRequests_data()
{
    addBackends();
}

void ScheduledTaskTest::_invalidRequests()
{
    QFETCH(bool, useQt);
    auto scheduler = makeScheduler(useQt);
    QObject context;
    const auto zero = std::chrono::microseconds::zero();
    QCOMPARE(scheduler->schedule(nullptr, zero, [] {}), RuntimeScheduler::TaskId{0});
    QCOMPARE(scheduler->schedule(&context, zero, {}), RuntimeScheduler::TaskId{0});
    QCOMPARE(scheduler->schedule(&context, RuntimeScheduler::MAX_DELAY + std::chrono::microseconds(1), [] {}),
             RuntimeScheduler::TaskId{0});
    const auto maximum = scheduler->schedule(&context, RuntimeScheduler::MAX_DELAY, [] {});
    QVERIFY(maximum);
    scheduler->cancel(maximum);

    bool called = false;
    const auto task = scheduler->schedule(&context, zero, [&] { called = true; });
    RuntimeScheduler::TaskId rejected = 1;
    bool advanced = true;
    auto worker = std::unique_ptr<QThread>(QThread::create([&] {
        rejected = scheduler->schedule(&context, zero, [] {});
        scheduler->cancel(task);
        if (auto* manual = qobject_cast<ManualScheduler*>(scheduler.get())) {
            advanced = manual->advanceBy(std::chrono::seconds(1));
        }
    }));
    worker->start();
    QVERIFY(worker->wait(TestTimeout::shortMs()));
    QCOMPARE(rejected, RuntimeScheduler::TaskId{0});
    if (!useQt)
        QVERIFY(!advanced);
    QVERIFY(dispatch(scheduler.get()));
    QTRY_VERIFY_WITH_TIMEOUT(called, TestTimeout::shortMs());
}

void ScheduledTaskTest::_callbackDestroysDependency_data()
{
    _dependencyLifetime_data();
}

void ScheduledTaskTest::_callbackDestroysDependency()
{
    QFETCH(bool, useQt);
    QFETCH(int, destroyed);
    auto scheduler = makeScheduler(useQt);
    auto context = std::make_unique<QObject>();
    auto task = std::make_unique<ScheduledTask>(scheduler.get(), context.get());
    bool called = false;
    QVERIFY(task->schedule(std::chrono::microseconds::zero(), [&] {
        called = true;
        if (destroyed == 0)
            task.reset();
        else if (destroyed == 1)
            context.reset();
        else
            scheduler.reset();
    }));
    // ManualScheduler returns false when a callback destroys the scheduler itself.
    const bool survived = dispatch(scheduler.get());
    QVERIFY(survived || (!useQt && destroyed == 2));
    QTRY_VERIFY_WITH_TIMEOUT(called, TestTimeout::shortMs());
}

UT_REGISTER_TEST(ScheduledTaskTest, TestLabel::Unit, TestLabel::Utilities)
