#include "GPSSourceHealthTest.h"

#include <QtCore/QScopeGuard>
#include <QtCore/QThread>
#include <QtTest/QSignalSpy>

#include <memory>

#include "GPSSourceHealth.h"
#include "ManualScheduler.h"

namespace {
QGeoPositionInfo position()
{
    QGeoPositionInfo result(QGeoCoordinate(0, 0, 500), QDateTime::fromMSecsSinceEpoch(1000));
    result.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 5);
    result.setAttribute(QGeoPositionInfo::VerticalAccuracy, 2);
    result.setAttribute(QGeoPositionInfo::GroundSpeed, 1);
    result.setAttribute(QGeoPositionInfo::Direction, 360);
    return result;
}

GPSObservation observation(const QGeoPositionInfo& position, const RuntimeScheduler& scheduler, qint64 ageMs = 0)
{
    GPSObservation result;
    result.position = position;
    result.receivedAt = QDateTime::currentDateTimeUtc().addMSecs(-ageMs);
    const quint64 now = scheduler.nowUs();
    result.monotonicTimestampUs = ageMs < 0 ? now + 1000000 : now - static_cast<quint64>(ageMs) * 1000;
    return result;
}
}  // namespace

void GPSSourceHealthTest::_normalizesObservation_data()
{
    QTest::addColumn<QGeoPositionInfo>("fix");
    QTest::addColumn<bool>("usable");
    QTest::addColumn<bool>("altitude");
    QTest::addColumn<bool>("heading");
    QTest::newRow("origin-and-old-receiver-clock") << position() << true << true << true;
    const auto row = [](const char* name, QGeoPositionInfo::Attribute attribute, double value, bool usable,
                        bool altitude, bool heading) {
        auto fix = position();
        fix.setAttribute(attribute, value);
        QTest::newRow(name) << fix << usable << altitude << heading;
    };
    row("inaccurate", QGeoPositionInfo::HorizontalAccuracy, 101, false, false, false);
    row("unknown-accuracy", QGeoPositionInfo::HorizontalAccuracy, qQNaN(), false, false, false);
    row("zero-vertical-accuracy", QGeoPositionInfo::VerticalAccuracy, 0, true, false, true);
    row("negative-vertical-accuracy", QGeoPositionInfo::VerticalAccuracy, -1, true, false, true);
    row("nan-vertical-accuracy", QGeoPositionInfo::VerticalAccuracy, qQNaN(), true, false, true);
    row("poor-vertical-accuracy", QGeoPositionInfo::VerticalAccuracy, 11, true, false, true);
    row("stationary", QGeoPositionInfo::GroundSpeed, 0, true, true, false);
    row("negative-direction-accuracy", QGeoPositionInfo::DirectionAccuracy, -1, true, true, false);
    row("poor-direction-accuracy", QGeoPositionInfo::DirectionAccuracy, 31, true, true, false);
    auto fix = position();
    fix.setCoordinate(QGeoCoordinate(0, 0));
    QTest::newRow("2d-with-vertical-accuracy") << fix << true << false << true;
    fix.removeAttribute(QGeoPositionInfo::HorizontalAccuracy);
    QTest::newRow("missing-accuracy") << fix << false << false << false;
}

void GPSSourceHealthTest::_normalizesObservation()
{
    QFETCH(QGeoPositionInfo, fix);
    QFETCH(bool, usable);
    QFETCH(bool, altitude);
    QFETCH(bool, heading);
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    health.updateObservation(observation(fix, scheduler));
    QCOMPARE(health.usable(), usable);
    QCOMPARE(health.coordinate().isValid(), usable);
    QCOMPARE(health.coordinate().type() == QGeoCoordinate::Coordinate3D, altitude);
    QCOMPARE(qIsFinite(health.observation().heading()), heading);
    if (heading) {
        QCOMPARE(health.observation().heading(), 0);
    }
}

void GPSSourceHealthTest::_ageAndRecovery()
{
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    health._freshnessTimeoutMs = 100;
    QCOMPARE(health.state(), GPSSourceHealth::State::NoData);
    QCOMPARE(scheduler.pendingCount(), 0);
    const auto before = QDateTime::currentDateTimeUtc();
    health.updateObservation(observation(position(), scheduler, 60));
    QVERIFY(health.usable());
    QVERIFY(health.receivedAt() >= before.addMSecs(-60));
    QVERIFY(health.receivedAt() <= QDateTime::currentDateTimeUtc().addMSecs(-60));
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(39)));
    QVERIFY(health.usable());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(health.state(), GPSSourceHealth::State::Stale);
    QVERIFY(!health.coordinate().isValid());
    QVERIFY(qIsNaN(health.horizontalAccuracy()));
    health.updateObservation(observation(position(), scheduler));
    QVERIFY(health.usable());
    health.updateObservation(observation(position(), scheduler, 100));
    QCOMPARE(health.state(), GPSSourceHealth::State::Stale);
    QCOMPARE(scheduler.pendingCount(), 0);
    health.updateObservation(observation(position(), scheduler, -1));
    QCOMPARE(health.state(), GPSSourceHealth::State::Invalid);
    health.updateObservation(observation(position(), scheduler));
    health.invalidatePosition();
    QCOMPARE(health.state(), GPSSourceHealth::State::Invalid);
    health.reset();
    QCOMPARE(health.state(), GPSSourceHealth::State::NoData);
    QVERIFY(!health.receivedAt().isValid());
    QVERIFY(!health.observation().position.isValid());
    QCOMPARE(scheduler.pendingCount(), 0);
}

