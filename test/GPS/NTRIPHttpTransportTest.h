#pragma once

#include "UnitTest.h"

class NTRIPHttpTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    // HTTP status line parsing
    void testParseHttpStatus200();
    void testParseHttpStatusICY();
    void testParseHttpStatus401();
    void testParseHttpStatus404();
    void testParseHttpStatusInvalid();
    void testParseHttpStatus201();
    void testParseHttpStatus500();
    void testParseHttpStatusNoReason();

    // Whitelist parsing
    void testWhitelistEmpty();
    void testWhitelistSingle();
    void testWhitelistMultiple();
    void testWhitelistInvalidEntries();

    // RTCM filtering
    void testFilterNoWhitelist();
    void testFilterWithWhitelist();
    void testFilterRejectsBadCrc();

    // NMEA checksum repair
    void testRepairNmeaChecksumCorrect();
    void testRepairNmeaChecksumWrong();
    void testRepairNmeaChecksumMissing();
    void testRepairNmeaChecksumTruncated();
    void testRepairNmeaChecksumAppendsCrLf();
    void testRepairNmeaChecksumShortSentence();
};
