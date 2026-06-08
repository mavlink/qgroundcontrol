// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

#include "UnitTest.h"

// Drives SerialWorker synchronously on the test thread with a MockSerialPort injected via
// SerialPlatform::setPortFactoryForTest() — covers the connect/disconnect lifecycle, RX/TX paths,
// write backpressure, single-shot error emission, and port error handling without hardware.
class SerialWorkerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _connectThenDisconnect_emitsLifecycle();
    void _receivedData_emittedToWorker();
    void _writeData_capturedAndEmitsDataSent();
    void _writeBackpressure_holdsThenDrainsOnResume();
    void _writeFailure_emitsErrorAndClearsBacklog();
    void _writeWhenNotConnected_emitsErrorOnce();
    void _openFailure_emitsErrorAndStaysDisconnected();
    void _reconfigureFailure_emitsErrorAndStaysDisconnected();
    void _resourceError_disconnectsWithoutErrorSignal();
    void _genericPortError_emitsErrorSignal();
    void _connectWithoutSetup_emitsPortNotCreated();
    void _factoryOverride_makeSerialPortReturnsMock();
};
