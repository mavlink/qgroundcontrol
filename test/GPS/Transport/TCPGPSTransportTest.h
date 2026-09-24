#pragma once

#include "UnitTest.h"

class TCPGPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _immediateRead_data();
    void _immediateRead();
    void _cancelFromAnotherThread_data();
    void _cancelFromAnotherThread();
    void _transferTimeoutAndPeerClose();
    void _cancelWait_data();
    void _cancelWait();
    void _refusedConnection();
    void _boundedIngressPreservesStream();
    void _boundedWriteEvidence_data();
    void _boundedWriteEvidence();
};
