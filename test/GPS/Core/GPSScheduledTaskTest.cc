#include "GPSScheduledTaskTest.h"

#include <memory>

#include "GPSQtRuntimeScheduler.h"
#include "GPSReplayScheduler.h"
#include "GPSScheduledTask.h"

void GPSScheduledTaskTest::_replacementAndReentrantScheduling()
{
    GPSReplayScheduler scheduler;
    QObject context;
    GPSScheduledTask first(&scheduler, &context);
    GPSScheduledTask second(&scheduler, &context);
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

void GPSScheduledTaskTest::_dependencyLifetime_data()
{
    QTest::addColumn<int>("destroyed");
    QTest::newRow("task") << 0;
    QTest::newRow("context") << 1;
    QTest::newRow("scheduler") << 2;
}

void GPSScheduledTaskTest::_dependencyLifetime()
{
    QFETCH(int, destroyed);
    auto scheduler = std::make_unique<GPSReplayScheduler>();
    auto context = std::make_unique<QObject>();
    auto task = std::make_unique<GPSScheduledTask>(scheduler.get(), context.get());
    bool called = false;
    QVERIFY(task->schedule(std::chrono::milliseconds(1), [&]() { called = true; }));
    if (destroyed == 0) {
        task.reset();
    } else if (destroyed == 1) {
        context.reset();
    } else {
        scheduler.reset();
    }
    if (task) {
        QVERIFY(!task->active());
        QVERIFY(!task->schedule(std::chrono::microseconds::zero(), [&]() { called = true; }));
    }
    if (scheduler) {
        QVERIFY(scheduler->advanceBy(std::chrono::seconds(1)));
    }
    QVERIFY(!called);
}

void GPSScheduledTaskTest::_productionDefersCallbacks()
{
    GPSQtRuntimeScheduler scheduler;
    QObject context;
    GPSScheduledTask task(&scheduler, &context);
    bool called = false;
    QVERIFY(task.schedule(std::chrono::microseconds::zero(), [&]() {
        QVERIFY(!task.active());
        called = true;
    }));
    QVERIFY(!called);
    QTRY_VERIFY_WITH_TIMEOUT(called, TestTimeout::shortMs());
}

UT_REGISTER_TEST(GPSScheduledTaskTest, TestLabel::Unit)
