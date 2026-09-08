#pragma once

#include "UnitTest.h"

class RTKAutoConnectTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _networkRetriesAndStops();
#ifndef QGC_NO_SERIAL_LINK
    void _manualSerialSelectionAndPause();
    void _discoveryUnplugAndDisable();
    void _excludedPorts_data();
    void _excludedPorts();
    void _failedAttemptsBackOffAndRespectReservations();
    void _failedOpenRetriesWithoutUnplug();
#endif
};
