#include "RemoteIDManagerTest.h"

#include <QtCore/QScopeGuard>

#include "GpsTestHelpers.h"
#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "MockLink.h"
#include "PositionManager.h"
#include "RemoteIDManager.h"
#include "RemoteIDSettings.h"
#include "SettingsManager.h"
#include "Vehicle.h"

namespace {

constexpr const char* kValidFullOperatorID = "FIN87astrdge12k8-xyz";
constexpr const char* kValidPublicOperatorID = "FIN87astrdge12k8";

using GpsTestHelpers::PositionSource;

}  // namespace

void RemoteIDManagerTest::init()
{
    VehicleTestNoInitialConnect::init();

    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    QVERIFY(settings);
    QVERIFY(vehicle());
    QVERIFY(vehicle()->remoteIDManager());

    _savedOperatorIDEU = settings->operatorIDEU()->rawValue();
    _savedOperatorIDFAA = settings->operatorIDFAA()->rawValue();
    _savedOperatorIDType = settings->operatorIDType()->rawValue();
    _savedRegion = settings->region()->rawValue();
    _savedSendOperatorID = settings->sendOperatorID()->rawValue();
    _savedLocationType = settings->locationType()->rawValue();
}

void RemoteIDManagerTest::cleanup()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();
    QVERIFY(settings);

    // Restore region first: its change handler writes sendOperatorID/locationType,
    // which are restored to their saved values afterwards.
    settings->region()->setRawValue(_savedRegion);
    settings->operatorIDType()->setRawValue(_savedOperatorIDType);
    settings->operatorIDEU()->setRawValue(_savedOperatorIDEU);
    settings->operatorIDFAA()->setRawValue(_savedOperatorIDFAA);
    settings->sendOperatorID()->setRawValue(_savedSendOperatorID);
    settings->locationType()->setRawValue(_savedLocationType);

    VehicleTestNoInitialConnect::cleanup();
}

// The basic-ID-missing flag must track the CURRENT arm status error: it is set while the
// RID device reports "missing basic_id message" and cleared when the device moves on to
// failing for a different reason (otherwise the UI keeps blaming basic ID forever)
void RemoteIDManagerTest::_basicIDMissingFlagFollowsArmStatusError()
{
    RemoteIDManager* manager = vehicle()->remoteIDManager();

    mockLink()->setRemoteIDArmStatus(MAV_ODID_ARM_STATUS_PRE_ARM_FAIL_GENERIC,
                                     QStringLiteral("missing basic_id message"));
    QTRY_VERIFY_WITH_TIMEOUT(manager->vehicleReportsBasicIDMissing(), 10000);
    QVERIFY(!manager->armStatusGoodToArm());

    // Different pre-arm failure: basic ID is no longer the reported problem
    mockLink()->setRemoteIDArmStatus(MAV_ODID_ARM_STATUS_PRE_ARM_FAIL_GENERIC,
                                     QStringLiteral("operator location invalid"));
    QTRY_VERIFY_WITH_TIMEOUT(!manager->vehicleReportsBasicIDMissing(), 10000);
    QVERIFY(!manager->armStatusGoodToArm());

    // Device recovers: the stale error text must clear so the UI stops showing it
    mockLink()->setRemoteIDArmStatus(MAV_ODID_ARM_STATUS_GOOD_TO_ARM, QString());
    QTRY_VERIFY_WITH_TIMEOUT(manager->armStatusGoodToArm(), 10000);
    QTRY_VERIFY_WITH_TIMEOUT(manager->armStatusError().isEmpty(), 10000);
}

