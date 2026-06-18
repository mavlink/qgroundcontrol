#pragma once

#include "UnitTest.h"

class QGCSerialPortInfoTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testLoadJsonData();
    void _testLoadJsonDataIdempotent();
    void _testBoardTypeStringMapping_data();
    void _testBoardTypeStringMapping();
    void _testBoardClassStringToTypeCaseInsensitivity();
    void _testBoardInfoListEntriesAreWellFormed();
    void _testFallbackRegexesCompile();
    void _testFallbackSchemaIsPlatformNeutral();
    void _testLinuxSystemPortFiltering();
};
