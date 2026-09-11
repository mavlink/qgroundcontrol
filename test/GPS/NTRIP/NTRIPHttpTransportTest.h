#pragma once

#include "UnitTest.h"

class NTRIPHttpTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testSourceTableRejectsMountpoint_data();
    void testSourceTableRejectsMountpoint();
    void testBodyBeforeMalformedChunk_data();
    void testBodyBeforeMalformedChunk();
    void testEofFinalization_data();
    void testEofFinalization();
    void testBodyObserverRetiresAttempt_data();
    void testBodyObserverRetiresAttempt();
    void testChunkedCorrectionsYieldBetweenReadBatches();
    void testPublicObserverCanStop_data();
    void testPublicObserverCanStop();
    void testResponseDecoder_data();
    void testResponseDecoder();
    void testNormalizedRequest_data();
    void testNormalizedRequest();
    void testEncodedRequestReachesCaster();
    void testAddressValidation_data();
    void testAddressValidation();

    // Whitelist parsing
    void _testWhitelistParsing_data();
    void _testWhitelistParsing();
    void _testFragmentedReceiptTime();

    // RTCM filtering
    void _testFilterNoWhitelist();
    void _testFilterWithWhitelist();
    void _testFilterRejectsBadCrc();

    // Transport config validation
    void testConfigValidEmpty();
    void testConfigValidGood();
    void testConfigValidBadPort();
    void testConfigRejectsColonUsername();
    void testConfigRejectsControlChars();
    void testConfigDiffClassifiersCoverIndependentFields();
    void testConfigCasterIdentityExcludesMountpointAndFilter();

    // Live TLS error path
    void testTlsFatalErrorEmitsSingleError();
    void testStreamingRequiresMountpoint();
    void testConnectionWaitsForHttpResponse();
    void testHandshakeTimeoutClosesSocket();
    void testRemoteCloseEmitsSingleError();
    void testCorrectionWatchdog_data();
    void testCorrectionWatchdog();

    // HTTP request building
    void _testBuildRequestPlaintextCredentialsWarns();
    void _testBuildRequestTlsCredentialsNoWarn();
    void _testBuildRequestNoCredentialsNoWarn();

    // NMEA checksum repair
    void _testRepairNmeaChecksumCorrect();
    void _testRepairNmeaChecksumWrong();
    void _testRepairNmeaChecksumMissing();
    void _testRepairNmeaChecksumTruncated();
    void _testRepairNmeaChecksumAppendsCrLf();
    void _testRepairNmeaChecksumShortSentence();
};
