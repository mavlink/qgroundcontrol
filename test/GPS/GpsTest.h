#pragma once

#include "UnitTest.h"

class GpsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testGpsRTCM();
    void _testRtcmParserIgnoresBytesUntilPreamble();
    void _testRtcmParserRecoversAfterInvalidLength();
    void _testRtcmParserRejectsOversizedLengthAndRecovers();
    void _testRtcmParserResetClearsPartialState();
    void _testNtripAutoStartDisabledDefersSocketCreation();
    void _testNtripAutoStartDefaultCreatesSocket();
    void _testNtripWhitelistFiltersMessages();
    void _testNtripSpartnHeaderIsStrippedOnlyOnce();
};
