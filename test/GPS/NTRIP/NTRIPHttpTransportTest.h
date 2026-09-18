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
    void _testBuildRequestPreservesValues();
    void _testBuildRequestRejectsInvalidConfig_data();
    void _testBuildRequestRejectsInvalidConfig();
};
