#pragma once

#include "UnitTest.h"

class AndroidGPSCompatibilityTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void _backendWriteResults_data();
    void _backendWriteResults();
    void _writesAreBufferedAndReported();
    void _writeWaitTimeoutKeepsPendingBytes();
    void _closeSendsPendingWrites();
    void _readWaitSharesWriteDeadline();
    void _posixWriteTimeoutReportsProgress();
    void _expiredConfigurationSendsNothing();
    void _cancellationAfterFullWriteCompletes();
    void _quietReadTimeoutAndCancellation();
    void _incomingDataIsDeliveredOnOwnerThread();
    void _unsupportedControlLineClassification_data();
    void _unsupportedControlLineClassification();
    void _posixControlLineUnsupported_data();
    void _posixControlLineUnsupported();
};
