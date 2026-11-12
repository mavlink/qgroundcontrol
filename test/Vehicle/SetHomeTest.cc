#include "SetHomeTest.h"

#include <QtCore/QDateTime>
#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoPositionInfo>

#include "BaseClasses/TerrainTest.h"
#include "FlyViewSettings.h"
#include "GPSManager.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "PositionManager.h"
#include "QGCGeoid.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"
#include "Vehicle.h"

namespace {

/// The home update from the ground station's position, turned on, with a device position source of one altitude
/// datum whose fixes the test publishes. The setting and the application's position source are restored at its end.
class GcsHomeUpdate
{
public:
    explicit GcsHomeUpdate(GPSAltitudeDatum datum)
    {
        _updateHomePosition->setRawValue(true);
        _positioning->setConfiguration({.sourceMode = PositionManager::SourceMode::InternalOnly});
        _positioning->setInternalPositionSource(&_source, PositionManager::SourceStatus::WaitingForFix, false, datum);
    }

    ~GcsHomeUpdate()
    {
        _updateHomePosition->setRawValue(_savedUpdateHomePosition);
        _positioning->setInternalPositionSource(nullptr, PositionManager::SourceStatus::NoSource);
        _positioning->setConfiguration(_savedConfiguration);
    }

    GcsHomeUpdate(const GcsHomeUpdate&) = delete;
    GcsHomeUpdate& operator=(const GcsHomeUpdate&) = delete;

    /// Publishes a fix at @a coordinate, which carries the fix's altitude; a fix without one has no vertical accuracy.
    void publish(const QGeoCoordinate& coordinate)
    {
        QGeoPositionInfo fix(coordinate, QDateTime::currentDateTimeUtc());
        fix.setAttribute(QGeoPositionInfo::HorizontalAccuracy, 1);
        if (coordinate.type() == QGeoCoordinate::Coordinate3D) {
            fix.setAttribute(QGeoPositionInfo::VerticalAccuracy, verticalAccuracy);
        }
        _source.publish(fix);
    }

    double verticalAccuracy = 1.0;

private:
    Fact* const _updateHomePosition = SettingsManager::instance()->flyViewSettings()->updateHomePosition();
    PositionManager* const _positioning = GPSManager::instance()->positionManager();
    const QVariant _savedUpdateHomePosition = _updateHomePosition->rawValue();
    const PositionManager::Configuration _savedConfiguration = _positioning->configuration();
    GPSTest::PositionSource _source;
};

/// The MAV_CMD_DO_SET_HOME commands written to a MockLink from now on, each as the message it was sent in. It reads
/// what the link is given to write, so the other commands QGroundControl sends meanwhile, such as message requests,
/// do not hide one, and a command is listed by the time the call that sent it returns.
class SentSetHomeCommands
{
public:
    explicit SentSetHomeCommands(MockLink* link)
        : _connection(QObject::connect(link, &MockLink::writeBytesQueuedSignal, link,
                                       [this](const QByteArray& bytes) { _read(bytes); }))
    {}

    ~SentSetHomeCommands() { QObject::disconnect(_connection); }

    SentSetHomeCommands(const SentSetHomeCommands&) = delete;
    SentSetHomeCommands& operator=(const SentSetHomeCommands&) = delete;

    qsizetype count() const { return asCommandInt.size() + asCommandLong.size(); }

    QList<mavlink_command_int_t> asCommandInt;
    QList<mavlink_command_long_t> asCommandLong;

private:
    void _read(const QByteArray& bytes)
    {
        for (const char byte : bytes) {
            mavlink_message_t message{};
            mavlink_status_t status{};
            if (mavlink_frame_char_buffer(&_frame, &_parser, static_cast<uint8_t>(byte), &message, &status) !=
                MAVLINK_FRAMING_OK) {
                continue;
            }
            if (message.msgid == MAVLINK_MSG_ID_COMMAND_INT) {
                mavlink_command_int_t command{};
                mavlink_msg_command_int_decode(&message, &command);
                if (command.command == MAV_CMD_DO_SET_HOME) {
                    asCommandInt.append(command);
                }
            } else if (message.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
                mavlink_command_long_t command{};
                mavlink_msg_command_long_decode(&message, &command);
                if (command.command == MAV_CMD_DO_SET_HOME) {
                    asCommandLong.append(command);
                }
            }
        }
    }

