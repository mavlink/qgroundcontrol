#pragma once

#include "UnitTest.h"

class RTKAutoConnectTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _receiverErrorDetailReachesStatus();
    void _networkRetriesAndStops();
    void _disconnectDoesNotBlockAndReconnectWaits();
#ifndef QGC_NO_SERIAL_LINK
    void _manualSerialSelectionAndPause();
    void _serialRetriesKeepConfiguration();
    void _discoveryUnplugAndDisable();
    void _excludedPorts_data();
    void _excludedPorts();
    void _failedAttemptsBackOffAndRespectReservations();
    void _failedOpenRetriesWithoutUnplug();
    void _nmeaDiscoveryExclusionDoesNotRevokeReceiver();
#endif
};
