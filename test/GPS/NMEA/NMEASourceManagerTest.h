#pragma once

#include "UnitTest.h"

class NMEASourceManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init();
    void _udpActivityAndSatellites_data();
    void _udpActivityAndSatellites();
    void _udpSwitchAndDisable();
    void _bindFailureAndTeardown();
    void _configuredSerialRoutingSurvivesReconnect();
    void _settingsUseSharedSerialInventory_data();
    void _settingsUseSharedSerialInventory();
};
