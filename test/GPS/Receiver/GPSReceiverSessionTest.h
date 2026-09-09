#pragma once

#include "UnitTest.h"

class GPSReceiverSessionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _cancelBeforeStart_data();
    void _cancelBeforeStart();
    void _nmeaStreamBoundsPendingBytes();
    void _nmeaStreamPreservesReceiptAge_data();
    void _nmeaStreamPreservesReceiptAge();
    void _nmeaStreamDiscardsPartialFixAcrossGap();
    void _destroyDuringStreamClose();
};
