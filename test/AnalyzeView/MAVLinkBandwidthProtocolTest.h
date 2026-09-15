#pragma once

#include "UnitTest.h"

class MAVLinkBandwidthProtocolTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _roundTripTest_data();
    void _roundTripTest();
    void _maximumDataTest();
    void _invalidHeaderTest_data();
    void _invalidHeaderTest();
    void _invalidDataLengthTest();
};