    mavlink_message_t _frame{};
    mavlink_status_t _parser{};
    const QMetaObject::Connection _connection;
};

/// Checks that @a sent holds one command, a COMMAND_LONG for @a coord at @a amslAltitude.
void verifyOneCommandLong(const SentSetHomeCommands& sent, const QGeoCoordinate& coord, double amslAltitude)
{
    QCOMPARE(sent.asCommandInt.size(), 0);
    QCOMPARE(sent.asCommandLong.size(), 1);
    const mavlink_command_long_t& command = sent.asCommandLong.constFirst();
    QCOMPARE(command.param1, 0.0f);  // use the location in the command, not the current position
    // No yaw: home keeps the vehicle's default heading.
    QVERIFY(qIsNaN(command.param4));
    // COMMAND_LONG carries the location as floats, about half a meter apart at this latitude.
    QCOMPARE(command.param5, static_cast<float>(coord.latitude()));
    QCOMPARE(command.param6, static_cast<float>(coord.longitude()));
    QCOMPARE(command.param7, static_cast<float>(amslAltitude));
}

/// Checks that @a sent holds one command, a COMMAND_INT in MAV_FRAME_GLOBAL for @a coord at @a amslAltitude.
void verifyOneCommandInt(const SentSetHomeCommands& sent, const QGeoCoordinate& coord, double amslAltitude)
{
    QCOMPARE(sent.asCommandLong.size(), 0);
    QCOMPARE(sent.asCommandInt.size(), 1);
    const mavlink_command_int_t& command = sent.asCommandInt.constFirst();
    QCOMPARE(command.frame, static_cast<uint8_t>(MAV_FRAME_GLOBAL));
    QCOMPARE(command.param1, 0.0f);  // use the location in the command, not the current position
    // No yaw: home keeps the vehicle's default heading.
    QVERIFY(qIsNaN(command.param4));
    QCOMPARE(command.x, static_cast<int32_t>(coord.latitude() * 1e7));
    QCOMPARE(command.y, static_cast<int32_t>(coord.longitude() * 1e7));
    QCOMPARE(command.z, static_cast<float>(amslAltitude));
}

}  // namespace

void SetHomeTest::_setHomeHereSendsCommandInt()
{
    QVERIFY(vehicle());

    // Unit tests serve synthetic terrain, so the home altitude is the region's known elevation
    const QGeoCoordinate homeCoord = UnitTestTerrainData::flat10Region.center();

    mockLink()->clearReceivedMavCommandCounts();
    SentSetHomeCommands sent(mockLink());
    vehicle()->doSetHome(homeCoord);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_SET_HOME) >= 1, TestTimeout::longMs());
    verifyOneCommandInt(sent, homeCoord, UnitTestTerrainData::Flat10Region::amslElevation);
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
    GcsHomeUpdate gcs(datum);

    mockLink()->clearReceivedMavCommandCounts();
    SentSetHomeCommands sent(mockLink());
    QGeoCoordinate gcsCoord(47.3977419, 8.5455938);
    if (qIsFinite(verticalAccuracy)) {
        gcsCoord.setAltitude(homeAltitude + 25);
        gcs.verticalAccuracy = verticalAccuracy;
    }
    gcs.publish(gcsCoord);
    // Otherwise home moves horizontally and keeps the altitude the vehicle reports.
    double expectedAltitude = homeAltitude;
    if (movesHomeVertically) {
        // The geoid is about 47.5 m above the ellipsoid here.
        const double separation = QGCGeo::geoidSeparation(gcsCoord.latitude(), gcsCoord.longitude());
        expectedAltitude = gcsCoord.altitude() - (datum == GPSAltitudeDatum::Ellipsoid ? separation : 0.0);
    }
    verifyOneCommandInt(sent, gcsCoord, expectedAltitude);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_SET_HOME) >= 1, TestTimeout::longMs());
    QCOMPARE(mockLink()->receivedMavCommandCount(MAV_CMD_DO_SET_HOME), 1);
}

void SetHomeTest::_gcsPositionUpdateWithoutHomePosition()
{
    QVERIFY(vehicle());
    QVERIFY_TRUE_WAIT(vehicle()->coordinate().isValid(), TestTimeout::shortMs());
    GcsHomeUpdate gcs(GPSAltitudeDatum::MeanSeaLevel);
    // The vehicle as it is before its first HOME_POSITION. MockLink sends one every second, which reaches the vehicle
    // only when the event loop runs again, after this test's checks.
    QGeoCoordinate noHome;
    vehicle()->_setHomePosition(noHome);
    QVERIFY(!vehicle()->homePosition().isValid());
    SentSetHomeCommands sent(mockLink());

    // Neither the ground station nor the vehicle has an altitude, and the command has no value for an unknown one:
    // nothing is sent.
    gcs.publish(QGeoCoordinate(47.3977419, 8.5455938));
    QCOMPARE(sent.count(), 0);

    // The ground station's own altitude needs no home altitude to fall back on.
    const QGeoCoordinate gcsCoord(47.3987419, 8.5465938, 512.5);
    gcs.publish(gcsCoord);
    QCOMPARE(sent.asCommandLong.size(), 0);
    QCOMPARE(sent.asCommandInt.size(), 1);
    const mavlink_command_int_t& command = sent.asCommandInt.constFirst();
    QCOMPARE(command.frame, static_cast<uint8_t>(MAV_FRAME_GLOBAL));
    QCOMPARE(command.x, static_cast<int32_t>(gcsCoord.latitude() * 1e7));
    QCOMPARE(command.y, static_cast<int32_t>(gcsCoord.longitude() * 1e7));
    QCOMPARE(command.z, static_cast<float>(gcsCoord.altitude()));
}

