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
    void _testIsValidHeaderQByteArray();

    // Version detection tests
    void _testGetVersion();
    void _testGetVersionInvalid();

    // Timestamp tests
    void _testGetHeaderTimestamp();

    // Message iteration tests
    void _testIterateMessages();
    void _testIterateMessagesMultiple();
    void _testIterateMessagesUnknownMessage();
    void _testIterateMessagesFatalError();
};
