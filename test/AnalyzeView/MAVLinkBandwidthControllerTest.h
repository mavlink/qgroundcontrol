#pragma once

#include "BaseClasses/VehicleTest.h"

class MAVLinkBandwidthControllerTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _firmwareModeAvailabilityTest();
    void _scriptInstallAndRebootTest();
    void _mavftpRoundTripTest();
    void _mavftpCancelRestartTest();
    void _mavftpControllerDestructionCleanupTest();
};