bool SetHomeTest::_reconnectWithoutCommandInt()
{
    _disconnectMockLink();
    if (QTest::currentTestFailed()) {
        return false;
    }
    // MockLink always advertises COMMAND_INT. This one leaves the capability request of the initial connect
    // unanswered, and the test answers it as a vehicle without COMMAND_INT does.
    _mockLink = MockLink::startPX4MockLink(MockConfiguration::OptionNone,
                                           MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost);
    if (!_mockLink) {
        return false;
    }
    (void) connect(_mockLink, &QObject::destroyed, this, [this]() { _mockLink = nullptr; });
    MultiVehicleManager* const vehicles = MultiVehicleManager::instance();
    if (!waitForCondition([vehicles]() { return vehicles->activeVehicle() != nullptr; }, TestTimeout::longMs(),
                          QStringLiteral("vehicle connected"))) {
        return false;
    }
    _vehicle = vehicles->activeVehicle();
    if (!waitForCondition(
            [this]() { return _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_AUTOPILOT_VERSION) > 0; },
            TestTimeout::longMs(), QStringLiteral("AUTOPILOT_VERSION requested"))) {
        return false;
    }
    // The firmware version MockLink reports for PX4, 1.17.0: the firmware plugin warns about one it does not know.
    constexpr uint32_t flightVersion = (1U << 24) | (17U << 16) | FIRMWARE_VERSION_TYPE_OFFICIAL;
    const uint8_t noCustomVersion[8]{};
    mavlink_message_t version{};
    (void) mavlink_msg_autopilot_version_pack_chan(
        _mockLink->vehicleId(), MAV_COMP_ID_AUTOPILOT1, _mockLink->outgoingMavlinkChannel(), &version,
        MAV_PROTOCOL_CAPABILITY_MAVLINK2 | MAV_PROTOCOL_CAPABILITY_MISSION_INT, flightVersion, 0, 0, 0, noCustomVersion,
        noCustomVersion, noCustomVersion, 0, 0, 0, nullptr);
    _mockLink->respondWithMavlinkMessage(version);
    return waitForInitialConnect() && !(_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_COMMAND_INT);
}

void SetHomeTest::_setHomeHereSendsCommandLong()
{
    QVERIFY(_reconnectWithoutCommandInt());
    const QGeoCoordinate homeCoord = UnitTestTerrainData::flat10Region.center();
    SentSetHomeCommands sent(mockLink());

    // MockLink only accepts DO_SET_HOME as COMMAND_INT, and a failed "Set Home Here" is shown.
    expectAppMessage(QRegularExpression(QStringLiteral("MAV_CMD_DO_SET_HOME.*not supported")));
    vehicle()->doSetHome(homeCoord);
    QVERIFY_TRUE_WAIT(sent.count() >= 1, TestTimeout::longMs());
    QVERIFY_TRUE_WAIT(!vehicle()->isMavCommandPending(vehicle()->defaultComponentId(), MAV_CMD_DO_SET_HOME),
                      TestTimeout::longMs());
    verifyExpectedLogMessage();
    verifyOneCommandLong(sent, homeCoord, UnitTestTerrainData::Flat10Region::amslElevation);
}

void SetHomeTest::_gcsPositionUpdateSendsCommandLong()
{
    QVERIFY(_reconnectWithoutCommandInt());
    QVERIFY_TRUE_WAIT(vehicle()->coordinate().isValid(), TestTimeout::shortMs());
    GcsHomeUpdate gcs(GPSAltitudeDatum::MeanSeaLevel);
    SentSetHomeCommands sent(mockLink());

    const QGeoCoordinate gcsCoord(47.3977419, 8.5455938, 512.5);
    gcs.publish(gcsCoord);
    verifyOneCommandLong(sent, gcsCoord, gcsCoord.altitude());
    // MockLink answers that it does not support the command, which a home update does not show.
    QVERIFY_TRUE_WAIT(!vehicle()->isMavCommandPending(vehicle()->defaultComponentId(), MAV_CMD_DO_SET_HOME),
                      TestTimeout::longMs());
}

UT_REGISTER_TEST(SetHomeTest, TestLabel::Integration, TestLabel::Vehicle)
