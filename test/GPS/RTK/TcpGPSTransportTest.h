#pragma once

#include "UnitTest.h"

class TcpGPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _transferTimeoutAndPeerClose();
    void _cancelWait_data();
    void _cancelWait();
    void _refusedConnection();
};
