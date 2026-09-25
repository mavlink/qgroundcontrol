#include "MAVLinkSystemIdTest.h"

#include <QtCore/QFile>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "FTPManager.h"
#include "Fact.h"
#include "MAVLinkInspectorController.h"
#include "MAVLinkLib.h"
#include "MAVLinkMessage.h"
#include "MAVLinkProtocol.h"
#include "MAVLinkSystem.h"
#include "MavlinkSettings.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"

void MAVLinkSystemIdTest::_connectWideVehicle_data()
{
    QTest::addColumn<quint32>("systemId");
    QTest::addColumn<quint32>("gcsId");
    QTest::newRow("legacy") << quint32(128) << quint32(255);
    QTest::newRow("wide-vehicle") << quint32(0x80000180) << quint32(255);
    QTest::newRow("wide-gcs") << quint32(128) << quint32(0x800001ff);
    QTest::newRow("both-wide") << quint32(0xffffffff) << quint32(0x800001ff);
}

void MAVLinkSystemIdTest::_connectWideVehicle()
{
    QFETCH(quint32, systemId);
    QFETCH(quint32, gcsId);
    Fact* const gcsSetting = SettingsManager::instance()->mavlinkSettings()->gcsMavlinkSystemID();
    const QVariant oldId = gcsSetting->rawValue();
    const auto restore = qScopeGuard([gcsSetting, oldId]() { gcsSetting->setRawValue(oldId); });
    gcsSetting->setRawValue(gcsId);
    QCOMPARE(MAVLinkProtocol::instance()->getSystemId(), gcsId);

    auto* config = new MockConfiguration(QStringLiteral("32-bit system ID"));
    config->setFirmwareType(MAV_AUTOPILOT_ARDUPILOTMEGA);
    config->setVehicleType(MAV_TYPE_QUADROTOR);
    config->setSystemId(systemId);
    _mockLink = MockLink::startMockLink(config);
    QVERIFY(_mockLink);
    connect(_mockLink, &QObject::destroyed, this, [this]() { _mockLink = nullptr; });

    MultiVehicleManager* const manager = MultiVehicleManager::instance();
    QTRY_VERIFY_WITH_TIMEOUT(manager->getVehicleById(systemId), TestTimeout::longMs());
    _vehicle = manager->getVehicleById(systemId);
    QVERIFY(_vehicle);
    QCOMPARE(_vehicle->id(), systemId);
    QCOMPARE(_vehicle->property("id").toULongLong(), quint64(systemId));
    QVERIFY(waitForInitialConnect());
    QVERIFY(waitForParametersReady());

    _mockLink->clearReceivedMavlinkMessageCounts();
    _vehicle->sendMavCommand(_vehicle->defaultComponentId(), MAV_CMD_COMPONENT_ARM_DISARM, false, 1);
    mavlink_message_t command{};
    QTRY_VERIFY_WITH_TIMEOUT(_mockLink->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_COMMAND_LONG, command),
                             TestTimeout::mediumMs());
    QCOMPARE(command.sysid, gcsId);
    QCOMPARE(mavlink_msg_get_target_sysid(&command, mavlink_get_msg_entry(command.msgid)), systemId);
    QTRY_VERIFY_WITH_TIMEOUT(_vehicle->armed(), TestTimeout::mediumMs());

    _vehicle->parameterManager()->refreshParameter(_vehicle->defaultComponentId(), QStringLiteral("RC1_MIN"));
    mavlink_message_t parameter{};
    QTRY_VERIFY_WITH_TIMEOUT(_mockLink->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_PARAM_REQUEST_READ, parameter),
                             TestTimeout::mediumMs());
    QCOMPARE(parameter.sysid, gcsId);
    QCOMPARE(mavlink_msg_get_target_sysid(&parameter, mavlink_get_msg_entry(parameter.msgid)), systemId);
    ParameterManager* const parameters = _vehicle->parameterManager();
    Fact* const rcMinimum = parameters->getParameter(_vehicle->defaultComponentId(), QStringLiteral("RC1_MIN"));
    QVERIFY(rcMinimum);
    const int newValue = rcMinimum->rawValue().toInt() + 1;
    QSignalSpy parameterWritten(parameters, &ParameterManager::_paramSetSuccess);
    rcMinimum->setRawValue(newValue);
    QTRY_VERIFY_WITH_TIMEOUT(!parameterWritten.isEmpty(), TestTimeout::mediumMs());
    mavlink_message_t parameterSet{};
    QVERIFY(_mockLink->lastReceivedMavlinkMessage(MAVLINK_MSG_ID_PARAM_SET, parameterSet));
    QCOMPARE(mavlink_msg_get_target_sysid(&parameterSet, mavlink_get_msg_entry(parameterSet.msgid)), systemId);
    QCOMPARE(mavlink_msg_param_set_get_param_value(&parameterSet), float(newValue));

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    FTPManager* const ftp = _vehicle->ftpManager();
    QSignalSpy downloaded(ftp, &FTPManager::downloadComplete);
    QVERIFY(ftp->download(MAV_COMP_ID_AUTOPILOT1, QStringLiteral("/general.json"), directory.path()));
    QTRY_COMPARE_WITH_TIMEOUT(downloaded.count(), 1, TestTimeout::longMs());
    QVERIFY2(downloaded.at(0).at(1).toString().isEmpty(), qPrintable(downloaded.at(0).at(1).toString()));
    QVERIFY(QFile::exists(downloaded.at(0).at(0).toString()));
    _mockLink->setCommLost(true);
    int receivedCommands = 0;
    const auto connection = connect(_vehicle, &Vehicle::mavlinkMessageReceived, this,
                                    [&receivedCommands](const mavlink_message_t& message) {
                                        if (message.msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
                                            ++receivedCommands;
                                        }
                                    });
    const auto injectCommand = [&](quint32 target, quint8 component) {
        mavlink_message_t message{};
        mavlink_msg_command_long_pack(systemId, MAV_COMP_ID_AUTOPILOT1, &message, target, component, 65535, 0, 0, 0, 0,
                                      0, 0, 0, 0);
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        const auto length = mavlink_msg_to_send_buffer(buffer, &message);
        MAVLinkProtocol::instance()->receiveBytes(_mockLink, QByteArray(reinterpret_cast<const char*>(buffer), length));
    };
    // A real target-bearing message for another wide ID must not reach Vehicle.
    injectCommand(gcsId ^ 0x40000000U, MAVLinkProtocol::getComponentId());
    QCOMPARE(receivedCommands, 0);
    if (gcsId > 255) {
        injectCommand(gcsId, MAVLinkProtocol::getComponentId() + 1);
        QCOMPARE(receivedCommands, 0);
    }
    injectCommand(gcsId, MAVLinkProtocol::getComponentId());
    QCOMPARE(receivedCommands, 1);
    disconnect(connection);
    _disconnectMockLink();
}

