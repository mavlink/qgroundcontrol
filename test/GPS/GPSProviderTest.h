#pragma once

#include "UnitTest.h"

class GPSProviderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testSurveyInStatusDataDefaults();
    void testRtkDataDefaults();
    void testGPSTypeEnum();
    void testGPSTypeFromString();
    void testGPSTypeDisplayName();

    // Callback chain / lifecycle tests
    void testTransportOpenFailEmitsError();
    void testRtcmInjection();
    void testDrainRtcmBufferEmpty();
    void testReconnectDelay();
};
