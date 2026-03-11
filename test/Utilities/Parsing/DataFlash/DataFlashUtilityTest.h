#pragma once

#include "UnitTest.h"

class DataFlashUtilityTest : public UnitTest
{
    Q_OBJECT

public:
    DataFlashUtilityTest() = default;

private slots:
    // Format character size tests
    void _testFormatCharSize();
    void _testFormatCharSizeUnknown();
    void _testCalculatePayloadSize();

    // Value parsing tests
    void _testParseValueIntegers();
    void _testParseValueScaled();
    void _testParseValueFloats();
    void _testParseValueStrings();

    // Half-precision float tests
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

    // Message iteration tests
    void _testIterateMessages();
};
