#pragma once

#include "UnitTest.h"

class RTKConnectionPolicyTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _manualRetryDrivesTarget();
    void _connectConfiguredValidationErrors_data();
    void _connectConfiguredValidationErrors();
    void _autoDiscoveryWaitsReconnectsAndDisables();
    void _tcpModeSkipsSerialDiscovery();
    void _excludedPorts_data();
    void _excludedPorts();
    void _autoRetryBacksOffAndRespectsReservations();
    void _compositeReceiverSelection_data();
    void _compositeReceiverSelection();
    void _genericUsbNeedsExplicitSelection_data();
    void _genericUsbNeedsExplicitSelection();
    void _manualConnectionRetiresAutoOwnership();
    void _manualRetryWaitsForReturningPort();
    void _configurationChangeCancelsManualRetry_data();
    void _configurationChangeCancelsManualRetry();
    void _connectSavedWaitsForReceiver();
    void _connectSavedKeepsDiscovery_data();
    void _connectSavedKeepsDiscovery();
    void _shutdownDuringConnectionTick();
    void _manualRetryRecreatesGpsRtkSession();
    void _serialPolicyIntegrationUsesSelectedPort();
};
