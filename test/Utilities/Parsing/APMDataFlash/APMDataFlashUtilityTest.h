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
    UT_PARAMETERIZED_TEST(_testParseValue);
    UT_PARAMETERIZED_TEST(_testParseValueInvalid);

    // Half-precision float tests
    UT_PARAMETERIZED_TEST(_testHalfToFloat);
    void _testHalfToFloatSpecial();

    // Header validation tests
    void _testIsValidHeader();
    void _testIsValidHeaderInvalid();
    UT_PARAMETERIZED_TEST(_testFindNextHeader);

    // FMT message parsing tests
    void _testParseFmtPayload();
    void _testParseFmtMessages();
    void _testParseFmtMessagesInstanceColumn();
    void _testParseFmtMessagesInvalidLength();

    // Message parsing tests
    void _testParseMessage();
    UT_PARAMETERIZED_TEST(_testParseMessageTruncated);
    void _testParseMessageWithoutPayload();
    UT_PARAMETERIZED_TEST(_testParseMessageUnsupportedFormat);

    // Message iteration tests
    void _testIterateMessages();
    void _testIterateMessagesProgress();
    UT_PARAMETERIZED_TEST(_testIterateMessagesFormatShorterThanHeader);
    UT_PARAMETERIZED_TEST(_testTruncatedLog);
};
