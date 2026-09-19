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
    void _nativeConfigurationRejectedBeforeIo_data();
    void _nativeConfigurationRejectedBeforeIo();
    void _invalidRtcmPayload_data();
    void _invalidRtcmPayload();
    void _ubloxRoleTransition_data();
    void _ubloxRoleTransition();
    void _ubloxDisableFailure_data();
    void _ubloxDisableFailure();
    void _ubloxPositionNonBase_data();
    void _ubloxPositionNonBase();
    void _ubloxBaseRoleDefaults_data();
    void _ubloxBaseRoleDefaults();
    void _ubloxAmbiguousAcknowledgements_data();
    void _ubloxAmbiguousAcknowledgements();
    void _ubloxReadbackFailure_data();
    void _ubloxReadbackFailure();
    void _ubloxSbasConfiguration_data();
    void _ubloxSbasConfiguration();
};
