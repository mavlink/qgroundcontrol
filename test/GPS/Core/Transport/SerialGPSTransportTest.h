#pragma once

#include "UnitTest.h"

class SerialGPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testOperationsAbortWhenStopRequested();
    void _openFailureNamesCause();
    void _testCancelPendingOperation_data();
    void _testCancelPendingOperation();
    void _testPendingWriteDeadline();
    void _inputBudgetEndsStream();
    void _consecutiveWrites();
    void _refusedBaudrateKeepsLink();
    void _openRetriesUntilTimeout_data();
    void _openRetriesUntilTimeout();
    void _openRetriesUntilReleased();
};
