#pragma once

#include "UnitTest.h"

// End-to-end pipeline tests: MockGPSRtk survey-in produces RTCM, which is
// routed through the real RTCMMavlink and asserted on the emitted MAVLink
// GPS_RTCM_DATA frames. For mock-backed device orchestration tests see
// RTKIntegrationTest.
class RTKEndToEndTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Full pipeline: MockGPSRtk survey-in -> RTCM inject -> RTCMMavlink
    void testSurveyInToMavlinkPipeline();
    void testMultiDeviceRtcmRouting();
    void testLargeRtcmFragmentedToDevice();
    void testDeviceDisconnectStopsRouting();
    void testFactGroupReflectsSurveyInProgress();
};
