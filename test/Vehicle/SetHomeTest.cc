#include "SetHomeTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QScopeGuard>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>

#include "BaseClasses/TerrainTest.h"
#include "FlyViewSettings.h"
#include "GPSManager.h"
#include "MockLink.h"
#include "PositionManager.h"
#include "QGCGeoid.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"
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

void SetHomeTest::_gcsPositionUpdateSendsCommandInt_data()
{
    QTest::addColumn<GPSAltitudeDatum>("datum");
    // NaN for a position without an altitude.
    QTest::addColumn<double>("verticalAccuracy");
    // MAV_FRAME_GLOBAL altitudes are MSL, so only an altitude known in that datum becomes the home altitude.
    QTest::addColumn<bool>("movesHomeVertically");
    QTest::newRow("mean-sea-level") << GPSAltitudeDatum::MeanSeaLevel << 1.0 << true;
    // An ellipsoid height, as Android reports, is converted with the geoid.
    QTest::newRow("ellipsoid") << GPSAltitudeDatum::Ellipsoid << 1.0 << true;
    QTest::newRow("inaccurate-ellipsoid") << GPSAltitudeDatum::Ellipsoid << 11.0 << false;
    QTest::newRow("unknown-datum") << GPSAltitudeDatum::Unknown << 1.0 << false;
    // The command has no value for an unknown altitude.
    QTest::newRow("no-altitude") << GPSAltitudeDatum::MeanSeaLevel << qQNaN() << false;
}

void SetHomeTest::_gcsPositionUpdateSendsCommandInt()
{
    QFETCH(GPSAltitudeDatum, datum);
    QFETCH(double, verticalAccuracy);
    QFETCH(bool, movesHomeVertically);
    QVERIFY(vehicle());
    QVERIFY_TRUE_WAIT(vehicle()->coordinate().isValid(), TestTimeout::shortMs());
    QVERIFY_TRUE_WAIT(qIsFinite(vehicle()->homePosition().altitude()), TestTimeout::shortMs());
    const double homeAltitude = vehicle()->homePosition().altitude();

    Fact* const updateHomePosition = SettingsManager::instance()->flyViewSettings()->updateHomePosition();
    auto* const positioning = GPSManager::instance()->positionManager();
    GPSTest::PositionSource source;
    const QVariant savedUpdateHomePosition = updateHomePosition->rawValue();
    const auto savedConfiguration = positioning->configuration();
    const auto restore = qScopeGuard([&]() {
        updateHomePosition->setRawValue(savedUpdateHomePosition);
        positioning->setInternalPositionSource(nullptr, PositionManager::SourceStatus::NoSource);
        positioning->setConfiguration(savedConfiguration);
    });
    updateHomePosition->setRawValue(true);
    positioning->setConfiguration({.sourceMode = PositionManager::SourceMode::InternalOnly});
    positioning->setInternalPositionSource(&source, PositionManager::SourceStatus::WaitingForFix, false, datum);

    mockLink()->clearReceivedMavCommandCounts();
    QGeoCoordinate gcsCoord(47.3977419, 8.5455938);
    QGeoPositionInfo fix(gcsCoord, QDateTime::currentDateTimeUtc());
    fix.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
    if (qIsFinite(verticalAccuracy)) {
        gcsCoord.setAltitude(homeAltitude + 25);
        fix.setCoordinate(gcsCoord);
        fix.setAttribute(QGeoPositionInfo::VerticalAccuracy, verticalAccuracy);
    }
    source.publish(fix);
    // Otherwise home moves horizontally and keeps the altitude the vehicle reports.
    double expectedAltitude = homeAltitude;
    if (movesHomeVertically) {
        // The geoid is about 47.5 m above the ellipsoid here.
        const double separation = QGCGeo::geoidSeparation(gcsCoord.latitude(), gcsCoord.longitude());
        expectedAltitude = gcsCoord.altitude() - (datum == GPSAltitudeDatum::Ellipsoid ? separation : 0.0);
    }
    _verifySetHomeCommandInt(gcsCoord, expectedAltitude);
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_DO_SET_HOME), 1);
}

UT_REGISTER_TEST(SetHomeTest, TestLabel::Integration, TestLabel::Vehicle)
