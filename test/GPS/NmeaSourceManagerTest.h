#pragma once

#include "UnitTest.h"

class NmeaSourceManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init();
    void _udpSwitchAndDisable();
    void _bindFailureAndTeardown();
    void _configuredSerialRoutingSurvivesReconnect();
    void _settingsUseSharedSerialInventory_data();
    void _settingsUseSharedSerialInventory();
};