// Operator ID broadcast starts when the ID is valid and stops when it becomes invalid
void RemoteIDManagerTest::_operatorIDBroadcastGating()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::EU));
    settings->operatorIDType()->setRawValue(0);
    settings->operatorIDEU()->setRawValue(QString::fromLatin1(kValidFullOperatorID));

    // MockLink sends GOOD_TO_ARM at 1Hz which brings comms up and starts the send loop
    QTRY_VERIFY_WITH_TIMEOUT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_OPEN_DRONE_ID_OPERATOR_ID) >= 1, 10000);

    settings->operatorIDEU()->setRawValue(QString());
    QVERIFY(!SettingsManager::instance()->remoteIDSettings()->operatorIDValidForRegion());

    // Wait one SYSTEM tick to drain any in-flight send, then verify OPERATOR_ID stops
    // while SYSTEM (always sent) keeps ticking
    int systemCount = mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM);
    QTRY_VERIFY_WITH_TIMEOUT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM) >= systemCount + 1, 5000);

    const int operatorIDCount = mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_OPEN_DRONE_ID_OPERATOR_ID);
    systemCount = mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM);
    QTRY_VERIFY_WITH_TIMEOUT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM) >= systemCount + 2, 5000);
    QCOMPARE(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_OPEN_DRONE_ID_OPERATOR_ID), operatorIDCount);
}

// The 20-byte operator_id field must be null-padded past the 16-char public ID
void RemoteIDManagerTest::_operatorIDBroadcastNullPadded()
{
    RemoteIDSettings* settings = SettingsManager::instance()->remoteIDSettings();

    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::EU));
    settings->operatorIDType()->setRawValue(0);
    settings->operatorIDEU()->setRawValue(QString::fromLatin1(kValidFullOperatorID));

    QTRY_VERIFY_WITH_TIMEOUT(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_OPEN_DRONE_ID_OPERATOR_ID) >= 1, 10000);

    mavlink_message_t message{};
    QVERIFY(mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_OPEN_DRONE_ID_OPERATOR_ID, message));

    mavlink_open_drone_id_operator_id_t operatorIdMsg{};
    mavlink_msg_open_drone_id_operator_id_decode(&message, &operatorIdMsg);

    const QByteArray expectedPublicID = QByteArray(kValidPublicOperatorID);
    QCOMPARE(QByteArray(operatorIdMsg.operator_id, expectedPublicID.size()), expectedPublicID);
    for (size_t i = expectedPublicID.size(); i < sizeof(operatorIdMsg.operator_id); i++) {
        QCOMPARE(operatorIdMsg.operator_id[i], '\0');
    }
}

void RemoteIDManagerTest::_liveGpsFailureDiagnostics_data()
{
    QTest::addColumn<int>("error");
    QTest::addColumn<GPSPositionService::SourceStatus>("status");
    QTest::addColumn<QString>("diagnostic");
    QTest::newRow("stale") << int(QGeoPositionInfoSource::NoError) << GPSPositionService::SourceStatus::Stale
                           << QStringLiteral("Position data is stale");
    QTest::newRow("inaccurate") << int(QGeoPositionInfoSource::NoError) << GPSPositionService::SourceStatus::InvalidFix
                                << QStringLiteral("Position fix does not meet accuracy requirements");
    QTest::newRow("permission-denied")
        << int(QGeoPositionInfoSource::AccessError) << GPSPositionService::SourceStatus::PermissionDenied
        << QStringLiteral("GCS GPS data error: %1").arg(QGeoPositionInfoSource::AccessError);
    QTest::newRow("backend-unavailable")
        << int(QGeoPositionInfoSource::ClosedError) << GPSPositionService::SourceStatus::BackendUnavailable
        << QStringLiteral("GCS GPS data error: %1").arg(QGeoPositionInfoSource::ClosedError);
}

