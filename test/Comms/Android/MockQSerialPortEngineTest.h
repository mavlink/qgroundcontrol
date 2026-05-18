// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "UnitTest.h"

// Validates the MockQSerialPortEngine implementation of IQSerialPortEngine. Establishes
// the contract that future MockQSerialPortEngine-backed integration tests rely on.
class MockQSerialPortEngineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _openClose_transitionsState();
    void _openShouldFail_returnsFalseLeavesClosed();
    void _setters_failBeforeOpen();
    void _setters_succeedAfterOpen();
    void _writeSync_recordsBytes_returnsLength();
    void _writeAsync_recordsBytes_returnsLength();
    void _writeFailure_returnsInjectedValue();
    void _purgeBuffers_clearsInputStaging();
    void _waitForReadyRead_drainsStaging();
    void _simulateRead_invokesDataReady();
    void _simulateClose_invokesCloseNotification();
    void _simulateException_invokesExceptionNotification();
    void _readThread_lifecycle();
    void _controlLines_returnsInjectedMask();
};
