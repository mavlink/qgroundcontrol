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
    void _notificationSupersedesLifecycle_data();
    void _notificationSupersedesLifecycle();
    void _externalReplacementKeepsOwnership_data();
    void _externalReplacementKeepsOwnership();
    void _configuredSerialRoutingSurvivesReconnect();
    void _settingsUseSharedSerialInventory_data();
    void _settingsUseSharedSerialInventory();
};
