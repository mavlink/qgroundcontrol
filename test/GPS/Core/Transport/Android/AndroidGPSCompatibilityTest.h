#pragma once

#include "UnitTest.h"

/// SerialGPSTransport as Android builds it, over Android's QSerialPort and a simulated Java side
/// (test/Android).
class AndroidGPSCompatibilityTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void _backendWriteResults_data();
    void _backendWriteResults();
    void _expiredConfigurationSendsNothing();
    void _cancellationAfterFullWriteCompletes();
    void _quietReadTimeoutAndCancellation();
};
