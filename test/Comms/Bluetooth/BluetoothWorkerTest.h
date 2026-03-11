#pragma once

#include "UnitTest.h"

class BluetoothWorkerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFactoryCreatesClassicWorker();
    void _testFactoryCreatesBleWorker();
    void _testInitialState();
    void _testWriteEmptyDataEmitsNothing();
    void _testWriteWhileDisconnectedEmitsError();
    void _testConnectWhileConnectedWarns();
    void _testDisconnectResetsReconnectState();
    void _testClassicDiscoveryTimerStopsOnFinished();
    void _testClassicDiscoveryTimerStopsOnCanceled();
    void _testClassicDiscoveryTimerStopsOnError();
    void _testDestroyClassicWorker();
    void _testDestroyBleWorker();
};
