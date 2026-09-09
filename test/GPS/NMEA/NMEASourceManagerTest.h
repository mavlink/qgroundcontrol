#pragma once

#include "UnitTest.h"

class NMEASourceManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init();
    void _receiverPreparationFailureAndCancellation();
    void _udpSwitchAndDisable();
    void _inactiveSettingsKeepConnection();
    void _udpActivityStatus();
    void _bindFailureAndTeardown();
    void _configuredSerialRoutingSurvivesReconnect();
    void _tcpRecoveryAndSourceSwitch();
    void _tcpManualAndAutoConnect();
    void _tcpRefusalBackoff();
    void _settingsUseSharedSerialInventory();
    void _satellitesShareUdpAndStayFresh();
    void _satellitesShareTcpConnection();
    void _disconnectDuringSatelliteUpdate();
    void _disconnectDuringPositionUpdate();
};
