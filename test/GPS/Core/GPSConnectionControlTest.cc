#include "GPSConnectionControlTest.h"

#include <memory>

#include "GPSConnectionControl.h"
#include "GPSReplayScheduler.h"

void GPSConnectionControlTest::_reentrantProfileReplacement_data()
{
    QTest::addColumn<bool>("deferred");
    QTest::newRow("native-immediate") << false;
    QTest::newRow("passive-after-commands") << true;
}

void GPSConnectionControlTest::_reentrantProfileReplacement()
{
    QFETCH(bool, deferred);
    using Policy = GPSConnectionControl::NotificationPolicy;
    GPSReplayScheduler scheduler;
    GPSReceiverProfile first;
    first.endpoint = {.kind = GPSReceiverProfile::Endpoint::Kind::Tcp, .host = "localhost", .port = 1234};
    auto second = first;
    second.endpoint.port = 5678;
    GPSConnectionControl control(deferred ? Policy::AfterCommands : Policy::Immediate, nullptr, &scheduler, first);
    control.changeAutomatic(true);
    control.connection().updateIntent(true);
    bool replaced = false;
    int admissions = 0;
    connect(&control, &GPSConnectionControl::changed, &control, [&]() {
        if (!replaced && control.connection().state() != GPSConnectionState::Disconnected) {
            replaced = true;
            control.dispatch([&]() {
                control.changeProfile(second);
                control.connection().stop();
                control.connection().stopped();
                control.changeSuspended(true);
            });
        }
    });
    control.dispatch([&]() {
        control.startAttempt([&]() {
            ++admissions;
            control.connection().ready();
            return true;
        });
    });
    QVERIFY(replaced);
    QCOMPARE(admissions, deferred ? 1 : 0);
    QCOMPARE(control.profile(), second);
    QCOMPARE(control.connection().state(), GPSConnectionState::Disconnected);
    QVERIFY(!control.connection().active());
    QVERIFY(control.suspended());
    int updates = 0;
    control.scheduleUpdate(true, -1, false, [&]() { ++updates; });
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(updates, 0);
}

void GPSConnectionControlTest::_retryScheduling_data()
{
    _reentrantProfileReplacement_data();
}

void GPSConnectionControlTest::_retryScheduling()
{
    QFETCH(bool, deferred);
    using Policy = GPSConnectionControl::NotificationPolicy;
    GPSReplayScheduler scheduler;
    GPSReceiverProfile profile;
    profile.endpoint = {.kind = GPSReceiverProfile::Endpoint::Kind::Tcp, .host = "localhost", .port = 1234};
    GPSConnectionControl control(deferred ? Policy::AfterCommands : Policy::Immediate, nullptr, &scheduler, profile);
    control.changeAutomatic(true);
    control.connection().updateIntent(true);
    QVERIFY(control.startAttempt([]() { return true; }));
    control.connection().failed();
    int updates = 0;
    control.scheduleUpdate(true, -1, false, [&]() { ++updates; });
    control.scheduleUpdate(true, -1, false, [&]() { ++updates; });
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(999)));
    QCOMPARE(updates, 0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(updates, 1);
    QVERIFY(control.startAttempt([]() { return true; }));
    control.connection().failed();
    control.scheduleUpdate(true, -1, false, [&]() { ++updates; });
    control.connection().pause();
    control.scheduleUpdate(true, -1, false, [&]() { ++updates; });
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(10)));
    QCOMPARE(updates, 1);
}

void GPSConnectionControlTest::_queuedCommandsRetireWithOwner()
{
    GPSReplayScheduler scheduler;
    auto control = std::make_unique<GPSConnectionControl>(GPSConnectionControl::NotificationPolicy::AfterCommands,
                                                          nullptr, &scheduler);
    bool staleCommandRan = false;
    control->dispatch([&]() {
        control->dispatch([&]() { control.reset(); });
        control->dispatch([&]() { staleCommandRan = true; });
    });
    QVERIFY(!control);
    QVERIFY(!staleCommandRan);
}

UT_REGISTER_TEST(GPSConnectionControlTest, TestLabel::Unit)
