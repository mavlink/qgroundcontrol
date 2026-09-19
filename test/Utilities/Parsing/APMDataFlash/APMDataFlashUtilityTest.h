#pragma once

#include "UnitTest.h"

class APMDataFlashUtilityTest : public UnitTest
{
    Q_OBJECT

public:
    APMDataFlashUtilityTest() = default;

private slots:
    // Format character size tests
    void _testFormatCharSize();
    void _testFormatCharSizeUnknown();
    void _testCalculatePayloadSize();

    // Value parsing tests
    void _testParseValue_data();
    void _testParseValue();
    void _testParseValueInvalid_data();
    void _testParseValueInvalid();
    void _testParseValueStrings();

    // Half-precision float tests
    void _testHalfToFloat_data();
    void _testHalfToFloat();
    void _testHalfToFloatSpecial();

    // Header validation tests
    void _testIsValidHeader();
    void _testIsValidHeaderInvalid();
    void _testFindNextHeader();

    // FMT message parsing tests
    void _testParseFmtPayload();
    void _testParseFmtMessages();

    // Message parsing tests
    void _testParseMessage();
    void _testParseMessageTruncated_data();
    void _testParseMessageTruncated();
    void _testParseMessageUnsupportedFormat_data();
    void _testParseMessageUnsupportedFormat();

    // Message iteration tests
    void _testIterateMessages();
    void _testIterateMessagesProgress();
};
