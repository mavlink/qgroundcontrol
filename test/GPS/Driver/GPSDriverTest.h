#pragma once

class GPSDriver;

#include "UnitTest.h"

class GPSDriverTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _positionBackendWithoutBaseSupport();
    void _invalidConfiguration_data();
    void _invalidConfiguration();
    void _configurationWriteEvidence_data();
    void _configurationWriteEvidence();
    void _familyCancellation_data();
    void _familyCancellation();
    void _observationMetadata();
    void _satelliteAzimuthEncoding_data();
    void _satelliteAzimuthEncoding();
    void _relativePositionCallback();
    void _unsupportedOutputProtocol();
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
    void _testCallbacksWithoutSinksAreSafe();
    void _testDefaultConfigHeadingOffsetMatchesSeptentrioPreset();
    void _positionRoleDoesNotForwardBaseData();
    void _receiverRoleCommands_data();
    void _receiverRoleCommands();
};
