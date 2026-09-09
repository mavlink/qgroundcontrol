#pragma once

#include "UnitTest.h"

class RTCMUdpInputTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStartStop();
    void _testPassthroughWithoutValidation();
    void _testEmitsOneSignalPerFrame();
    void _testDropsBadCrcFrame();
    void _testFrameSplitAcrossDatagrams();
};
