#pragma once

#include "UnitTest.h"

class VideoManagerRestartTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testRestartsOnlyAfterRequestedStop_data();
    void _testRestartsOnlyAfterRequestedStop();
    void _testStopVideoStopsReceiverWaitingToReconnect();
};
