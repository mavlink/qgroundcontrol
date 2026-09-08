#pragma once

#include "UnitTest.h"

class NmeaSourceManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init();
    void _udpSwitchAndDisable();
    void _udpActivityStatus();
    void _bindFailureAndTeardown();
    void _configuredSerialRoutingSurvivesReconnect();
    void _tcpRecoveryAndSourceSwitch();
    void _tcpManualAndAutoConnect();
    void _tcpRefusalBackoff();
    void _settingsUseSharedSerialInventory();
};
