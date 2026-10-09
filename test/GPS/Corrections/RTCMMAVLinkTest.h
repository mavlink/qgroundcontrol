#pragma once

#include "UnitTest.h"

class RTCMMAVLinkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testOutputFanout();
    void _testPartialOutput_data();
    void _testPartialOutput();
    void _testEmpty();
    void _testSequenceAdvances();
};
