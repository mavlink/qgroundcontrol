#pragma once

#include "UnitTest.h"

class PX4ULogUtilityTest : public UnitTest
{
    Q_OBJECT

public:
    PX4ULogUtilityTest() = default;

private slots:
    // Header validation tests
    void _testIsValidHeader();
    void _testIsValidHeaderInvalid();

    // Version detection tests
    void _testGetVersion();
    void _testGetVersionInvalid();

    // Timestamp tests
    UT_PARAMETERIZED_TEST(_testGetHeaderTimestamp);

    // Message iteration tests
    void _testIterateMessagesMultiple();
    void _testIterateMessagesStops();
    void _testIterateMessagesAcrossChunks();
    void _testIterateMessagesUnknownMessage();
    void _testIterateMessagesFatalError();
    UT_PARAMETERIZED_TEST(_testIterateMessagesTruncated);
};
