#pragma once

#include "UnitTest.h"

class RTKEndToEndTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Full pipeline: MockGPSRtk survey-in -> RTCM inject -> RTCMRouter -> RTCMMavlink
    void testSurveyInToMavlinkPipeline();
    void testMultiDeviceRtcmRouting();
    void testLargeRtcmFragmentedToDevice();
    void testDeviceDisconnectStopsRouting();
    void testFactGroupReflectsSurveyInProgress();
};
