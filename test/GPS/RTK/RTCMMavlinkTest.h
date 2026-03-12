#pragma once

#include "UnitTest.h"

class RTCMMavlinkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testTotalBytesSent();
    void testSmallMessageNotFragmented();
    void testLargeMessageFragmented();
    void testSequenceIdIncrements();
};
