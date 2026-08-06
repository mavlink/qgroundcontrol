#include "RemoteIDManagerTest.h"

#include "MAVLinkLib.h"
#include "MockLink.h"
#include "RemoteIDManager.h"
#include "RemoteIDSettings.h"
#include "SettingsManager.h"
#include "Vehicle.h"

namespace {

constexpr const char* kValidFullOperatorID = "FIN87astrdge12k8-xyz";
constexpr const char* kValidPublicOperatorID = "FIN87astrdge12k8";

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

UT_REGISTER_TEST(RemoteIDManagerTest, TestLabel::Integration, TestLabel::Vehicle)
