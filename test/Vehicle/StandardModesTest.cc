#include "StandardModesTest.h"

#include "QGCMAVLink.h"
#include "Vehicle.h"

void StandardModesTest::_monitorSequenceBumpTriggersRequery()
{
    // Initial connect has completed, so the initial AVAILABLE_MODES enumeration has settled.
    // Assert on the re-query itself rather than on flightModes(), which a FirmwarePlugin can
    // filter (e.g. custom builds hide modes that cannot be set by the user).
    const int baselineRequests = _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_AVAILABLE_MODES);
    QVERIFY(baselineRequests > 0);

    // Bumping the sequence number changes the 1Hz AVAILABLE_MODES_MONITOR, which must cause
    // StandardModes to re-query the mode list.
    _mockLink->bumpAvailableModesMonitorSequence();

    QTRY_VERIFY_WITH_TIMEOUT(
        _mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_AVAILABLE_MODES) > baselineRequests,
        TestTimeout::longMs());
}

UT_REGISTER_TEST(StandardModesTest, TestLabel::Integration, TestLabel::Vehicle)
