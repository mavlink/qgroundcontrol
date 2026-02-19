#pragma once

#include "UnitTest.h"

class GStreamerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;

    void _testIsValidRtspUri();
    void _testIsHardwareDecoderFactory();
    void _testSetCodecPrioritiesDefault();
    void _testSetCodecPrioritiesSoftware();
    void _testSetCodecPrioritiesHardware();
    void _testRedirectGLibLogging();
};
