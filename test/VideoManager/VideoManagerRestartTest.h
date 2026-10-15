#pragma once

#include "UnitTest.h"

/// VideoManager's half of the reconnect contract: it restarts a receiver only after a stop it requested and
/// leaves recovery from stream failures to the receiver.
class VideoManagerRestartTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testRestartsOnlyAfterRequestedStop_data();
    void _testRestartsOnlyAfterRequestedStop();
    void _testStopVideoStopsReceiverWaitingToReconnect();
};
