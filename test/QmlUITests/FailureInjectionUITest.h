#pragma once

#include "VehicleConfigUITestBase.h"

/// UI test that boots the full QML UI with a PX4 MockLink vehicle connected,
/// navigates to the Failure Injection setup page and drives an injection and a
/// reset through the real UI controls. MockLink doesn't special-case
/// MAV_CMD_INJECT_FAILURE, so it acks MAV_RESULT_UNSUPPORTED for every send —
/// this test exercises the send/ack/log round trip, not PX4 behavior.
class FailureInjectionUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

public:
    FailureInjectionUITest() = default;

private slots:
    void _testInjectAndReset();
};
