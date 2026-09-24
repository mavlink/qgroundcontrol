#include "SetHomeTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>

#include "BaseClasses/TerrainTest.h"
#include "FlyViewSettings.h"
#include "GpsTestHelpers.h"
#include "MockLink.h"
#include "PositionManager.h"
#include "SettingsManager.h"
#include "Vehicle.h"

void SetHomeTest::_verifySetHomeCommandInt(const QGeoCoordinate& coord, double amslAltitude)
{
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_SET_HOME) >= 1, TestTimeout::longMs());

    mavlink_message_t message{};
    QVERIFY(mockLink()->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_COMMAND_INT, message));
    mavlink_command_int_t command{};
    mavlink_msg_command_int_decode(&message, &command);
    QCOMPARE(command.command, static_cast<uint16_t>(MAV_CMD_DO_SET_HOME));
    QCOMPARE(command.frame, static_cast<uint8_t>(MAV_FRAME_GLOBAL));
    QCOMPARE(command.param1, 0.0f);  // use the location in the command, not the current position
    QCOMPARE(command.x, static_cast<int32_t>(coord.latitude() * 1e7));
    QCOMPARE(command.y, static_cast<int32_t>(coord.longitude() * 1e7));
    QCOMPARE(command.z, static_cast<float>(amslAltitude));
}

void SetHomeTest::_setHomeHereSendsCommandInt()
{
    QVERIFY(vehicle());

    // Unit tests serve synthetic terrain, so the home altitude is the region's known elevation
    const QGeoCoordinate homeCoord = UnitTestTerrainData::flat10Region.center();

    mockLink()->clearReceivedMavCommandCounts();
    vehicle()->doSetHome(homeCoord);
    _verifySetHomeCommandInt(homeCoord, UnitTestTerrainData::Flat10Region::amslElevation);
}

void SetHomeTest::_gcsPositionUpdateSendsCommandInt()
{
    QVERIFY(vehicle());
    QVERIFY_TRUE_WAIT(vehicle()->coordinate().isValid(), TestTimeout::shortMs());

    Fact* const updateHomePosition = SettingsManager::instance()->flyViewSettings()->updateHomePosition();
    auto* const positioning = QGCPositionManager::instance();
    GpsTestHelpers::PositionSource source;
    const QVariant savedUpdateHomePosition = updateHomePosition->rawValue();
    const auto savedMode = positioning->sourceMode();
    const auto restore = qScopeGuard([&]() {
        updateHomePosition->setRawValue(savedUpdateHomePosition);
        positioning->setInternalPositionSource(nullptr, GPSPositionService::SourceStatus::NoSource);
        positioning->setSourceMode(savedMode);
    });
    updateHomePosition->setRawValue(true);
    positioning->setSourceMode(GPSPositionService::SourceMode::InternalOnly);
    positioning->setInternalPositionSource(&source, GPSPositionService::SourceStatus::WaitingForFix);

    mockLink()->clearReceivedMavCommandCounts();
    const QGeoCoordinate gcsCoord(47.3977419, 8.5455938, 488.0);
    QGeoPositionInfo fix(gcsCoord, QDateTime::currentDateTimeUtc());
    fix.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    fix.setAttribute(QGeoPositionInfo::VerticalAccuracy, 1);
    source.publish(fix);
    _verifySetHomeCommandInt(gcsCoord, gcsCoord.altitude());
}

UT_REGISTER_TEST(SetHomeTest, TestLabel::Integration, TestLabel::Vehicle)
