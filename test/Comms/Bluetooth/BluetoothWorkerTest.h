#pragma once

#include "UnitTest.h"

class BluetoothLink;
class BluetoothWorker;

class BluetoothWorkerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _testFactoryCreatesClassicWorker();
    void _testFactoryCreatesBleWorker();
    void _testMacLinkKeepsWorkerOnMainThread();
    void _testInitialState();
    void _testWriteEmptyDataEmitsNothing();
    void _testWriteWhileDisconnectedEmitsError();
    void _testConnectWhileConnectedWarns();
    void _testDisconnectResetsReconnectState();
    void _testClassicServiceSelection_data();
    void _testClassicServiceSelection();
    void _testClassicDiscoveryFailure_data();
    void _testClassicDiscoveryFailure();
    void _testPermissionDefersConnectionUntilGranted();
    void _testPermissionDeniedTerminatesConnection();
    void _testCanceledPermissionDoesNotConnect_data();
    void _testCanceledPermissionDoesNotConnect();
    void _testDestroyUninitializedLink();
    void _testClassicSocketErrorHasDescription();
    void _testDestroyClassicWorker();
    void _testDestroyBleWorker();

private:
    void _preparePendingPermission(BluetoothLink& link, BluetoothWorker* worker);
};
