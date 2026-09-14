#pragma once

#include "UnitTest.h"

class GPSDriverTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testReceiveUnconfiguredReturnsError();
    void _testReadDeviceDataRoutesToTransport();
    void _testWriteDeviceDataRoutesToTransport();
    void _testWriteDeviceDataErrorPropagates();
    void _testSetBaudrateRoutesToTransport();
    void _testRtcmMessageForwardedToSink();
    void _testSurveyInStatusTranslatedAndFlagsDecoded();
    void _testSurveyInStatusPreservesLargeValues();
    void _testSurveyInAccuracy_data();
    void _testSurveyInAccuracy();
    void _testSurveyInStatusNullDataIgnored();
    void _testSurveyInCoordinates_data();
    void _testSurveyInCoordinates();
    void _testInvalidConfiguration_data();
    void _testInvalidConfiguration();
    void _testInvalidFixedBaseRejected_data();
    void _testInvalidFixedBaseRejected();
    void _testCallbacksWithoutSinksAreSafe();
    void _testUnknownCallbackIgnored();
    void _transportResultsMapToLegacyCallbacks();
};
