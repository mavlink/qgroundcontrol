#pragma once

#include "UnitTest.h"

class GPSManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _invalidEndpoint_data();
    void _invalidEndpoint();
    void _networkRecoveryAndDisconnect();
    void _networkStartupAndPause();
    void _suspendedConnections();
    void _serialDiscoveryPausesForNetwork();
    void _rtkConnectionSelectionMigration();
    void _nmeaAndRtkIndependent();
    void _networkSettingsPanel();
};
