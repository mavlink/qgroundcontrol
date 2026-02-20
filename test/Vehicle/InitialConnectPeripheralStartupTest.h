#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class InitialConnectPeripheralStartupTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void init() override;
    void _noCameraOrGimbalRequestsBeforeInitialConnectComplete();
};
