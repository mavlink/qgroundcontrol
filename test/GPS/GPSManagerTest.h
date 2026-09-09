#pragma once

#include "UnitTest.h"

class GPSManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _correctionRoutingSettings_data();
    void _correctionRoutingSettings();
    void _correctionRuntimeLifecycle();
    void _correctionSettingsPanel();
    void _correctionDiagnosticsPanel();
    void _receiverConfigurationPanel_data();
    void _receiverConfigurationPanel();
    void _invalidEndpoint_data();
    void _invalidEndpoint();
    void _networkRecoveryAndDisconnect();
    void _udpRecoveryAndSelection();
    void _networkStartupAndPause();
    void _suspendedConnections();
    void _serialDiscoveryPausesForNetwork();
    void _rtkConnectionSelectionMigration();
    void _nmeaAndRtkIndependent();
    void _networkSettingsPanel_data();
    void _networkSettingsPanel();
};
