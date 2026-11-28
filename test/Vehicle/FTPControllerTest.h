#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

/// Tests the QML-facing controller of MAVLink FTP against MockLinkFTP
class FTPControllerTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testCancelReportsAbortedOnce);
    void _testVehicleGoneEndsOperation();
    void _testWithoutVehicle();
    void _testNamesRefusedFileName();
};