void RemoteIDManagerTest::_liveGpsFailureDiagnostics()
{
    QFETCH(int, error);
    QFETCH(GPSPositionService::SourceStatus, status);
    QFETCH(QString, diagnostic);

    auto* settings = SettingsManager::instance()->remoteIDSettings();
    auto* manager = vehicle()->remoteIDManager();
    auto* positioning = QGCPositionManager::instance();
    PositionSource source;
    const auto savedMode = positioning->sourceMode();
    const auto restore = qScopeGuard([&]() {
        settings->locationType()->setRawValue(_savedLocationType);
        positioning->setInternalPositionSource(nullptr, GPSPositionService::SourceStatus::NoSource);
        positioning->setSourceMode(savedMode);
    });
    positioning->setSourceMode(GPSPositionService::SourceMode::InternalOnly);
    positioning->setInternalPositionSource(&source, GPSPositionService::SourceStatus::WaitingForFix);
    settings->region()->setRawValue(static_cast<int>(RemoteIDSettings::RegionOperation::FAA));
    settings->locationType()->setRawValue(RemoteIDManager::LocationTypes::LiveGNSS);

    QCOMPARE(positioning->sourceStatus(), GPSPositionService::SourceStatus::WaitingForFix);
    QVERIFY(QMetaObject::invokeMethod(manager, "_sendMessages", Qt::DirectConnection));
    QVERIFY(!manager->gcsPositionUsable());

    QGeoPositionInfo fix(QGeoCoordinate(47, 8, 500), QDateTime::currentDateTimeUtc());
    fix.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    fix.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1);
    source.publish(fix);
    QVERIFY(QMetaObject::invokeMethod(manager, "_sendMessages", Qt::DirectConnection));
    QVERIFY(manager->gcsPositionUsable());

    mavlink_message_t message{};
    mavlink_open_drone_id_system_t system{};
    QTRY_VERIFY_WITH_TIMEOUT(
        ([&]() {
            if (!mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM, message)) {
                return false;
            }
            mavlink_msg_open_drone_id_system_decode(&message, &system);
            return system.operator_latitude == 470000000 && system.operator_longitude == 80000000;
        })(),
        5000);

    expectLogMessage("Vehicle.RemoteIDManager", QtWarningMsg,
                     QRegularExpression(QRegularExpression::escape(diagnostic)));
    if (error != QGeoPositionInfoSource::NoError) {
        source.fail(static_cast<QGeoPositionInfoSource::Error>(error));
    } else if (status == GPSPositionService::SourceStatus::Stale) {
        positioning->sourceHealth()->setFreshnessTimeoutMs(1);
    } else {
        fix.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 101);
        source.publish(fix);
    }
    QTRY_COMPARE_WITH_TIMEOUT(positioning->sourceStatus(), status, 1000);
    QVERIFY(QMetaObject::invokeMethod(manager, "_sendMessages", Qt::DirectConnection));
    verifyExpectedLogMessage();
    QVERIFY(!manager->gcsPositionUsable());
    QVERIFY(!positioning->geoPositionInfo().isValid());
    QVERIFY(!positioning->gcsPositionTimestamp().isValid());

    QTRY_VERIFY_WITH_TIMEOUT(
        ([&]() {
            if (!mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM, message)) {
                return false;
            }
            mavlink_msg_open_drone_id_system_decode(&message, &system);
            return system.operator_latitude == 0 && system.operator_longitude == 0;
        })(),
        5000);

    positioning->sourceHealth()->setFreshnessTimeoutMs(5000);
    fix.setTimestamp(QDateTime::currentDateTimeUtc());
    fix.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    source.publish(fix);
    QVERIFY(QMetaObject::invokeMethod(manager, "_sendMessages", Qt::DirectConnection));
    QVERIFY(manager->gcsPositionUsable());
    QCOMPARE(positioning->gcsPosition(), fix.coordinate());
}

