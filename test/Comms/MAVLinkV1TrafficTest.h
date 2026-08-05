#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

/// Tests handling of MAVLink v1 traffic.
///
/// ArduPilot starts each link in MAVLink v1 and only upgrades to v2 once it receives a v2
/// message from the GCS. QGC must tolerate that handshake without reporting the
/// "only supports MAVLink v2" warning, while still reporting vehicles which never upgrade.
/// RADIO_STATUS is exempt since SiK radios always frame it as v1.
class MAVLinkV1TrafficTest : public VehicleTestManualConnect
{
    Q_OBJECT

protected slots:
    void cleanup() override;

private slots:
    void _testV1UpgradeToV2NoWarning();
    void _testV1OnlyVehicleWarns();
    void _testV1RadioStatusDoesNotWarn();
};
