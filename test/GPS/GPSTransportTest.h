#pragma once

#include "UnitTest.h"

class GPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testMockTransport();
    void testOpenCloseLifecycle();
    void testReadEmptyBuffer();
    void testWriteWhenClosed();
    void testSetBaudRate();
    void testBytesAvailableAccuracy();
    void testPartialRead();
};
