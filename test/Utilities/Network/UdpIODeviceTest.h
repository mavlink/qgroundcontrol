#pragma once

#include "UnitTest.h"

class UdpIODeviceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _selectedPeerIsolation();
    void _byteAccountingAndPeek();
    void _fragmentedAndPartialLines();
    void _nmeaStartDiscardsBufferedData();
    void _overflowKeepsNewestLines();
    void _overflowDiscardsPartialLine();
    void _closeClearsBufferedData();
};
