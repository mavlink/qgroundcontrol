#include "PX4RepositionTest.h"

#include <QtCore/QtMath>
#include <QtPositioning/QGeoCoordinate>

#include "MockLink.h"
#include "Vehicle.h"

namespace {
constexpr double kAltitudeChange = 5.0;

/// The MAV_CMD_DO_REPOSITION commands written to a MockLink from now on, each as the message it was sent in. It
/// reads what the link is given to write: MockLink keeps only the last COMMAND_INT and the last COMMAND_LONG it
/// received, and QGroundControl may send another one meanwhile, such as a message request.
class SentRepositionCommands
{
public:
    explicit SentRepositionCommands(MockLink* link)
        : _connection(QObject::connect(link, &MockLink::writeBytesQueuedSignal, link,
                                       [this](const QByteArray& bytes) { _read(bytes); }))
    {}

    ~SentRepositionCommands() { QObject::disconnect(_connection); }

    SentRepositionCommands(const SentRepositionCommands&) = delete;
    SentRepositionCommands& operator=(const SentRepositionCommands&) = delete;

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
                if (command.command == MAV_CMD_DO_REPOSITION) {
                    asCommandInt.append(command);
                }
            } else if (message.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
                mavlink_command_long_t command{};
                mavlink_msg_command_long_decode(&message, &command);
                if (command.command == MAV_CMD_DO_REPOSITION) {
                    asCommandLong.append(command);
                }
            }
        }
    }

    mavlink_message_t _frame{};
    mavlink_status_t _parser{};
    const QMetaObject::Connection _connection;
};

/// Checks @a command is a DO_REPOSITION to Hold that keeps the current latitude/longitude
void verifyRepositionCommandInt(const mavlink_command_int_t& command, float yaw, float amslAltitude)
{
    QCOMPARE(command.frame, static_cast<uint8_t>(MAV_FRAME_GLOBAL));
    QCOMPARE(command.param1, -1.0f);  // no ground speed change
    QCOMPARE(command.param2, static_cast<float>(MAV_DO_REPOSITION_FLAGS_CHANGE_MODE));
    QCOMPARE(command.param4, yaw);
    QCOMPARE(command.x, INT32_MAX);
    QCOMPARE(command.y, INT32_MAX);
    QCOMPARE(command.z, amslAltitude);
}
}  // namespace

double PX4RepositionTest::_changedAmslAltitude(double altitudeChange) const
{
    return vehicle()->homePosition().altitude() + vehicle()->altitudeRelative()->rawValue().toDouble() + altitudeChange;
}

void PX4RepositionTest::_noChangeLatLonSentAsInt32Max()
{
    QVERIFY(vehicle());

    mockLink()->clearReceivedMavCommandCounts();
    SentRepositionCommands sent(mockLink());
    vehicle()->sendMavCommandInt(vehicle()->defaultComponentId(), MAV_CMD_DO_REPOSITION, MAV_FRAME_GLOBAL,
                                 false,  // showError
                                 -1.0f, MAV_DO_REPOSITION_FLAGS_CHANGE_MODE, 0.0f, qQNaN(), qQNaN(), qQNaN(), qQNaN());
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 1, TestTimeout::longMs());

    QVERIFY(!sent.asCommandInt.isEmpty());
    const mavlink_command_int_t& command = sent.asCommandInt.constFirst();
    // NaN has no integer form, so COMMAND_INT marks an unused x/y with INT32_MAX
    QCOMPARE(command.x, INT32_MAX);
    QCOMPARE(command.y, INT32_MAX);
    QVERIFY(qIsNaN(command.z));
}

void PX4RepositionTest::_pauseSendsCommandInt()
{
    QVERIFY(vehicle());

    mockLink()->clearReceivedMavCommandCounts();
    SentRepositionCommands sent(mockLink());
    vehicle()->pauseVehicle();
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 1, TestTimeout::longMs());
    QVERIFY(!sent.asCommandInt.isEmpty());
    verifyRepositionCommandInt(sent.asCommandInt.constFirst(), qQNaN(), qQNaN());
}

