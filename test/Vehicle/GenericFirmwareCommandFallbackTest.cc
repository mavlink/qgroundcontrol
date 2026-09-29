#include "GenericFirmwareCommandFallbackTest.h"

#include "MockLink.h"
#include "Vehicle.h"

/// MockLink NAKs MAV_CMD_DO_SET_MISSION_CURRENT with MAV_RESULT_UNSUPPORTED, so the probe must be
/// followed by the deprecated MISSION_SET_CURRENT message rather than a crash on the missing cache.
void GenericFirmwareCommandFallbackTest::_setCurrentMissionSequence_probesAndFallsBack()
{
    // Generic firmware has no metadata source; this connect-time warning is unrelated to the behavior under test
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression("failed to load metadata"));
    _connectMockLink(MAV_AUTOPILOT_GENERIC);
    QVERIFY(_vehicle);
    QVERIFY(!_vehicle->firmwarePluginInstanceData());

    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->clearReceivedMavlinkMessageCounts();
    _vehicle->setCurrentMissionSequence(1);

    QVERIFY_TRUE_WAIT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_MISSION_SET_CURRENT) == 1,
                      TestTimeout::longMs());
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_DO_SET_MISSION_CURRENT), 1);
}

UT_REGISTER_TEST(GenericFirmwareCommandFallbackTest, TestLabel::Integration, TestLabel::Vehicle)
