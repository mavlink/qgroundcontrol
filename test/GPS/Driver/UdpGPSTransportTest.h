#pragma once

#include "UnitTest.h"

class UdpGPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _transferAndPartialReads_data();
    void _transferAndPartialReads();
    void _cancelRead();
    void _bindFailure();
};
