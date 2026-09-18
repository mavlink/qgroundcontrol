#include "FollowMeTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtTest/QSignalSpy>

#include "AppSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "FollowMe.h"
#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "MultiVehicleManager.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"

void FollowMeTest::_testFollowMe()
{
    TestFixtures::SettingsFixture saved;
    // MockLink has no configured follow mode.
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
    saved.setFactValue(SettingsManager::instance()->appSettings()->followTarget(), 1);
    QSignalSpy spyGCSMotionReport(vehicle, &Vehicle::messagesSentChanged);
    QVERIFY_SIGNAL_WAIT(spyGCSMotionReport, TestTimeout::mediumMs());
    _disconnectMockLink();
}

void FollowMeTest::_motionPolicyReports_data()
{
    QTest::addColumn<double>("speed");
    QTest::addColumn<bool>("hasCourse");
    QTest::addColumn<double>("verticalAccuracy");
    QTest::addColumn<double>("altitude");
    QTest::addColumn<bool>("hasHorizontalAccuracy");
    QTest::addColumn<bool>("hasPreviousReport");
    QTest::addColumn<bool>("ardupilot");
    QTest::newRow("moving") << 2.0 << true << 1.0 << 500.0 << true << false << false;
    QTest::newRow("moving-after-previous-report") << 2.0 << true << 1.0 << 500.0 << true << true << false;
    QTest::newRow("slow") << 0.1 << true << 1.0 << 500.0 << true << false << false;
    QTest::newRow("no-course") << 2.0 << false << 1.0 << 500.0 << true << false << false;
    QTest::newRow("no-motion-or-vertical-attributes") << qQNaN() << false << qQNaN() << 500.0 << true << false << false;
    QTest::newRow("uncertain-altitude") << 2.0 << true << 11.0 << 500.0 << true << false << false;
    QTest::newRow("uncertain-altitude-after-report") << 2.0 << true << 11.0 << 500.0 << true << true << false;
    QTest::newRow("missing-vertical-accuracy-after-report") << 2.0 << true << qQNaN() << 500.0 << true << true << false;
    QTest::newRow("missing-altitude") << 2.0 << true << 1.0 << qQNaN() << true << false << false;
    QTest::newRow("missing-altitude-after-report") << 2.0 << true << 1.0 << qQNaN() << true << true << false;
    QTest::newRow("infinite-altitude") << 2.0 << true << 1.0 << qInf() << true << false << false;
    QTest::newRow("no-horizontal-accuracy") << 2.0 << true << 1.0 << 500.0 << false << false << false;
    QTest::newRow("ardupilot-home-altitude") << 2.0 << true << 1.0 << 500.0 << true << false << true;
    QTest::newRow("ardupilot-2d-after-report") << 2.0 << true << qQNaN() << qQNaN() << true << true << true;
}

void FollowMeTest::_motionPolicyReports()
{
    QFETCH(double, speed);
    QFETCH(bool, hasCourse);
    QFETCH(double, verticalAccuracy);
    QFETCH(double, altitude);
    QFETCH(bool, hasHorizontalAccuracy);
    QFETCH(bool, hasPreviousReport);
    QFETCH(bool, ardupilot);
    if (ardupilot && !apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->appSettings()->followTarget(), 0);
    if (ardupilot) {
        _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    } else {
        _connectMockLinkNoInitialConnectSequence();
    }
    QVERIFY(vehicle());
    if (ardupilot) {
        QTRY_VERIFY_WITH_TIMEOUT(vehicle()->homePosition().isValid() && qIsFinite(vehicle()->homePosition().altitude()),
                                 TestTimeout::mediumMs());
    }
    const uint32_t messageId = ardupilot ? MAVLINK_MSG_ID_GLOBAL_POSITION_INT : MAVLINK_MSG_ID_FOLLOW_TARGET;
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
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8, altitude), observation.receivedAt);
    if (hasHorizontalAccuracy) {
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    }
    if (qIsFinite(verticalAccuracy)) {
        observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, verticalAccuracy);
    }
    if (hasCourse) {
        observation.position.setAttribute(QGeoPositionInfo::Direction, 90);
    }
    if (qIsFinite(speed)) {
        observation.position.setAttribute(QGeoPositionInfo::GroundSpeed, speed);
    }
    FollowMe follow;
    QVERIFY(QMetaObject::invokeMethod(&follow, "_settingsChanged", Qt::DirectConnection, Q_ARG(QVariant, QVariant(1))));
    if (hasPreviousReport) {
        auto previousObservation = observation;
        previousObservation.position.setCoordinate(QGeoCoordinate(46, 7, 400));
        health.updateObservation(previousObservation);
        QVERIFY(QMetaObject::invokeMethod(&follow, "_sendGCSMotionReport", Qt::DirectConnection));
        QTRY_COMPARE_WITH_TIMEOUT(mockLink()->receivedMavlinkMessageCount(messageId), 1, TestTimeout::mediumMs());
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        observation.monotonicTimestampUs = scheduler.nowUs();
    }
    health.updateObservation(observation);
    const int previousReportCount = mockLink()->receivedMavlinkMessageCount(messageId);
    QSignalSpy sent(vehicle(), &Vehicle::messagesSentChanged);
    QVERIFY(QMetaObject::invokeMethod(&follow, "_sendGCSMotionReport", Qt::DirectConnection));
    const bool expectedReport = hasHorizontalAccuracy && (ardupilot || qIsFinite(altitude));
    QCOMPARE(sent.size(), expectedReport ? 1 : 0);
    if (!expectedReport) {
        return;
    }
    QTRY_COMPARE_WITH_TIMEOUT(mockLink()->receivedMavlinkMessageCount(messageId), previousReportCount + 1,
                              TestTimeout::mediumMs());
    mavlink_message_t message{};
    QVERIFY(mockLink()->lastReceivedMavlinkMessage(messageId, message));
    if (ardupilot) {
        mavlink_global_position_int_t report{};
        mavlink_msg_global_position_int_decode(&message, &report);
        QCOMPARE(report.lat, 470000000);
        QCOMPARE(report.lon, 80000000);
        QCOMPARE(report.alt, static_cast<int32_t>(vehicle()->homePosition().altitude() * 1000));
    } else {
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
        QCOMPARE(report.position_cov[2], qIsFinite(verticalAccuracy) ? float(verticalAccuracy) : 0.0f);
        QCOMPARE(report.alt, float(altitude));
    }
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(5)));
    sent.clear();
    QVERIFY(QMetaObject::invokeMethod(&follow, "_sendGCSMotionReport", Qt::DirectConnection));
    QVERIFY(sent.isEmpty());
}

UT_REGISTER_TEST(FollowMeTest, TestLabel::Integration, TestLabel::Vehicle)
