#pragma once

#include "UnitTest.h"

class GPSPositionFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sharedVehicleObservations();
    void _independentReceiverIntegrityExpiry();
    void _metadataOwnershipAndVirtualIntegrity();
    void _highLatencyTransitions_data();
    void _highLatencyTransitions();
    void _independentIntegrityReports();
    void _integrityProjection();
    void _integrityExpiryAndReentrancy();
    void _vehicleMessages_data();
    void _vehicleMessages();
    void _vehicleSentinels_data();
    void _vehicleSentinels();
    void _localObservation();
    void _resetDuringUpdate();
};
