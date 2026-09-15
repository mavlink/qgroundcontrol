#pragma once

#include "UnitTest.h"

class NTRIPHttpTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    // HTTP status line parsing
    void _testParseHttpStatus200();
    void _testParseHttpStatusICY();
    void _testParseHttpStatus401();
    void _testParseHttpStatus404();
    void _testParseHttpStatusInvalid();
    void _testParseHttpStatus201();
    void _testParseHttpStatus500();
    void _testParseHttpStatusNoReason();

    // Whitelist parsing
    void _testWhitelist_data();
    void _testWhitelist();

    // RTCM filtering
    void _testFilterNoWhitelist();
    void _testFilterWithWhitelist();
    void _testFilterRejectsInvalidFrame_data();
    void _testFilterRejectsInvalidFrame();
    void _testCompatibilityCallbackRetiresTransport_data();
    void _testCompatibilityCallbackRetiresTransport();
    void _testMockCompatibilityProjection_data();
    void _testMockCompatibilityProjection();

    // Transport config validation
    void testConfigValidEmpty();
    void testConfigValidGood();
    void testConfigValidBadPort();
    void testConfigRejectsColonUsername();
    void testConfigRejectsControlChars();
    void testConfigDiffClassifiersCoverIndependentFields();
    void testConfigCasterIdentityExcludesMountpointAndSinks();

    // Live TLS error path
    void testTlsFatalErrorEmitsSingleError();
    void testStreamingRequiresMountpoint();
    void testConnectionWaitsForHttpResponse();
    void testHandshakeTimeoutClosesSocket();
    void testRemoteCloseEmitsSingleError();

    // HTTP request building
    void _testBuildRequestPlaintextCredentialsWarns();
    void _testBuildRequestTlsCredentialsNoWarn();
    void _testBuildRequestNoCredentialsNoWarn();
};
