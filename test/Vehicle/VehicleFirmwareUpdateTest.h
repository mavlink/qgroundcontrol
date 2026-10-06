#pragma once

#include "BaseClasses/CommsTest.h"

class VehicleFirmwareUpdateTest : public CommsTest
{
    Q_OBJECT

private slots:
    void _acknowledgementPersistsUntilNewerRelease();
    void _acknowledgementAppliesToMatchingConnectedVehicles();
};
