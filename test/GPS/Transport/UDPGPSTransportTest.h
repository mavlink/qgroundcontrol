#pragma once

#include "UnitTest.h"

class UDPGPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _receivesSelectedSender();
    void _idleSenderIsReplaced();
    void _receiveOnlyLink();
    void _cancelledRead();
    void _portInUse();
};
