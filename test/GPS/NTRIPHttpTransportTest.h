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
    void _testWhitelistEmpty();
    void _testWhitelistSingle();
    void _testWhitelistMultiple();
    void _testWhitelistInvalidEntries();

    // RTCM filtering
    void _testFilterNoWhitelist();
    void _testFilterWithWhitelist();
    void _testFilterRejectsBadCrc();

    // NMEA checksum repair
    void _testRepairNmeaChecksumCorrect();
    void _testRepairNmeaChecksumWrong();
    void _testRepairNmeaChecksumMissing();
    void _testRepairNmeaChecksumTruncated();
    void _testRepairNmeaChecksumAppendsCrLf();
    void _testRepairNmeaChecksumShortSentence();
};
