#pragma once

#include "UnitTest.h"

class GPSDriverTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testReceiveUnconfiguredReturnsError();
    void _testReadDeviceDataRoutesToTransport();
    void _testReadCancellation_data();
    void _testReadCancellation();
    void _testCancelledConfigurationDoesNotWarn();
    void _testWriteDeviceDataRoutesToTransport();
    void _testWriteDeviceDataErrorPropagates();
    void _testSetBaudrateRoutesToTransport();
    void _testFixedTransportBaudrate();
    void _testRtcmMessageForwardedToSink();
    void _testSurveyInStatusTranslatedAndFlagsDecoded();
    void _testSurveyInStatusPreservesLargeValues();
    void _testSurveyInStatusNullDataIgnored();
    void _testCallbacksWithoutSinksAreSafe();
    void _testDefaultConfigHeadingOffsetMatchesSeptentrioPreset();
    void _testUnknownCallbackIgnored();
};
