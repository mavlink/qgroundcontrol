#pragma once

#include "UnitTest.h"

class LogFormatterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFormatText();
    void _testFormatJson();
    void _testFormatJsonSourceLocation();
    void _testFormatCsvRow();
    void _testCsvHeader();
    void _testFormatAsText();
    void _testFormatAsJson();
    void _testFormatAsCsv();
    void _testFormatAsJsonLines();
    void _testFormatDispatch();
    void _testFileExtension();
    void _testMimeType();
    void _testCsvEscaping();
    void _testSourceLocationGating();
};
