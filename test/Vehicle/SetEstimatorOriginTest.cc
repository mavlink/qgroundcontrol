#include "SetEstimatorOriginTest.h"

#include <QtPositioning/QGeoCoordinate>

#include "FirmwarePlugin.h"
#include "MockLink.h"
#include "Vehicle.h"

namespace {
using CommandSupportedResult = FirmwarePluginInstanceData::CommandSupportedResult;

// Arbitrary valid origin (Zurich) used for all cases.
const QGeoCoordinate kOrigin(47.3977419, 8.5455938, 488.0);
}

/// When the command is already known to be unsupported, setEstimatorOrigin must skip the
/// command entirely and send the deprecated SET_GPS_GLOBAL_ORIGIN message directly.
void SetEstimatorOriginTest::_cachedUnsupported_sendsLegacyMessageOnly()
{
    QVERIFY(_vehicle);
    FirmwarePluginInstanceData* instanceData = _vehicle->firmwarePluginInstanceData();
    QVERIFY(instanceData);

    instanceData->setCommandSupported(MAV_CMD_DO_SET_GLOBAL_ORIGIN, CommandSupportedResult::UNSUPPORTED);

    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->clearReceivedMavlinkMessageCounts();
    _vehicle->setEstimatorOrigin(kOrigin);

    QVERIFY_TRUE_WAIT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_SET_GPS_GLOBAL_ORIGIN) == 1,
                      TestTimeout::longMs());
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_DO_SET_GLOBAL_ORIGIN), 0);
}

/// When the command is already known to be supported, setEstimatorOrigin must send it as a
/// COMMAND_INT and must never fall back to the deprecated message (even if the vehicle NAKs).
void SetEstimatorOriginTest::_cachedSupported_sendsCommandIntOnly()
{
    QVERIFY(_vehicle);
    FirmwarePluginInstanceData* instanceData = _vehicle->firmwarePluginInstanceData();
    QVERIFY(instanceData);

    instanceData->setCommandSupported(MAV_CMD_DO_SET_GLOBAL_ORIGIN, CommandSupportedResult::SUPPORTED);

    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->clearReceivedMavlinkMessageCounts();
    _vehicle->setEstimatorOrigin(kOrigin);

    QVERIFY_TRUE_WAIT(_mockLink->receivedMavCommandCount(MAV_CMD_DO_SET_GLOBAL_ORIGIN) == 1,
                      TestTimeout::longMs());
    QCOMPARE(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_SET_GPS_GLOBAL_ORIGIN), 0);
    // The cached-supported path installs no fallback handler, so the vehicle's UNSUPPORTED ack
    // must not re-cache the command or trigger the legacy message.
    QVERIFY(instanceData->getCommandSupported(MAV_CMD_DO_SET_GLOBAL_ORIGIN) == CommandSupportedResult::SUPPORTED);
}

/// With support unknown, setEstimatorOrigin probes with a COMMAND_INT. MockLink NAKs it with
/// MAV_RESULT_UNSUPPORTED, which must trigger the SET_GPS_GLOBAL_ORIGIN fallback and cache the
/// command as unsupported so subsequent calls skip the probe.
void SetEstimatorOriginTest::_probeUnsupported_fallsBackAndCachesUnsupported()
{
    QVERIFY(_vehicle);
    FirmwarePluginInstanceData* instanceData = _vehicle->firmwarePluginInstanceData();
    QVERIFY(instanceData);
    QVERIFY(instanceData->getCommandSupported(MAV_CMD_DO_SET_GLOBAL_ORIGIN) == CommandSupportedResult::UNKNOWN);

    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->clearReceivedMavlinkMessageCounts();
    _vehicle->setEstimatorOrigin(kOrigin);

    // Probe goes out as a COMMAND_INT, then the legacy message is sent once the NAK arrives.
    QVERIFY_TRUE_WAIT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_SET_GPS_GLOBAL_ORIGIN) == 1,
                      TestTimeout::longMs());
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_DO_SET_GLOBAL_ORIGIN), 1);
    QVERIFY(instanceData->getCommandSupported(MAV_CMD_DO_SET_GLOBAL_ORIGIN) == CommandSupportedResult::UNSUPPORTED);

    // Now that it is cached unsupported, a second call must skip the command and go straight to
    // the legacy message.
    _mockLink->clearReceivedMavCommandCounts();
    _mockLink->clearReceivedMavlinkMessageCounts();
    _vehicle->setEstimatorOrigin(kOrigin);

    QVERIFY_TRUE_WAIT(_mockLink->receivedMavlinkMessageCount(MAVLINK_MSG_ID_SET_GPS_GLOBAL_ORIGIN) == 1,
                      TestTimeout::longMs());
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_DO_SET_GLOBAL_ORIGIN), 0);
}

UT_REGISTER_TEST(SetEstimatorOriginTest, TestLabel::Integration, TestLabel::Vehicle)