void RemoteIDManagerTest::_gpsAltitudePolicy_data()
{
    QTest::addColumn<bool>("fixed");
    QTest::addColumn<bool>("faa");
    QTest::addColumn<double>("altitude");
    QTest::addColumn<double>("verticalAccuracy");
    QTest::addColumn<double>("ellipsoid");
    QTest::addColumn<bool>("usable");
    QTest::newRow("fixed") << true << true << 500.0 << qQNaN() << qQNaN() << true;
    QTest::newRow("accurate") << false << true << 500.0 << 1.0 << qQNaN() << true;
    QTest::newRow("poor-vertical-accuracy") << false << true << 500.0 << 100.0 << qQNaN() << true;
    QTest::newRow("no-vertical-accuracy") << false << true << 500.0 << qQNaN() << qQNaN() << true;
    QTest::newRow("ellipsoid") << false << true << 500.0 << 100.0 << 550.0 << true;
    QTest::newRow("ellipsoid-only") << false << true << qQNaN() << qQNaN() << 550.0 << true;
    QTest::newRow("faa-missing-altitude") << false << true << qQNaN() << qQNaN() << qQNaN() << false;
    QTest::newRow("eu-missing-altitude") << false << false << qQNaN() << qQNaN() << qQNaN() << true;
}

void RemoteIDManagerTest::_gpsAltitudePolicy()
{
    QFETCH(bool, fixed);
    QFETCH(bool, faa);
    QFETCH(double, altitude);
    QFETCH(double, verticalAccuracy);
    QFETCH(double, ellipsoid);
    QFETCH(bool, usable);
    auto* settings = SettingsManager::instance()->remoteIDSettings();
    auto* manager = vehicle()->remoteIDManager();
    auto* positioning = QGCPositionManager::instance();
    const auto savedMode = positioning->sourceMode();
    const auto savedLatitude = settings->latitudeFixed()->rawValue();
    const auto savedLongitude = settings->longitudeFixed()->rawValue();
    const auto savedAltitude = settings->altitudeFixed()->rawValue();
    const auto restore = qScopeGuard([&]() {
        settings->locationType()->setRawValue(_savedLocationType);
        settings->latitudeFixed()->setRawValue(savedLatitude);
        settings->longitudeFixed()->setRawValue(savedLongitude);
        settings->altitudeFixed()->setRawValue(savedAltitude);
        positioning->setSourceMode(savedMode);
    });
    ManualScheduler scheduler;
    QObject producer;
    GPSSourceHealth health(nullptr, &scheduler);
    auto registration =
        positioning->registerPositionSource(GPSPositionService::SelectedSource::Receiver, &producer, &health, 7);
    positioning->setSourceMode(GPSPositionService::SourceMode::ReceiverOnly);
    settings->region()->setRawValue(
        int(faa ? RemoteIDSettings::RegionOperation::FAA : RemoteIDSettings::RegionOperation::EU));
    settings->locationType()->setRawValue(fixed ? RemoteIDManager::FIXED : RemoteIDManager::LiveGNSS);
    if (fixed) {
        settings->latitudeFixed()->setRawValue(47);
        settings->longitudeFixed()->setRawValue(8);
        settings->altitudeFixed()->setRawValue(altitude);
        QVERIFY(!positioning->acceptedObservation());
    } else {
        GPSObservation observation;
        observation.sessionId = 7;
        observation.receivedAt = QDateTime::currentDateTimeUtc();
        observation.monotonicTimestampUs = scheduler.nowUs();
        observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8, altitude), observation.receivedAt);
        observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
        if (qIsFinite(verticalAccuracy)) {
            observation.position.setAttribute(QGeoPositionInfo::VerticalAccuracy, verticalAccuracy);
        }
        if (qIsFinite(ellipsoid)) {
            observation.altitudeEllipsoidMeters = ellipsoid;
        }
        health.updateObservation(observation);
        QVERIFY(positioning->acceptedObservation(GPSObservation::PositionUse::RemoteID));
        if (!(verticalAccuracy <= 10)) {
            QVERIFY(qIsNaN(positioning->gcsPosition().altitude()));
        }
    }
    if (!usable) {
        expectLogMessage("Vehicle.RemoteIDManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Altitude data is mandatory for FAA regions")));
    }
    QVERIFY(QMetaObject::invokeMethod(manager, "_sendMessages", Qt::DirectConnection));
    QCOMPARE(manager->gcsPositionUsable(), usable);
    if (!usable) {
        verifyExpectedLogMessage();
    }
    const double acceptedAltitude = qIsFinite(ellipsoid) ? ellipsoid : altitude;
    const float expectedAltitude = usable && qIsFinite(acceptedAltitude) ? float(acceptedAltitude) : -1000.0f;
    QTRY_VERIFY_WITH_TIMEOUT(
        ([&]() {
            mavlink_message_t message{};
            if (!mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM, message)) {
                return false;
            }
            mavlink_open_drone_id_system_t system{};
            mavlink_msg_open_drone_id_system_decode(&message, &system);
            return system.operator_latitude == (usable ? 470000000 : 0) &&
                   system.operator_longitude == (usable ? 80000000 : 0) &&
                   system.operator_altitude_geo == expectedAltitude;
        })(),
        TestTimeout::mediumMs());
}

