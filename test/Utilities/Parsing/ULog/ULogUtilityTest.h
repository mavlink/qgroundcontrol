#pragma once

#include "UnitTest.h"

class ULogUtilityTest : public UnitTest
{
    Q_OBJECT

public:
    ULogUtilityTest() = default;

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
};
