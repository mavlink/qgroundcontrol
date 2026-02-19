#pragma once

#include "TempDirectoryTest.h"

class ULogParserTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    void _getTagsFromLogTest();
    void _getTagsFromLogEmptyTest();
    void _getTagsFromLogInvalidTest();
    void _parseGeoTagDataFieldsTest();
    void _generatedULogTest();
    void _benchmarkGetTagsFromLog();
};