void GPSSourceHealthTest::_resetDuringPositionNotification()
{
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    QSignalSpy positions(&health, &GPSSourceHealth::positionChanged);
    connect(&health, &GPSSourceHealth::positionChanged, &health, [&]() {
        if (health.usable()) {
            health.reset();
        }
    });
    GPSObservation fix;
    fix.monotonicTimestampUs = scheduler.nowUs();
    fix.position = position();
    health.updateObservation(fix);
    QCOMPARE(health.state(), GPSSourceHealth::State::NoData);
    QCOMPARE(positions.size(), 2);
    QCOMPARE(scheduler.pendingCount(), 0);
    QVERIFY(!health.acceptedObservation());
}

UT_REGISTER_TEST(GPSSourceHealthTest, TestLabel::Unit)

void GPSSourceHealthTest::_retainedMeasurementExpires_data()
{
    QTest::addColumn<QString>("quality");
    for (const auto& quality : {"missing", "poor", "no-fix", "invalidated"}) {
        QTest::newRow(quality) << QString::fromLatin1(quality);
    }
}

void GPSSourceHealthTest::_retainedMeasurementExpires()
{
    QFETCH(QString, quality);
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    health.setFreshnessTimeoutMs(100);
    GPSObservation observation;
    observation.position = position();
    observation.monotonicTimestampUs = scheduler.nowUs();
    if (quality == QStringLiteral("missing")) {
        observation.position.removeAttribute(QGeoPositionInfo::HorizontalAccuracy);
    } else if (quality == QStringLiteral("poor")) {
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 101);
    } else if (quality == QStringLiteral("no-fix")) {
        observation.receiverFixValid = false;
        observation.fixQuality = GPSObservation::FixQuality::NoFix;
    }
    health.updateObservation(observation);
    if (quality == QStringLiteral("invalidated")) {
        health.invalidatePosition();
    }
    QCOMPARE(health.state(), GPSSourceHealth::State::Invalid);
    QVERIFY(!health.acceptedObservation());
    QSignalSpy updates(&health, &GPSSourceHealth::positionChanged);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(99)));
    QVERIFY(updates.isEmpty());
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(health.state(), GPSSourceHealth::State::Stale);
    QCOMPARE(updates.size(), 1);
    QVERIFY(!health.acceptedObservation());
    QCOMPARE(health.observation().position, observation.position);
    QCOMPARE(scheduler.pendingCount(), 0);
}

void GPSSourceHealthTest::_invalidatedPositionTimeout()
{
    ManualScheduler scheduler;
    GPSSourceHealth health(nullptr, &scheduler);
    GPSObservation observation;
    observation.position = position();
    observation.monotonicTimestampUs = scheduler.nowUs();
    health.updateObservation(observation);
    health.invalidatePosition();
    health.setFreshnessTimeoutMs(1000);
    QSignalSpy updates(&health, &GPSSourceHealth::positionChanged);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(999)));
    QCOMPARE(health.state(), GPSSourceHealth::State::Invalid);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(health.state(), GPSSourceHealth::State::Stale);
    QCOMPARE(updates.count(), 1);
    health.setFreshnessTimeoutMs(5000);
    QCOMPARE(health.state(), GPSSourceHealth::State::Stale);
    observation.monotonicTimestampUs = scheduler.nowUs();
    health.updateObservation(observation);
    health.invalidatePosition();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    health.setFreshnessTimeoutMs(1000);
    QCOMPARE(health.state(), GPSSourceHealth::State::Stale);
}

void GPSSourceHealthTest::_schedulerDestructionClearsAcceptedState()
{
    auto scheduler = std::make_unique<ManualScheduler>();
    GPSSourceHealth health(nullptr, scheduler.get());
    auto fix = observation(position(), *scheduler);
    health.updateObservation(fix);
    QVERIFY(health.usable());
    scheduler.reset();
    QCOMPARE(health.state(), GPSSourceHealth::State::NoData);
    QVERIFY(!health.acceptedObservation());
    health.updateObservation(fix);
    QVERIFY(!health.usable());
}

void GPSSourceHealthTest::_foreignSchedulerRejected()
{
    QThread worker;
    ManualScheduler scheduler;
    const auto owner = QThread::currentThread();
    const auto fix = observation(position(), scheduler);
    QVERIFY(scheduler.moveToThread(&worker));
    worker.start();
    const auto cleanup = qScopeGuard([&]() {
        QMetaObject::invokeMethod(&scheduler, [&]() { scheduler.moveToThread(owner); }, Qt::BlockingQueuedConnection);
        worker.quit();
        worker.wait();
    });
    expectLogMessage("GPS.Core.GPSSourceHealth", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Scheduler must share the store thread")));
    GPSSourceHealth health(nullptr, &scheduler);
    verifyExpectedLogMessage();
    health.updateObservation(fix);
    QVERIFY(!health.usable());
    QVERIFY(!health.acceptedObservation());
}
