#pragma once

#include "UnitTest.h"

class UDPGPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _cancelFromAnotherThread();
    void _transferAndPartialReads_data();
    void _transferAndPartialReads();
    void _cancelRead();
    void _bindFailure();
    void _oversizedWriteIsRejectedWithoutRetiringPeer();
};
