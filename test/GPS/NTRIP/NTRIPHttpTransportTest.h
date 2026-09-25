#pragma once

#include "UnitTest.h"

class NTRIPHttpTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Whitelist parsing
    void _testWhitelist_data();
    void _testWhitelist();

    void _testParseHttpStatus_data();
    void _testParseHttpStatus();
    void _testHttpDecoderReset();

    // RTCM filtering
    void _testFilterNoWhitelist();
    void _testFilterWithWhitelist();
    void _testFilterRejectsInvalidFrame_data();
    void _testFilterRejectsInvalidFrame();
    void _testFrameCallbackRetiresTransport_data();
    void _testFrameCallbackRetiresTransport();

    // Transport config validation
    void testConfigValidEmpty();
    void testConfigValidGood();
    void testConfigValidBadPort();
    void testConfigRejectsColonUsername();
    void testConfigRejectsControlChars();
    void testConfigurationDomainsCompareIndependently();

    // Live TLS error path
    void testTlsFatalErrorEmitsSingleError();
    void testStreamingRequiresMountpoint();
    void testConnectionWaitsForHttpResponse();
    void testHandshakeTimeoutClosesSocket();
    void testRemoteCloseEmitsSingleError();
    void warningRetiresAttempt_data();
    void warningRetiresAttempt();
    void nmeaLogsMetadataOnly();
    void writeAdmissionFailure_data();
    void writeAdmissionFailure();
    void handshakeRetiresAttempt_data();
    void handshakeRetiresAttempt();
    void failureCanRestart_data();
    void failureCanRestart();
    void legacyCaster_data();
    void legacyCaster();
    void icyPartialFrame_data();
    void icyPartialFrame();
    void errorDiagnostics_data();
    void errorDiagnostics();
    void errorBodyBounds();
    void errorBodyDeadline();
    void errorBodySocketFailure();
    void pendingErrorRetiresAttempt_data();
    void pendingErrorRetiresAttempt();
    void httpFraming_data();
    void httpFraming();
    void retryAfter_data();
    void retryAfter();
    void bodyPublicationRetiresAttempt_data();
    void bodyPublicationRetiresAttempt();
    void receiptTimesAndEvidence_data();
    void receiptTimesAndEvidence();
    void socketTermination_data();
    void socketTermination();
    void filterConfigurationUpdatesWithoutReconnect();
    void validFrameWatchdog_data();
    void validFrameWatchdog();

    // HTTP request building
    void _testBuildRequestPlaintextCredentialsWarns();
    void _testBuildRequestTlsCredentialsNoWarn();
    void _testBuildRequestNoCredentialsNoWarn();
    void _testBuildRequestPreservesValues_data();
    void _testBuildRequestPreservesValues();
    void _testBuildRequestAuthority_data();
    void _testBuildRequestAuthority();
    void _testBuildRequestRejectsInvalidConfig_data();
    void _testBuildRequestRejectsInvalidConfig();

private:
    void _expectDebugMessage(const char* category, const QString& message);
    void _verifyDebugMessage();
};
