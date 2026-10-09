#pragma once

#include "UnitTest.h"

class GPSTransportContractTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void _readDeliversBytes_data();
    void _readDeliversBytes();
    void _emptyReadTimesOut_data();
    void _emptyReadTimesOut();
    void _stopWakesBlockedRead_data();
    void _stopWakesBlockedRead();
    void _writeReachesPeer_data();
    void _writeReachesPeer();
    void _closedPeerIsTerminal_data();
    void _closedPeerIsTerminal();
};