void RemoteIDManagerTest::_liveGpsArrivalBudget_data()
{
    QTest::addColumn<int>("utcJumpSeconds");
    QTest::newRow("steady-utc") << 0;
    QTest::newRow("utc-jumped-forward") << 86400;
    QTest::newRow("utc-jumped-backward") << -86400;
}

void RemoteIDManagerTest::_liveGpsArrivalBudget()
{
    QFETCH(int, utcJumpSeconds);
    auto* settings = SettingsManager::instance()->remoteIDSettings();
    auto* manager = vehicle()->remoteIDManager();
    auto* positioning = QGCPositionManager::instance();
    const auto savedMode = positioning->sourceMode();
    const auto restore = qScopeGuard([&]() {
        settings->locationType()->setRawValue(_savedLocationType);
        positioning->setSourceMode(savedMode);
    });
    ManualScheduler scheduler;
    QObject producer;
    GPSSourceHealth health(nullptr, &scheduler);
    health.setFreshnessTimeoutMs(60000);
    auto registration =
        positioning->registerPositionSource(GPSPositionService::SelectedSource::Receiver, &producer, &health);
    positioning->setSourceMode(GPSPositionService::SourceMode::ReceiverOnly);
    settings->region()->setRawValue(int(RemoteIDSettings::RegionOperation::FAA));
    settings->locationType()->setRawValue(RemoteIDManager::LiveGNSS);
    GPSObservation observation;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = scheduler.nowUs();
    observation.position = QGeoPositionInfo(QGeoCoordinate(47, 8, 500), observation.receivedAt);
    observation.position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    health.updateObservation(observation);
    QVERIFY(QMetaObject::invokeMethod(manager, "_sendMessages", Qt::DirectConnection));
    QVERIFY(manager->gcsPositionUsable());

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds{4999}));
    observation.receivedAt = observation.receivedAt.addSecs(utcJumpSeconds);
    observation.position.setTimestamp(observation.receivedAt);
    health.updateObservation(observation);
    QVERIFY(QMetaObject::invokeMethod(manager, "_sendMessages", Qt::DirectConnection));
    QVERIFY(manager->gcsPositionUsable());

    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds{1}));
    QVERIFY(positioning->acceptedObservation(GPSObservation::PositionUse::RemoteID));
    QVERIFY(!positioning->acceptedObservation(GPSObservation::PositionUse::RemoteID, std::chrono::milliseconds{5000}));
    expectLogMessage("Vehicle.RemoteIDManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("GCS GPS data is not valid")));
    QVERIFY(QMetaObject::invokeMethod(manager, "_sendMessages", Qt::DirectConnection));
    verifyExpectedLogMessage();
    QVERIFY(!manager->gcsPositionUsable());
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.position.setTimestamp(observation.receivedAt);
    observation.monotonicTimestampUs = scheduler.nowUs();
    health.updateObservation(observation);
    QVERIFY(QMetaObject::invokeMethod(manager, "_sendMessages", Qt::DirectConnection));
    QVERIFY(manager->gcsPositionUsable());
}

UT_REGISTER_TEST(RemoteIDManagerTest, TestLabel::Integration, TestLabel::Vehicle)
