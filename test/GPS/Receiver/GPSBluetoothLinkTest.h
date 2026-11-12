#pragma once

#include "UnitTest.h"

/// The Bluetooth end of a receiver session, without a radio: the receiver's Bluetooth side, the pipe of a session
/// whose transport runs on a thread of its own, and a worker of the application's Bluetooth links that the tests
/// script.
class GPSBluetoothLinkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _linkCarriesBytesAndStopsItsWorker();
    void _linkRefusesToOpen_data();
    void _linkRefusesToOpen();
    void _discoveryIsTheLinkConfiguration();
};
