#pragma once

#include "UnitTest.h"

class RTCMParserTest : public UnitTest
{
    Q_OBJECT

private slots:
    // CRC-24Q
    void _testCrc24qEmpty();
    void _testCrc24qSingleByte();
    void _testCrc24qKnownVector();
    void _testCrc24qReferenceVector();
    void _testCrc24qIncremental();

    // RTCMParser state machine
    void _testParserReset();
    void _testParserValidMessage();
    void _testParserCrcValidation();
    void _testParserInvalidCrc();
    void _testParserMessageId();
    void _testParserGarbageBeforePreamble();
    void _testParserInvalidLength();
    void _testParserOverlengthRejected();
    void _testParserMultipleMessages();
    void _testParserMaxLength();
    void _testParserTruncatedFrame();
    void _testParserCorruptedPreamble();
};
