#pragma once

#include "UnitTest.h"

class SerialGPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testReadAbortsWhenStopRequested();
    void _testWriteAbortsWhenStopRequested();
    void _testCancelPendingOperation_data();
    void _testCancelPendingOperation();
    void _testPendingWriteDeadline();
    void _testLowBaudCorrectionAllowance_data();
    void _testLowBaudCorrectionAllowance();
};