void MAVLinkSystemIdTest::_distinctSystems()
{
    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_mockLink);
    _mockLink->setCommLost(true);
    MAVLinkInspectorController inspector;
    MAVLinkProtocol* const protocol = MAVLinkProtocol::instance();
    protocol->resetMetadataForLink(_mockLink);
    QSignalSpy statusSpy(protocol, &MAVLinkProtocol::mavlinkMessageStatus);

    // Interleave two streams whose IDs share a low byte and have unrelated sequence numbers.
    for (int i = 0; i < 31; ++i) {
        const quint32 systemId = (i % 2) ? 0x80000101U : 1U;
        mavlink_status_t status{};
        status.current_tx_seq = (i % 2 ? 100 : 0) + i / 2;
        mavlink_message_t message{};
        mavlink_msg_attitude_pack_status(systemId, MAV_COMP_ID_AUTOPILOT1, &status, &message, 0, 0, 0, 0, 0, 0, 0);
        uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
        const auto length = mavlink_msg_to_send_buffer(buffer, &message);
        protocol->receiveBytes(_mockLink, QByteArray(reinterpret_cast<const char*>(buffer), length));
    }

    QCOMPARE(inspector.systems()->count(), 2);
    inspector.setActiveSystem(0x80000101U);
    QVERIFY(inspector.activeSystem());
    QCOMPARE(inspector.activeSystem()->id(), quint32(0x80000101));
    const auto* message = inspector.activeSystem()->messages()->value<QGCMAVLinkMessage*>(0);
    QVERIFY(message);
    QCOMPARE(message->sysId(), quint32(0x80000101));
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(statusSpy.at(0).at(3).toULongLong(), quint64(0));
}

UT_REGISTER_TEST(MAVLinkSystemIdTest, TestLabel::Integration, TestLabel::Comms)
