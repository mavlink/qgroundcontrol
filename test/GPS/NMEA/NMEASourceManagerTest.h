#pragma once

#include "UnitTest.h"

class NMEASourceManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _scheduledRetryAndSuspension();
    void _decodesWithoutPositionManager();
    void _managedReceiverSettingsPreservePassiveBaud();
    void _reentrantConnectionCommands_data();
    void _reentrantConnectionCommands();
    void _managedAttemptRetainsReservationWhileStopping();
    void _injectedSerialDiscovery();
    void init();
    void _managedReceiverFailureAndCancellation();
    void _managedModeChangeCancelsRetry();
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
    void _satelliteSnapshotsPreserveProvenance();
    void _satellitesShareTcpConnection();
    void _disconnectDuringSatelliteUpdate();
    void _disconnectDuringPositionUpdate();
};
