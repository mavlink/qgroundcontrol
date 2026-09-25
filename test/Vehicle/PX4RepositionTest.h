#pragma once

#include "BaseClasses/VehicleTest.h"

/// Verifies how MAV_CMD_DO_REPOSITION is sent to PX4 as COMMAND_INT (MockLink advertises
/// MAV_PROTOCOL_CAPABILITY_COMMAND_INT), including the INT32_MAX encoding of "no change" for
/// latitude/longitude.
class PX4RepositionTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit PX4RepositionTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {}

private slots:
    void _noChangeLatLonSentAsInt32Max();
};