void PX4RepositionTest::_changeHeadingSendsCommandInt()
{
    QVERIFY(vehicle());
    QVERIFY_TRUE_WAIT(vehicle()->coordinate().isValid(), TestTimeout::shortMs());

    const QGeoCoordinate headingCoord = vehicle()->coordinate().atDistanceAndAzimuth(100, 90);
    const float expectedYaw = qDegreesToRadians(vehicle()->coordinate().azimuthTo(headingCoord));

    mockLink()->clearReceivedMavCommandCounts();
    SentRepositionCommands sent(mockLink());
    vehicle()->guidedModeChangeHeading(headingCoord);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 1, TestTimeout::longMs());
    QVERIFY(!sent.asCommandInt.isEmpty());
    verifyRepositionCommandInt(sent.asCommandInt.constFirst(), expectedYaw, qQNaN());
}

void PX4RepositionTest::_changeAltitudeSendsCommandInt()
{
    QVERIFY(vehicle());
    // Home position arrives on MockLink's first 1Hz tick and is required to compute the AMSL target
    QVERIFY_TRUE_WAIT(vehicle()->homePosition().isValid() && !qIsNaN(vehicle()->homePosition().altitude()),
                      TestTimeout::shortMs());

    const double expectedAltitude = _changedAmslAltitude(kAltitudeChange);

    mockLink()->clearReceivedMavCommandCounts();
    SentRepositionCommands sent(mockLink());
    vehicle()->guidedModeChangeAltitude(kAltitudeChange, false /* pauseVehicle */);
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 1, TestTimeout::longMs());
    QVERIFY(!sent.asCommandInt.isEmpty());
    verifyRepositionCommandInt(sent.asCommandInt.constFirst(), qQNaN(), static_cast<float>(expectedAltitude));
}

void PX4RepositionTest::_changeAltitudeAfterPauseSendsCommandInt()
{
    QVERIFY(vehicle());
    QVERIFY_TRUE_WAIT(vehicle()->homePosition().isValid() && !qIsNaN(vehicle()->homePosition().altitude()),
                      TestTimeout::shortMs());

    const double expectedAltitude = _changedAmslAltitude(kAltitudeChange);

    mockLink()->clearReceivedMavCommandCounts();
    SentRepositionCommands sent(mockLink());
    vehicle()->guidedModeChangeAltitude(kAltitudeChange, true /* pauseVehicle */);
    // The pause is sent first and the altitude change follows once it is accepted
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 2, TestTimeout::longMs());
    QCOMPARE(sent.asCommandLong.size(), 0);
    QVERIFY(sent.asCommandInt.size() >= 2);
    verifyRepositionCommandInt(sent.asCommandInt.constFirst(), qQNaN(), qQNaN());
    verifyRepositionCommandInt(sent.asCommandInt.constLast(), qQNaN(), static_cast<float>(expectedAltitude));
}

void PX4RepositionTest::_pauseBeforePx4_1_12SendsCommandLong()
{
    QVERIFY(vehicle());

    // PX4 1.11 advertises COMMAND_INT, but would read the INT32_MAX latitude/longitude as a position
    vehicle()->setFirmwareVersion(1, 11, 3);

    mockLink()->clearReceivedMavCommandCounts();
    mockLink()->clearReceivedMavlinkMessageCounts();
    SentRepositionCommands sent(mockLink());
    // MockLink only accepts DO_REPOSITION as COMMAND_INT
    expectAppMessage(QRegularExpression(QStringLiteral("MAV_CMD_DO_REPOSITION.*not supported")));
    vehicle()->pauseVehicle();
    QVERIFY_TRUE_WAIT(mockLink()->receivedMavCommandCount(MAV_CMD_DO_REPOSITION) == 1, TestTimeout::longMs());
    QVERIFY_TRUE_WAIT(!vehicle()->isMavCommandPending(vehicle()->defaultComponentId(), MAV_CMD_DO_REPOSITION),
                      TestTimeout::longMs());
    verifyExpectedLogMessage();
    QCOMPARE(mockLink()->receivedMavlinkMessageCount(MAVLINK_MSG_ID_COMMAND_INT), 0);

    QVERIFY(!sent.asCommandLong.isEmpty());
    const mavlink_command_long_t& command = sent.asCommandLong.constFirst();
    QVERIFY(qIsNaN(command.param5));
    QVERIFY(qIsNaN(command.param6));
}

UT_REGISTER_TEST(PX4RepositionTest, TestLabel::Integration, TestLabel::Vehicle)
