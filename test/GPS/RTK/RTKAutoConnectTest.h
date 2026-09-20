#pragma once

#include "UnitTest.h"

class RTKAutoConnectTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _discoveryUnplugAndDisable();
    void _compositeReceiverSelection_data();
    void _compositeReceiverSelection();
    void _excludedPorts_data();
    void _excludedPorts();
    void _failedAttemptsBackOffAndRespectReservations();
    void _failedOpenRetriesWithoutUnplug();
    void _genericUsbNeedsExplicitSelection_data();
    void _genericUsbNeedsExplicitSelection();
    void _manualConnectionRetiresAutoOwnership();
    void _notificationSupersedesDiscovery_data();
    void _notificationSupersedesDiscovery();
    void _shutdownDuringConnectionTick();
};
