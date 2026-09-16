#include "FollowMeTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "FollowMe.h"
#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "MultiVehicleManager.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"

void FollowMeTest::_testFollowMe()
{
    // The mock vehicle does not have a follow mode configured, so setFlightMode produces expected warnings.
    ignoreLogMessage("FirmwarePlugin.PX4FirmwarePlugin", QtWarningMsg,
                     QRegularExpression("Unknown flight Mode"));
    ignoreLogMessage("Vehicle.Vehicle", QtWarningMsg,
                     QRegularExpression("setFlightMode failed"));
    FollowMe::instance()->init();
    QGCPositionManager::instance()->init();
    _connectMockLinkNoInitialConnectSequence();
    MultiVehicleManager* vehicleMgr = MultiVehicleManager::instance();
    Vehicle* vehicle = vehicleMgr->activeVehicle();
    vehicle->setFlightMode(vehicle->followFlightMode());
    SettingsManager::instance()->appSettings()->followTarget()->setRawValue(1);
    QSignalSpy spyGCSMotionReport(vehicle, &Vehicle::messagesSentChanged);
    QVERIFY_SIGNAL_WAIT(spyGCSMotionReport, TestTimeout::mediumMs());
    _disconnectMockLink();
}

void FollowMeTest::_motionPolicyReports_data()
{
    QTest::addColumn<double>("speed");
    QTest::addColumn<bool>("hasCourse");
    QTest::addColumn<bool>("hasVerticalAccuracy");
    QTest::addColumn<bool>("hasHorizontalAccuracy");
    QTest::newRow("moving") << 2.0 << true << true << true;
    QTest::newRow("slow") << 0.1 << true << true << true;
    QTest::newRow("no-course") << 2.0 << false << true << true;
    QTest::newRow("no-motion-or-vertical-attributes") << qQNaN() << false << false << true;
    QTest::newRow("no-horizontal-accuracy") << 2.0 << true << true << false;
}

void FollowMeTest::_motionPolicyReports()
{
    QFETCH(double, speed);
    QFETCH(bool, hasCourse);
    QFETCH(bool, hasVerticalAccuracy);
    QFETCH(bool, hasHorizontalAccuracy);
    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(vehicle());
    auto* positioning = QGCPositionManager::instance();
    const auto savedMode = positioning->sourceMode();
    const auto restore = qScopeGuard([&]() { positioning->setSourceMode(savedMode); });
    ManualScheduler scheduler;
    QObject producer;
    GPSSourceHealth health(nullptr, &scheduler);
    auto registration =
        positioning->registerPositionSource(GPSPositionService::SelectedSource::Receiver, &producer, &health, 7);
    positioning->setSourceMode(GPSPositionService::SourceMode::ReceiverOnly);
    GPSObservation observation;
    observation.sessionId = 7;
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), observation.receivedAt);
    if (hasHorizontalAccuracy) {
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    }
    if (hasVerticalAccuracy) {
        observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1);
    }
    if (hasCourse) {
        observation.position.setAttribute(QGeoPositionInfo::Direction, 90);
    }
    if (qIsFinite(speed)) {
        observation.position.setAttribute(QGeoPositionInfo::GroundSpeed, speed);
    }
    health.updateObservation(observation);
    FollowMe follow;
    QVERIFY(QMetaObject::invokeMethod(&follow, "_settingsChanged", Qt::DirectConnection, Q_ARG(QVariant, QVariant(1))));
    QSignalSpy sent(vehicle(), &Vehicle::messagesSentChanged);
    QVERIFY(QMetaObject::invokeMethod(&follow, "_sendGCSMotionReport", Qt::DirectConnection));
    QCOMPARE(sent.size(), hasHorizontalAccuracy ? 1 : 0);
    if (!hasHorizontalAccuracy) {
        return;
    }
    QTRY_VERIFY_WITH_TIMEOUT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_FOLLOW_TARGET) > 0,
                             TestTimeout::mediumMs());
    mavlink_message_t message{};
    QVERIFY(mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_FOLLOW_TARGET, message));
    mavlink_follow_target_t report{};
    mavlink_msg_follow_target_decode(&message, &report);
    QCOMPARE(report.lat, 470000000);
    QCOMPARE(report.lon, 80000000);
    QVERIFY(report.est_capabilities & (1 << FollowMe::POS));
    const bool reliableCourse = hasCourse && speed >= 0.5;
    QCOMPARE(bool(report.est_capabilities & (1 << FollowMe::HEADING)), reliableCourse);
    QCOMPARE(bool(report.est_capabilities & (1 << FollowMe::VEL)), reliableCourse);
    QVERIFY(qAbs(report.vel[0]) < 0.0001);
    QCOMPARE(report.vel[1], reliableCourse ? float(speed) : 0.0f);
    QCOMPARE(report.position_cov[2], hasVerticalAccuracy ? 1.0f : -1.0f);
    if (hasVerticalAccuracy) {
        QCOMPARE(report.alt, 500.0f);
    } else {
        QVERIFY(qIsNaN(report.alt));
    }
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    sent.clear();
    QVERIFY(QMetaObject::invokeMethod(&follow, "_sendGCSMotionReport", Qt::DirectConnection));
    QVERIFY(sent.isEmpty());
}

UT_REGISTER_TEST(FollowMeTest, TestLabel::Integration, TestLabel::Vehicle)
