#pragma once

#include "UnitTest.h"

class RTKConnectionPolicyTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _discoveryUnplugAndDisable();
    void _tcpModeSkipsSerialDiscovery();
    void _compositeReceiverSelection_data();
    void _compositeReceiverSelection();
    void _excludedPorts_data();
    void _excludedPorts();
    void _failedAttemptsBackOffAndRespectReservations();
    void _genericUsbNeedsExplicitSelection_data();
    void _genericUsbNeedsExplicitSelection();
    void _manualConnectionRetiresAutoOwnership();
    void _manualRetryWaitsForReturningPort();
    void _notificationSupersedesDiscovery_data();
    void _notificationSupersedesDiscovery();
    void _shutdownDuringConnectionTick();

private:
    struct Fixture;
};
