#pragma once

#include "UnitTest.h"

class RTCMMAVLinkPacketTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _packetization_data();
    void _packetization();
};
