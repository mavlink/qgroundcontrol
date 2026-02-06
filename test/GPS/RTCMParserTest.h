#pragma once

#include "UnitTest.h"

class RTCMParserTest : public UnitTest
{
    Q_OBJECT

private slots:
    // CRC-24Q
    void testCrc24qEmpty();
    void testCrc24qSingleByte();
    void testCrc24qKnownVector();
    void testCrc24qReferenceVector();
    void testCrc24qIncremental();

    // RTCMParser state machine
    void testParserReset();
    void testParserValidMessage();
    void testParserCrcValidation();
    void testParserInvalidCrc();
    void testParserMessageId();
    void testParserGarbageBeforePreamble();
    void testParserInvalidLength();
    void testParserOverlengthRejected();
    void testParserMultipleMessages();
    void testParserMaxLength();
    void testParserTruncatedFrame();
    void testParserCorruptedPreamble();
};
