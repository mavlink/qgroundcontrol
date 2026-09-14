#pragma once

#include "UnitTest.h"

class RTKAutoConnectTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _discoveryUnplugAndDisable();
    void _excludedPorts_data();
    void _excludedPorts();
    void _failedAttemptsBackOffAndRespectReservations();
    void _failedOpenRetriesWithoutUnplug();
};
