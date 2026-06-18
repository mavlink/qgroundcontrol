// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

#include "CommsTest.h"

// End-to-end SerialLink coverage through LinkManager::createConnectedLink, with a loopback
// MockSerialPort injected via SerialPlatform::setPortFactoryForTest(). Exercises the worker-thread
// marshalling that SerialWorkerTest can't: queued connected/disconnected/bytesReceived and the
// communicationError path, all driven by the real link stack.
class SerialLinkTest : public CommsTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _loopbackRoundTrip_deliversBytesReceived();
    void _openFailure_emitsCommunicationError();
};
